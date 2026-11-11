// Drives the running simulator like a device: tests Stop, and how long real
// approvals stay answerable. Usage: node scripts/soak.mjs [waitMinutes]
import { existsSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const base = "http://127.0.0.1:3918";
const sandbox = join(dirname(fileURLToPath(import.meta.url)), "../sandbox-repo");
const waitMs = Number(process.argv[2] ?? 10) * 60_000;
const started = Date.now();
const log = (m) => console.log(`[${Math.round((Date.now() - started) / 1000)}s] ${m}`);
const headers = { "content-type": "application/json", origin: base };
const post = (path, body) =>
  fetch(base + path, { method: "POST", headers, body: JSON.stringify(body) }).then((r) => r.json());
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const messages = [];
const waiters = new Set();
const events = await fetch(`${base}/events`);
(async () => {
  let buffer = "";
  for await (const chunk of events.body.pipeThrough(new TextDecoderStream())) {
    buffer += chunk;
    const frames = buffer.split("\n\n");
    buffer = frames.pop();
    for (const frame of frames) {
      if (!frame.startsWith("data: ")) continue;
      const message = JSON.parse(frame.slice(6));
      messages.push(message);
      for (const w of waiters) if (w.test(message)) (waiters.delete(w), w.resolve(message));
    }
  }
})();

const waitFor = (test, timeoutMs = 90_000) =>
  new Promise((resolve, reject) => {
    const hit = messages.find(test);
    if (hit) return resolve(hit);
    const waiter = { test, resolve };
    waiters.add(waiter);
    setTimeout(() => {
      waiters.delete(waiter);
      reject(new Error("timed out waiting for message"));
    }, timeoutMs).unref();
  });

const start = async (profile, text) => {
  const ref = crypto.randomUUID();
  await post("/device", { type: "task.start", ref, profile, text });
  const { id } = await waitFor((m) => m.type === "task.started" && m.ref === ref);
  return id;
};

const approvalFor = (title) => waitFor((m) => m.type === "approval.request" && m.title.startsWith(title));

// Stop while a real task is waiting for approval.
const stopTitle = "Stop test B";
const stopId = await start("claude-sandbox", `${stopTitle}. Use your shell tool to run \`touch stopped-task.txt\`, then stop.`);
const stopApproval = await approvalFor(stopTitle);
log(`stop test: approval received, risk=${stopApproval.risk}`);
await post("/device", { type: "task.stop", id: stopId });
const stopped = await waitFor((m) => m.type === "task.update" && m.task.id === stopId && !["working", "needs_you"].includes(m.task.state), 30_000);
await sleep(5_000);
log(`stop test: state=${stopped.task.state} approvalWithdrawn=${messages.some((m) => m.type === "approval.cancel" && m.id === stopApproval.id)} fileExists=${existsSync(join(sandbox, "stopped-task.txt"))}`);

// Leave real approvals unanswered for a long time.
const cases = [
  { profile: "claude-sandbox", file: "lifetime-claude.txt" },
  { profile: "codex-sandbox", file: "lifetime-codex.txt" },
];
for (const c of cases) {
  c.title = `Lifetime ${c.file}`;
  c.id = await start(c.profile, `${c.title}. Use your shell tool to run \`touch ${c.file}\`, then stop.`);
  c.approval = await approvalFor(c.title);
  c.askedAt = Date.now();
  log(`${c.profile}: approval received`);
}
for (const c of cases) {
  await waitFor((m) => m.type === "approval.cancel" && m.id === c.approval.id && m.reason === "expired", 150_000);
  log(`${c.profile}: device timed out; file exists=${existsSync(join(sandbox, c.file))}`);
  const late = await post("/device", { type: "approval.response", id: c.approval.id, nonce: c.approval.nonce, decision: "approve" });
  log(`${c.profile}: late device answer sent ${JSON.stringify(late)}; file exists=${existsSync(join(sandbox, c.file))}`);
}

await sleep(Math.max(0, cases[1].askedAt + waitMs - Date.now()));
for (const c of cases) {
  const { queue } = await fetch(`${base}/phone/state`).then((r) => r.json());
  const entry = queue.find((a) => a.id === c.approval.id);
  log(`${c.profile}: after ${Math.round((Date.now() - c.askedAt) / 60_000)} min, still in queue=${Boolean(entry)} state=${entry?.state}`);
  if (!entry) continue;
  const result = await post("/phone/answer", { id: c.approval.id, decision: "approve", biometricOk: true });
  log(`${c.profile}: phone answer ${JSON.stringify(result)}`);
}
await sleep(15_000);
for (const c of cases) log(`${c.profile}: file exists=${existsSync(join(sandbox, c.file))}`);
process.exit(0);
