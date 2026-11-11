// AgentBackend that runs Claude Code directly through the Claude Agent SDK, with no
// T3 in between. Each thread is a real Claude Code session using the laptop's login.
import { randomUUID } from "node:crypto";
import { isAbsolute, relative, resolve } from "node:path";
import { query as sdkQuery } from "@anthropic-ai/claude-agent-sdk";

const FILE_READ = new Set(["Read"]);
const FILE_CHANGE = new Set(["Write", "Edit", "MultiEdit", "NotebookEdit"]);
const HOLD_MS = 10 * 60_000;

const oneLine = (text, max = 160) => {
  const flat = String(text ?? "").replace(/\s+/g, " ").trim();
  return flat.length > max ? `${flat.slice(0, max - 1)}…` : flat;
};

// Paths inside the project become relative. Anything else stays absolute, so the risk
// rules see it as outside the project.
const projectPath = (path, cwd) => {
  const full = resolve(cwd, String(path ?? ""));
  const inside = relative(cwd, full);
  return inside.startsWith("..") || isAbsolute(inside) ? full : inside;
};

/** Turns a Claude Code tool call into the kind and exact text the risk rules expect. */
export const describeTool = (toolName, input, cwd) => {
  if (toolName === "Bash") return { kind: "command", text: String(input?.command ?? "") };
  const path = input?.file_path ?? input?.notebook_path;
  if (path && FILE_READ.has(toolName)) return { kind: "file-read", text: projectPath(path, cwd) };
  if (path && FILE_CHANGE.has(toolName)) return { kind: "file-change", text: projectPath(path, cwd) };
  return { kind: "permission", text: `${toolName}: ${JSON.stringify(input ?? {})}` };
};

const textOf = (message) =>
  (message?.message?.content ?? [])
    .filter((block) => block.type === "text")
    .map((block) => block.text)
    .join(" ");

export class ClaudeBackend {
  #projects;
  #query;
  #holdMs;
  #listeners = new Set();
  #threads = new Map();

  constructor({ projects, query = sdkQuery, holdMs = HOLD_MS }) {
    this.#projects = projects;
    this.#query = query;
    this.#holdMs = holdMs;
  }

  subscribe(onEvent) {
    this.#listeners.add(onEvent);
    return () => this.#listeners.delete(onEvent);
  }

  async connect() {
    this.#emit({ type: "connection", online: true });
    this.#emit({ type: "snapshot", threads: [...this.#threads.values()].map((t) => this.#view(t)) });
  }

  close() {
    for (const thread of this.#threads.values()) this.#denyAll(thread, "The device link closed.");
    for (const thread of this.#threads.values()) thread.query?.close?.();
  }

  async listProjects() {
    return this.#projects;
  }

  async startTask({ projectId, harness, model, title, prompt }) {
    const project = this.#projects.find((p) => p.id === projectId);
    if (!project) throw new Error("Unknown project");
    const thread = {
      id: randomUUID(), title, harness, model, projectId, cwd: project.path,
      state: "idle", summary: "", updatedAt: "", sessionId: null,
      running: false, stopped: false, query: null, requests: new Map(),
    };
    this.#threads.set(thread.id, thread);
    this.#runTurn(thread, prompt);
    return thread.id;
  }

  async sendMessage(threadId, text) {
    const thread = this.#thread(threadId);
    if (thread.running) throw new Error("Session is busy");
    this.#runTurn(thread, text);
  }

  async stopTask(threadId) {
    const thread = this.#thread(threadId);
    if (!thread.running) return;
    thread.stopped = true;
    this.#denyAll(thread, "Stopped from the device.");
    await thread.query?.interrupt();
  }

  async getSummary(threadId) {
    return this.#thread(threadId).summary;
  }

  async getApproval(threadId, requestId) {
    return this.#thread(threadId).requests.get(requestId)?.detail ?? null;
  }

  async answerApproval(threadId, requestId, decision) {
    const request = this.#thread(threadId).requests.get(requestId);
    if (!request) throw new Error("Request is no longer open");
    request.settle(
      decision === "approve"
        ? { behavior: "allow", updatedInput: request.input }
        : { behavior: "deny", message: "The user denied this from the device." },
    );
  }

  #thread(id) {
    const thread = this.#threads.get(id);
    if (!thread) throw new Error("Unknown thread");
    return thread;
  }

  async #runTurn(thread, prompt) {
    thread.running = true;
    thread.stopped = false;
    thread.state = "working";
    this.#publish(thread);

    // canUseTool needs streaming input, and stdin must stay open until the turn ends.
    let finish;
    const finished = new Promise((r) => { finish = r; });
    const input = async function* () {
      yield { type: "user", message: { role: "user", content: prompt }, parent_tool_use_id: null };
      await finished;
    };

    try {
      thread.query = this.#query({
        prompt: input(),
        options: {
          cwd: thread.cwd,
          model: thread.model,
          // Personal allow rules would skip the device, so user settings are not loaded.
          permissionMode: "default",
          settingSources: [],
          canUseTool: (name, toolInput, options) => this.#ask(thread, name, toolInput, options),
          ...(thread.sessionId ? { resume: thread.sessionId } : {}),
        },
      });
      for await (const message of thread.query) {
        if (message.session_id) thread.sessionId = message.session_id;
        if (message.type === "assistant") {
          const text = oneLine(textOf(message));
          if (text) { thread.summary = text; this.#publish(thread); }
        }
        if (message.type === "result") {
          this.#onResult(thread, message);
          finish();
        }
      }
    } catch (error) {
      if (thread.running) this.#end(thread, "failed", thread.stopped ? "Stopped" : oneLine(error.message));
    } finally {
      finish();
      thread.query = null;
      if (thread.running) this.#end(thread, "failed", "Session ended without a result");
    }
  }

  #onResult(thread, result) {
    if (thread.stopped) return this.#end(thread, "failed", "Stopped");
    const ok = result.subtype === "success" && !result.is_error;
    if (ok) return this.#end(thread, "done", oneLine(result.result) || thread.summary);
    this.#end(thread, "failed", oneLine(result.result ?? result.errors?.join(" ")) || "Session failed");
  }

  #end(thread, state, summary) {
    this.#denyAll(thread, "The session ended.");
    thread.running = false;
    thread.state = state;
    thread.summary = summary;
    this.#publish(thread);
  }

  #ask(thread, toolName, input, { signal } = {}) {
    const id = randomUUID();
    const detail = { id, ...describeTool(toolName, input, thread.cwd), reason: null };
    return new Promise((done) => {
      const onAbort = () => request.settle({ behavior: "deny", message: "Request was cancelled." });
      const request = {
        detail,
        input,
        settle: (result) => {
          clearTimeout(request.timer);
          signal?.removeEventListener("abort", onAbort);
          thread.requests.delete(id);
          if (thread.running) this.#publish(thread);
          done(result);
        },
      };
      // Silence is never consent, and a session must not stay blocked forever.
      request.timer = setTimeout(
        () => request.settle({ behavior: "deny", message: "Nobody answered in time." }),
        this.#holdMs,
      );
      request.timer.unref?.();
      signal?.addEventListener("abort", onAbort, { once: true });
      thread.requests.set(id, request);
      this.#publish(thread);
    });
  }

  #denyAll(thread, message) {
    for (const request of [...thread.requests.values()]) request.settle({ behavior: "deny", message });
  }

  #publish(thread) {
    thread.updatedAt = new Date().toISOString();
    this.#emit({ type: "thread", thread: this.#view(thread) });
  }

  // A copy, because the bridge compares each update with the one before it.
  #view(thread) {
    const first = thread.requests.values().next().value;
    const pending = first ? { id: first.detail.id, kind: first.detail.kind, blocking: true } : null;
    return {
      id: thread.id, title: thread.title, harness: thread.harness, projectId: thread.projectId,
      state: pending ? "needs_you" : thread.state,
      summary: thread.summary, updatedAt: thread.updatedAt, pending,
    };
  }

  #emit(event) {
    for (const listener of this.#listeners) listener(event);
  }
}
