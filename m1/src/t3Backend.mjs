// AgentBackend over T3 Code's WebSocket RPC and HTTP API. Uses only what M0 proved.
import { randomUUID } from "node:crypto";

const PING_MS = 5_000;
const PONG_TIMEOUT_MS = 15_000;
const MAX_BACKOFF_MS = 8_000;
const WORKING = new Set(["preparing", "queued", "starting", "running", "waiting"]);
const FAILED = new Set(["failed", "interrupted", "cancelled", "rolled_back"]);

export const stateOf = (shell) => {
  if (shell.pendingRuntimeRequest) return "needs_you";
  if (WORKING.has(shell.status)) return "working";
  if (FAILED.has(shell.status)) return "failed";
  if (shell.status === "completed") return "done";
  return "idle";
};

const oneLine = (text, max = 160) => {
  const flat = String(text ?? "").replace(/\s+/g, " ").trim();
  return flat.length > max ? `${flat.slice(0, max - 1)}…` : flat;
};

export const normalizeThread = (shell) => {
  const pending = shell.pendingRuntimeRequest;
  const state = stateOf(shell);
  const message = shell.latestVisibleMessage;
  return {
    id: shell.id,
    title: shell.title,
    harness: shell.providerInstanceId,
    projectId: shell.projectId,
    state,
    summary: oneLine(state === "failed" && shell.lastError ? shell.lastError : message?.text),
    updatedAt: message?.updatedAt ?? shell.latestRunCompletedAt ?? shell.latestRunStartedAt ?? "",
    pending: pending
      ? { id: pending.id, kind: pending.kind, blocking: pending.blocking !== false }
      : null,
  };
};

// T3 follows the real snapshot with a project-only one whose thread list is empty.
export const isAuthoritativeSnapshot = (item) => item.resolvedRepositoryIdentityRoots === undefined;

const isTopLevel = (shell) => !shell.lineage?.parentThreadId && !shell.archivedAt;

const findApprovalItem = (node, requestId) => {
  if (Array.isArray(node)) {
    for (const child of node) {
      const hit = findApprovalItem(child, requestId);
      if (hit) return hit;
    }
    return null;
  }
  if (!node || typeof node !== "object") return null;
  if (node.type === "approval_request" && node.requestId === requestId) return node;
  return findApprovalItem(Object.values(node), requestId);
};

export class T3Backend {
  #baseUrl;
  #token;
  #listeners = new Set();
  #ws = null;
  #nextId = 1;
  #pending = new Map();
  #closed = false;
  #backoff = 500;
  #pingTimer = null;
  #lastPong = 0;
  #online = false;
  #active = new Set();

  constructor({ baseUrl, token }) {
    this.#baseUrl = baseUrl.replace(/\/$/, "");
    this.#token = token;
  }

  subscribe(onEvent) {
    this.#listeners.add(onEvent);
    return () => this.#listeners.delete(onEvent);
  }

  async connect() {
    this.#closed = false;
    await this.#open();
  }

  close() {
    this.#closed = true;
    clearInterval(this.#pingTimer);
    this.#ws?.close();
  }

  async listProjects() {
    const { projects } = await this.#http("/api/projects");
    return projects
      .filter((p) => !p.archivedAt && !p.deletedAt)
      .map((p) => ({ id: p.id, title: p.title, path: p.workspaceRoot }));
  }

  async startTask({ projectId, harness, model, title, prompt }) {
    const launched = await this.#request("orchestration.launchThread", {
      commandId: randomUUID(),
      creationSource: "mobile",
      projectId,
      title,
      modelSelection: { instanceId: harness, model },
      // T3 defaults to full access. This product never launches without approvals.
      runtimeMode: "approval-required",
      interactionMode: "default",
      workspaceStrategy: { type: "root" },
      initialMessage: { text: prompt, attachments: [] },
    });
    return launched.threadId;
  }

  async sendMessage(threadId, text) {
    await this.#dispatch({
      type: "message.dispatch",
      createdBy: "user",
      creationSource: "mobile",
      threadId,
      messageId: randomUUID(),
      text,
      attachments: [],
      dispatchMode: { type: this.#active.has(threadId) ? "queue_after_active" : "start_immediately" },
    });
  }

  async getSummary(threadId) {
    const { messages = [] } = await this.#request("orchestration.getThreadProjection", { threadId });
    const reply = messages.filter((m) => m.role === "assistant" && m.text).at(-1);
    return oneLine(reply?.text);
  }

  async stopTask(threadId) {
    await this.#dispatch({ type: "thread.stop", threadId, reason: "Stopped from device" });
  }

  async getApproval(threadId, requestId) {
    const projection = await this.#request("orchestration.getThreadProjection", { threadId });
    const item = findApprovalItem(projection, requestId);
    if (!item) return null;
    return {
      id: requestId,
      kind: item.requestKind,
      text: item.prompt ?? null,
      reason: item.reason ?? null,
    };
  }

  async answerApproval(threadId, requestId, decision) {
    await this.#dispatch({
      type: "runtime-request.respond",
      threadId,
      requestId,
      // One-shot only: never "accept always" from a remote control.
      decision: decision === "approve" ? "accept" : "decline",
    });
  }

  #emit(event) {
    for (const listener of this.#listeners) listener(event);
  }

  #setOnline(online, detail) {
    if (this.#online === online) return;
    this.#online = online;
    this.#emit({ type: "connection", online, detail });
  }

  async #http(path, init) {
    const res = await fetch(this.#baseUrl + path, {
      ...init,
      headers: { Authorization: `Bearer ${this.#token}`, ...init?.headers },
    });
    if (!res.ok) throw new Error(`T3 ${path} returned ${res.status}`);
    return res.json();
  }

  async #open() {
    const { ticket } = await this.#http("/api/auth/websocket-ticket/rpc", { method: "POST" });
    const wsBase = this.#baseUrl.replace(/^http/, "ws");
    const ws = new WebSocket(
      `${wsBase}/ws?orchestrationProtocol=2&wsTicket=${encodeURIComponent(ticket)}`,
    );
    await new Promise((resolve, reject) => {
      ws.addEventListener("open", resolve, { once: true });
      ws.addEventListener("error", () => reject(new Error("T3 WebSocket failed")), { once: true });
    });
    this.#ws = ws;
    this.#lastPong = Date.now();
    this.#backoff = 500;
    ws.addEventListener("message", (e) => this.#onMessage(e.data));
    ws.addEventListener("close", () => this.#onClose(ws));
    clearInterval(this.#pingTimer);
    this.#pingTimer = setInterval(() => this.#ping(ws), PING_MS);
    this.#pingTimer.unref?.();
    this.#send({
      _tag: "Request",
      id: "shell",
      tag: "orchestration.subscribeShell",
      payload: {},
      headers: [],
    });
    this.#setOnline(true);
  }

  #ping(ws) {
    if (Date.now() - this.#lastPong > PONG_TIMEOUT_MS) return ws.close();
    this.#send({ _tag: "Ping" });
  }

  #onClose(ws) {
    if (ws !== this.#ws) return;
    clearInterval(this.#pingTimer);
    for (const { reject } of this.#pending.values()) reject(new Error("T3 connection lost"));
    this.#pending.clear();
    this.#setOnline(false, "T3 connection lost");
    if (!this.#closed) this.#retry();
  }

  #retry() {
    const delay = this.#backoff;
    this.#backoff = Math.min(this.#backoff * 2, MAX_BACKOFF_MS);
    const timer = setTimeout(() => {
      this.#open().catch(() => {
        if (!this.#closed) this.#retry();
      });
    }, delay);
    timer.unref?.();
  }

  #send(frame) {
    if (this.#ws?.readyState !== WebSocket.OPEN) throw new Error("T3 is offline");
    this.#ws.send(JSON.stringify(frame));
  }

  #request(tag, payload) {
    const id = String(this.#nextId++);
    return new Promise((resolve, reject) => {
      this.#pending.set(id, { resolve, reject, tag });
      try {
        this.#send({ _tag: "Request", id, tag, payload, headers: [] });
      } catch (error) {
        this.#pending.delete(id);
        reject(error);
      }
    });
  }

  #dispatch(command) {
    return this.#request("orchestration.dispatchCommand", { commandId: randomUUID(), ...command });
  }

  #onMessage(data) {
    const frames = JSON.parse(data);
    for (const frame of Array.isArray(frames) ? frames : [frames]) {
      if (frame._tag === "Ping") this.#send({ _tag: "Pong" });
      else if (frame._tag === "Pong") this.#lastPong = Date.now();
      else if (frame._tag === "Chunk") {
        this.#send({ _tag: "Ack", requestId: frame.requestId });
        if (frame.requestId === "shell") frame.values.forEach((v) => this.#onShellItem(v));
      } else if (frame._tag === "Exit") this.#onExit(frame);
    }
  }

  #onExit(frame) {
    const call = this.#pending.get(frame.requestId);
    if (!call) {
      // The shell stream ending means our view is stale, so force a reconnect.
      if (frame.requestId === "shell" && !this.#closed) this.#ws?.close();
      return;
    }
    this.#pending.delete(frame.requestId);
    if (frame.exit?._tag === "Success") call.resolve(frame.exit.value);
    else call.reject(new Error(`${call.tag} failed: ${JSON.stringify(frame.exit).slice(0, 300)}`));
  }

  #onShellItem(item) {
    if (item.kind === "snapshot") {
      if (!isAuthoritativeSnapshot(item)) return;
      this.#active = new Set(item.snapshot.threads.filter((t) => t.activeRunId).map((t) => t.id));
      const threads = item.snapshot.threads.filter(isTopLevel).map(normalizeThread);
      this.#emit({ type: "snapshot", threads });
    } else if (item.kind === "thread.updated") {
      if (item.thread.activeRunId) this.#active.add(item.thread.id);
      else this.#active.delete(item.thread.id);
      if (item.location === "archive" || !isTopLevel(item.thread)) {
        this.#emit({ type: "thread-removed", threadId: item.thread.id });
      } else this.#emit({ type: "thread", thread: normalizeThread(item.thread) });
    } else if (item.kind === "thread.removed") {
      this.#emit({ type: "thread-removed", threadId: item.threadId });
    }
  }
}
