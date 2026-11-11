// The logic that will live in the Android app: turns agent events into device
// messages and enforces every approval rule. The device itself is never trusted.
import { randomBytes } from "node:crypto";
import { canApproveOnDevice, classifyApproval } from "./risk.mjs";

const APPROVAL_KINDS = new Set(["command", "file-read", "file-change", "permission", "mcp-elicitation"]);
const MAX_TASK_TEXT = 2_000;
const MAX_TASKS = 20;
const DETAIL_RETRIES = 4;

export class DeviceBridge {
  #backend;
  #send;
  #now;
  #ttlMs;
  #retryMs;
  #profiles;
  #activity;
  #tasks = new Map();
  #approvals = new Map();
  #loading = new Set();
  #online = false;
  #synced = false;
  #timer = null;
  #unsubscribe = null;

  constructor({ backend, send, profiles = [], onActivity = () => {}, now = Date.now, ttlMs = 120_000, retryMs = 250 }) {
    this.#backend = backend;
    this.#send = send;
    this.#profiles = profiles;
    this.#activity = onActivity;
    this.#now = now;
    this.#ttlMs = ttlMs;
    this.#retryMs = retryMs;
  }

  start() {
    this.#unsubscribe = this.#backend.subscribe((event) => this.#onAgentEvent(event));
    this.#timer = setInterval(() => this.expireApprovals(), 1_000);
    this.#timer.unref?.();
  }

  stop() {
    this.#unsubscribe?.();
    clearInterval(this.#timer);
  }

  /** Everything a newly connected device needs. */
  snapshotMessages() {
    const open = [...this.#approvals.values()].filter((a) => a.state === "open");
    return [
      this.#snapshot(),
      { type: "profiles.sync", profiles: this.#deviceProfiles() },
      ...open.map((a) => this.#requestMessage(a)),
    ];
  }

  phoneQueue() {
    return [...this.#approvals.values()]
      .filter((a) => ["open", "expired"].includes(a.state))
      .map((a) => ({
        id: a.id, agent: a.agent, title: a.title, kind: a.kind, risk: a.risk,
        reason: a.reason, text: a.text, state: a.state, deviceApprovable: a.deviceApprovable,
      }));
  }

  async handleDeviceMessage(message) {
    switch (message?.type) {
      case "approval.response":
        return this.#onDeviceAnswer(message);
      case "task.start":
        return this.#startTask(message);
      case "task.stop":
        return this.#taskAction(message.id, () => this.#backend.stopTask(message.id));
      case "task.reply":
        return this.#reply(message);
      default:
        this.#log("rejected", { reason: "unknown message", messageType: message?.type });
    }
  }

  /** Phone path: the only way to answer high-risk or long requests. */
  async answerFromPhone({ id, decision, biometricOk }) {
    const approval = this.#approvals.get(id);
    if (!approval || !["open", "expired"].includes(approval.state)) return this.#result(id, "stale");
    if (decision === "approve" && approval.risk === "high" && !biometricOk) {
      return this.#result(id, "biometric_required");
    }
    return this.#answer(approval, decision, "phone");
  }

  expireApprovals() {
    for (const approval of this.#approvals.values()) {
      if (approval.state === "open" && this.#now() >= approval.expires) {
        // Silence is never consent. It stays pending in T3 and moves to the phone.
        approval.state = "expired";
        this.#log("expired", this.#audit(approval));
        this.#send({ type: "approval.cancel", id: approval.id, reason: "expired" });
      }
    }
  }

  #deviceProfiles() {
    return this.#profiles.map(({ id, name, icon, presets }) => ({
      id, name, icon, presets: presets.map((p) => p.label),
    }));
  }

  #onAgentEvent(event) {
    if (event.type === "connection") return this.#onConnection(event);
    if (event.type === "snapshot") return this.#onSnapshot(event.threads);
    if (event.type === "thread-removed") {
      this.#tasks.delete(event.threadId);
      this.#clearApprovals((a) => a.threadId === event.threadId, "resolved_elsewhere");
      return this.#send(this.#snapshot());
    }
    if (event.type === "thread") this.#onThread(event.thread);
  }

  #onConnection({ online }) {
    this.#online = online;
    if (!online) {
      this.#synced = false;
      // Without T3 nothing can be answered, so withdraw everything. A reconnect re-issues it.
      this.#clearApprovals(() => true, "offline");
    }
    this.#send(this.#snapshot());
  }

  #onSnapshot(threads) {
    this.#online = true;
    this.#tasks = new Map(threads.map((t) => [t.id, t]));
    const live = new Set(threads.flatMap((t) => (t.pending ? [t.pending.id] : [])));
    this.#clearApprovals((a) => !live.has(a.id), "resolved_elsewhere");
    this.#synced = true;
    this.#send(this.#snapshot());
    threads.forEach((t) => this.#trackPending(t));
  }

  #onThread(thread) {
    const previous = this.#tasks.get(thread.id);
    this.#tasks.set(thread.id, thread);
    const changed = previous?.state !== thread.state;
    const alert = changed && ["needs_you", "done", "failed"].includes(thread.state) ? thread.state : null;
    this.#send({ type: "task.update", task: this.#deviceTask(thread), alert, counts: this.#counts() });
    this.#clearApprovals(
      (a) => a.threadId === thread.id && a.id !== thread.pending?.id,
      "resolved_elsewhere",
    );
    this.#trackPending(thread);
    if (alert === "done" && !thread.summary) this.#loadSummary(thread.id);
  }

  async #loadSummary(threadId) {
    const summary = await this.#backend.getSummary(threadId).catch(() => "");
    const task = this.#tasks.get(threadId);
    if (!summary || !task) return;
    task.summary = summary;
    this.#send({ type: "task.update", task: this.#deviceTask(task), alert: null, counts: this.#counts() });
  }

  #clearApprovals(predicate, reason) {
    for (const approval of [...this.#approvals.values()]) {
      if (!predicate(approval)) continue;
      this.#approvals.delete(approval.id);
      if (["open", "expired"].includes(approval.state)) {
        this.#send({ type: "approval.cancel", id: approval.id, reason });
      }
    }
  }

  #trackPending(thread) {
    const pending = thread.pending;
    if (!pending || !APPROVAL_KINDS.has(pending.kind)) return;
    if (this.#approvals.has(pending.id) || this.#loading.has(pending.id)) return;
    this.#loading.add(pending.id);
    this.#loadApproval(thread, pending)
      .catch((error) => this.#log("error", { step: "load approval", detail: error.message }))
      .finally(() => this.#loading.delete(pending.id));
  }

  async #loadApproval(thread, pending) {
    let detail = null;
    for (let attempt = 0; attempt < DETAIL_RETRIES && !detail?.text; attempt += 1) {
      if (attempt) await new Promise((r) => setTimeout(r, this.#retryMs));
      detail = await this.#backend.getApproval(thread.id, pending.id).catch((error) => {
        this.#log("error", { step: "read approval", detail: error.message });
        return null;
      });
    }
    if (this.#tasks.get(thread.id)?.pending?.id !== pending.id) return;

    const text = detail?.text ?? null;
    const kind = detail?.kind ?? pending.kind;
    const { risk, reason } = classifyApproval({ kind, text });
    const approval = {
      id: pending.id,
      threadId: thread.id,
      nonce: randomBytes(8).toString("hex"),
      agent: thread.harness,
      title: thread.title,
      kind,
      risk,
      reason,
      text,
      deviceApprovable: canApproveOnDevice({ risk, text }),
      expires: this.#now() + this.#ttlMs,
      state: "open",
    };
    this.#approvals.set(approval.id, approval);
    this.#log("requested", this.#audit(approval));
    this.#send(this.#requestMessage(approval));
  }

  #requestMessage(a) {
    return {
      type: "approval.request",
      id: a.id, nonce: a.nonce, agent: a.agent, title: a.title, kind: a.kind, risk: a.risk, reason: a.reason,
      // Text that cannot be fully shown never reaches the device, so it can't be half-read.
      text: a.deviceApprovable ? a.text : null,
      deviceApprovable: a.deviceApprovable,
      expires: a.expires,
    };
  }

  async #onDeviceAnswer({ id, nonce, decision }) {
    const approval = this.#approvals.get(id);
    if (!approval || approval.nonce !== nonce) return this.#result(id, "stale");
    if (approval.state !== "open") return this.#result(id, "stale");
    if (this.#now() >= approval.expires) {
      this.expireApprovals();
      return this.#result(id, "expired");
    }
    if (!["approve", "deny"].includes(decision)) return this.#result(id, "invalid");
    if (decision === "approve" && !approval.deviceApprovable) {
      this.#log("rejected", { ...this.#audit(approval), source: "device" });
      return this.#result(id, "needs_phone");
    }
    return this.#answer(approval, decision, "device");
  }

  async #answer(approval, decision, source) {
    const previousState = approval.state;
    approval.state = "answering"; // First answer wins; set before any await.
    try {
      await this.#backend.answerApproval(approval.threadId, approval.id, decision);
      approval.state = "answered";
      this.#log("answered", { ...this.#audit(approval), decision, source });
      return this.#result(approval.id, "accepted", decision);
    } catch (error) {
      approval.state = previousState;
      this.#log("error", { ...this.#audit(approval), step: "answer", detail: error.message });
      return this.#result(approval.id, "failed");
    }
  }

  #result(id, status, decision) {
    const message = { type: "approval.result", id, status, ...(decision ? { decision } : {}) };
    this.#send(message);
    return message;
  }

  async #startTask({ ref, profile: profileId, preset, text }) {
    const fail = (reason) => this.#send({ type: "task.error", ref, reason });
    const profile = this.#profiles.find((p) => p.id === profileId);
    if (!profile) return fail("Unknown profile");
    const chosen = profile.presets.find((p) => p.label === preset);
    const prompt = chosen?.prompt ?? String(text ?? "").trim();
    if (!prompt || prompt.length > MAX_TASK_TEXT) return fail("Task text is empty or too long");
    try {
      const projects = await this.#backend.listProjects();
      const project = projects.find((p) => p.title === profile.project || p.path.endsWith(`/${profile.project}`));
      if (!project) return fail("Project not found");
      const id = await this.#backend.startTask({
        projectId: project.id,
        harness: profile.harness,
        model: profile.model,
        title: chosen?.label ?? prompt.slice(0, 60),
        prompt,
      });
      this.#log("task_started", { threadId: id, profile: profile.id, prompt });
      this.#send({ type: "task.started", ref, id });
    } catch (error) {
      this.#log("error", { step: "start task", detail: error.message });
      fail("Could not start task");
    }
  }

  async #reply({ id, text }) {
    const body = String(text ?? "").trim();
    if (!body || body.length > MAX_TASK_TEXT) {
      return this.#send({ type: "task.error", ref: id, reason: "Reply is empty or too long" });
    }
    return this.#taskAction(id, () => this.#backend.sendMessage(id, body));
  }

  async #taskAction(id, action) {
    if (!this.#tasks.has(id)) return this.#send({ type: "task.error", ref: id, reason: "Unknown task" });
    try {
      await action();
    } catch (error) {
      this.#log("error", { step: "task action", detail: error.message });
      this.#send({ type: "task.error", ref: id, reason: "Action failed" });
    }
  }

  #deviceTask({ id, title, harness, state, summary, updatedAt, pending }) {
    const needsPhone = Boolean(pending && !APPROVAL_KINDS.has(pending.kind));
    return { id, title, agent: harness, state, summary, updatedAt, needsPhone };
  }

  #counts() {
    const counts = { working: 0, needs_you: 0, done: 0, failed: 0, idle: 0 };
    for (const task of this.#tasks.values()) counts[task.state] += 1;
    return counts;
  }

  #snapshot() {
    const tasks = [...this.#tasks.values()]
      .sort((a, b) => String(b.updatedAt).localeCompare(String(a.updatedAt)))
      .slice(0, MAX_TASKS)
      .map((t) => this.#deviceTask(t));
    return { type: "state.snapshot", online: this.#online, synced: this.#synced, tasks, counts: this.#counts() };
  }

  #audit(a) {
    return { id: a.id, threadId: a.threadId, agent: a.agent, risk: a.risk, kind: a.kind, text: a.text };
  }

  #log(event, detail) {
    this.#activity({ at: new Date(this.#now()).toISOString(), event, ...detail });
  }
}
