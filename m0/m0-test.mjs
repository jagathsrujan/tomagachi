// M0: can a third-party client drive a real agent through T3 Code?
// Usage: node m0-test.mjs <providerInstanceId> <model> <accept|decline>
// Starts a Supervised thread in the sandbox repo, waits for a real approval,
// answers it over T3's RPC, then checks on disk whether the command ran.
import { randomUUID } from "node:crypto";
import { appendFileSync, existsSync, readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const [provider, model, decision] = process.argv.slice(2);
if (!provider || !model || !["accept", "decline"].includes(decision)) {
  console.error("usage: node m0-test.mjs <provider> <model> <accept|decline>");
  process.exit(64);
}

const BASE = "http://127.0.0.1:3917";
const token = readFileSync(join(here, ".m0-token"), "utf8").trim();
const auth = { Authorization: `Bearer ${token}` };
const sandbox = join(here, "sandbox-repo");
const marker = `m0-${provider}-${decision}-${Date.now()}.txt`;
const logFile = join(here, `m0-${provider}-${decision}.jsonl`);
const TIMEOUT_MS = 180_000;
const started = Date.now();
const log = (msg) => console.log(`[${((Date.now() - started) / 1000).toFixed(1)}s] ${msg}`);

const getJson = async (path, init) => {
  const res = await fetch(BASE + path, { ...init, headers: { ...auth, ...init?.headers } });
  if (!res.ok) throw new Error(`${path} -> ${res.status} ${await res.text()}`);
  return res.json();
};

const { projects } = await getJson("/api/projects");
const project = projects.find((p) => p.workspaceRoot === sandbox);
if (!project) throw new Error("sandbox project not found");
const { ticket } = await getJson("/api/auth/websocket-ticket/rpc", { method: "POST" });

const ws = new WebSocket(`ws://127.0.0.1:3917/ws?orchestrationProtocol=2&wsTicket=${encodeURIComponent(ticket)}`);
await new Promise((ok, fail) => {
  ws.addEventListener("open", ok, { once: true });
  ws.addEventListener("error", () => fail(new Error("websocket failed")), { once: true });
});
log("connected with limited token");

let nextId = 1;
const pending = new Map();
const streams = new Map();
const send = (obj) => ws.send(JSON.stringify(obj));
const request = (tag, payload) => {
  const id = String(nextId++);
  send({ _tag: "Request", id, tag, payload, headers: [] });
  return new Promise((resolve, reject) => pending.set(id, { resolve, reject, tag }));
};
const stream = (tag, payload, onChunk) => {
  const id = String(nextId++);
  streams.set(id, onChunk);
  send({ _tag: "Request", id, tag, payload, headers: [] });
};

// Walks a stream item and returns every unanswered approval it mentions.
const seenApprovals = new Map();
const collect = (node) => {
  if (Array.isArray(node)) return node.forEach(collect);
  if (!node || typeof node !== "object") return;
  if (node.type === "approval_request" && node.requestId) seenApprovals.set(node.requestId, node);
  Object.values(node).forEach(collect);
};

let answered = false;
let approvalAt = 0;
let finished;
const done = new Promise((resolve) => (finished = resolve));
let threadId;

const handleItem = async (item) => {
  appendFileSync(logFile, JSON.stringify(item) + "\n");
  collect(item);
  const text = JSON.stringify(item);
  if (/"status":"(completed|failed|interrupted)"/.test(text) && answered) {
    setTimeout(() => finished("turn ended"), 3000);
  }
  if (answered) return;
  for (const [requestId, node] of seenApprovals) {
    answered = true;
    approvalAt = Date.now();
    log(`APPROVAL requestKind=${node.requestKind} prompt=${JSON.stringify(node.prompt ?? "").slice(0, 160)}`);
    const t0 = Date.now();
    const ack = await request("orchestration.dispatchCommand", {
      type: "runtime-request.respond",
      commandId: randomUUID(),
      threadId,
      requestId,
      decision,
    });
    log(`answered ${decision} in ${Date.now() - t0} ms: ${JSON.stringify(ack).slice(0, 160)}`);
    return;
  }
};

ws.addEventListener("message", (event) => {
  const frames = JSON.parse(event.data);
  for (const m of Array.isArray(frames) ? frames : [frames]) {
    if (m._tag === "Ping") send({ _tag: "Pong" });
    else if (m._tag === "Chunk") {
      send({ _tag: "Ack", requestId: m.requestId });
      const onChunk = streams.get(m.requestId);
      if (onChunk) for (const v of m.values) onChunk(v);
    } else if (m._tag === "Exit") {
      const p = pending.get(m.requestId);
      if (p) {
        pending.delete(m.requestId);
        m.exit?._tag === "Success"
          ? p.resolve(m.exit.value)
          : p.reject(new Error(`${p.tag}: ${JSON.stringify(m.exit).slice(0, 500)}`));
      } else if (streams.has(m.requestId)) log(`stream ended: ${JSON.stringify(m.exit).slice(0, 300)}`);
    } else if (m._tag !== "Pong") log(`other frame: ${JSON.stringify(m).slice(0, 300)}`);
  }
});
setInterval(() => send({ _tag: "Ping" }), 5000).unref();

const launched = await request("orchestration.launchThread", {
  commandId: randomUUID(),
  creationSource: "mobile",
  projectId: project.id,
  title: `M0 ${provider} ${decision}`,
  modelSelection: { instanceId: provider, model },
  runtimeMode: "approval-required",
  interactionMode: "default",
  workspaceStrategy: { type: "root" },
  initialMessage: {
    text: `Use your shell tool to run exactly this command, then stop: touch ${marker}\nDo not create the file any other way. If the command is declined, do not retry.`,
    attachments: [],
  },
});
threadId = launched.threadId;
log(`launched thread ${threadId} in approval-required mode`);
stream("orchestration.subscribeThread", { threadId }, (item) => {
  handleItem(item).catch((e) => log(`handler error: ${e.message}`));
});

const outcome = await Promise.race([
  done,
  new Promise((r) => setTimeout(() => r("timeout"), TIMEOUT_MS)),
]);
if (decision === "decline") await new Promise((r) => setTimeout(r, 25_000));
const ran = existsSync(join(sandbox, marker));
const pass = answered && (decision === "accept" ? ran : !ran);
log(`end=${outcome} approvalSeen=${answered} fileExists=${ran}`);
console.log(`${pass ? "PASS" : "FAIL"} provider=${provider} decision=${decision} approvalAfter=${approvalAt ? ((approvalAt - started) / 1000).toFixed(1) + "s" : "none"}`);
ws.close();
process.exit(pass ? 0 : 1);
