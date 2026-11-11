import assert from "node:assert/strict";
import { test } from "node:test";
import { assertBackend } from "../src/backend.mjs";
import { DeviceBridge } from "../src/bridge.mjs";

class FakeBackend {
  listeners = new Set();
  answers = [];
  started = [];
  details = new Map();
  failAnswer = false;
  connect = async () => {};
  close = () => {};
  subscribe = (fn) => (this.listeners.add(fn), () => this.listeners.delete(fn));
  listProjects = async () => [{ id: "p1", title: "sandbox", path: "/x/sandbox-repo" }];
  startTask = async (t) => (this.started.push(t), "t-new");
  sendMessage = async () => {};
  stopTask = async () => {};
  getSummary = async () => "All done";
  getApproval = async (_t, id) => this.details.get(id) ?? null;
  answerApproval = async (threadId, id, decision) => {
    if (this.failAnswer) throw new Error("T3 refused");
    this.answers.push({ threadId, id, decision });
  };
  emit = (e) => this.listeners.forEach((fn) => fn(e));
}

const thread = (over = {}) => ({
  id: "t1", title: "Task", harness: "codex", projectId: "p1", state: "needs_you",
  summary: "", updatedAt: "2026-11-05", pending: { id: "r1", kind: "command", blocking: true }, ...over,
});

const setup = async (text = "touch a.txt", options = {}) => {
  const backend = assertBackend(new FakeBackend());
  backend.details.set("r1", { id: "r1", kind: "command", text, reason: null });
  const sent = [];
  const activity = [];
  let clock = 1_000;
  const bridge = new DeviceBridge({
    backend, send: (m) => sent.push(m), onActivity: (a) => activity.push(a),
    now: () => clock, ttlMs: 100, retryMs: 0,
    profiles: [{
      id: "c", name: "Codex", icon: "X", harness: "codex", model: "m", project: "sandbox-repo",
      presets: [{ label: "List", prompt: "run ls" }],
    }],
    ...options,
  });
  bridge.start();
  backend.emit({ type: "connection", online: true });
  backend.emit({ type: "snapshot", threads: [thread()] });
  await new Promise((r) => setImmediate(r));
  await new Promise((r) => setImmediate(r));
  const request = sent.find((m) => m.type === "approval.request");
  return { backend, bridge, sent, activity, request, tick: (ms) => (clock += ms) };
};

const results = (sent) => sent.filter((m) => m.type === "approval.result");

test("device approves a medium-risk command", async () => {
  const { backend, bridge, request, sent, activity } = await setup();
  assert.equal(request.risk, "medium");
  assert.equal(request.deviceApprovable, true);
  await bridge.handleDeviceMessage({ type: "approval.response", id: "r1", nonce: request.nonce, decision: "approve" });
  assert.deepEqual(backend.answers, [{ threadId: "t1", id: "r1", decision: "approve" }]);
  assert.equal(results(sent).at(-1).status, "accepted");
  assert.ok(activity.some((a) => a.event === "answered" && a.source === "device"));
});

test("device denies", async () => {
  const { backend, bridge, request } = await setup();
  await bridge.handleDeviceMessage({ type: "approval.response", id: "r1", nonce: request.nonce, decision: "deny" });
  assert.equal(backend.answers[0].decision, "deny");
});

test("wrong nonce is rejected", async () => {
  const { backend, bridge, sent } = await setup();
  await bridge.handleDeviceMessage({ type: "approval.response", id: "r1", nonce: "bad", decision: "approve" });
  assert.equal(backend.answers.length, 0);
  assert.equal(results(sent).at(-1).status, "stale");
});

test("first answer wins, even when two arrive together", async () => {
  const { backend, bridge, request, sent } = await setup();
  const msg = { type: "approval.response", id: "r1", nonce: request.nonce };
  await Promise.all([
    bridge.handleDeviceMessage({ ...msg, decision: "approve" }),
    bridge.handleDeviceMessage({ ...msg, decision: "deny" }),
  ]);
  assert.equal(backend.answers.length, 1);
  assert.equal(backend.answers[0].decision, "approve");
  assert.ok(results(sent).some((m) => m.status === "stale"));
});

test("high-risk approval from a device is refused, even with a valid nonce", async () => {
  const { backend, bridge, request, sent } = await setup("rm -rf build");
  assert.equal(request.risk, "high");
  assert.equal(request.deviceApprovable, false);
  await bridge.handleDeviceMessage({ type: "approval.response", id: "r1", nonce: request.nonce, decision: "approve" });
  assert.equal(backend.answers.length, 0);
  assert.equal(results(sent).at(-1).status, "needs_phone");
});

test("high-risk can still be denied from the device", async () => {
  const { backend, bridge, request } = await setup("rm -rf build");
  await bridge.handleDeviceMessage({ type: "approval.response", id: "r1", nonce: request.nonce, decision: "deny" });
  assert.equal(backend.answers[0].decision, "deny");
});

test("text too long for the screen never reaches the device", async () => {
  const { backend, bridge, request } = await setup(`touch ${"a".repeat(200)}`);
  assert.equal(request.text, null);
  assert.equal(request.deviceApprovable, false);
  await bridge.handleDeviceMessage({ type: "approval.response", id: "r1", nonce: request.nonce, decision: "approve" });
  assert.equal(backend.answers.length, 0);
});

test("high-risk from the phone needs the biometric check", async () => {
  const { backend, bridge } = await setup("rm -rf build");
  const refused = await bridge.answerFromPhone({ id: "r1", decision: "approve", biometricOk: false });
  assert.equal(refused.status, "biometric_required");
  assert.equal(backend.answers.length, 0);
  await bridge.answerFromPhone({ id: "r1", decision: "approve", biometricOk: true });
  assert.equal(backend.answers[0].decision, "approve");
});

test("no answer never approves: it expires and stays answerable on the phone", async () => {
  const { backend, bridge, request, sent, tick } = await setup();
  tick(100);
  bridge.expireApprovals();
  assert.equal(backend.answers.length, 0);
  assert.ok(sent.some((m) => m.type === "approval.cancel" && m.reason === "expired"));
  await bridge.handleDeviceMessage({ type: "approval.response", id: "r1", nonce: request.nonce, decision: "approve" });
  assert.equal(backend.answers.length, 0);
  assert.equal(bridge.phoneQueue()[0].state, "expired");
  await bridge.answerFromPhone({ id: "r1", decision: "deny" });
  assert.equal(backend.answers[0].decision, "deny");
});

test("late answer after expiry but before the timer is refused", async () => {
  const { backend, bridge, request, sent, tick } = await setup();
  tick(100);
  await bridge.handleDeviceMessage({ type: "approval.response", id: "r1", nonce: request.nonce, decision: "approve" });
  assert.equal(backend.answers.length, 0);
  assert.equal(results(sent).at(-1).status, "expired");
});

test("device only sees success after T3 accepts", async () => {
  const { backend, bridge, request, sent } = await setup();
  backend.failAnswer = true;
  await bridge.handleDeviceMessage({ type: "approval.response", id: "r1", nonce: request.nonce, decision: "approve" });
  assert.equal(results(sent).at(-1).status, "failed");
  backend.failAnswer = false;
  await bridge.handleDeviceMessage({ type: "approval.response", id: "r1", nonce: request.nonce, decision: "approve" });
  assert.equal(results(sent).at(-1).status, "accepted");
});

test("answered elsewhere withdraws the request from the device", async () => {
  const { backend, sent } = await setup();
  backend.emit({ type: "thread", thread: thread({ state: "working", pending: null }) });
  assert.ok(sent.some((m) => m.type === "approval.cancel" && m.reason === "resolved_elsewhere"));
});

test("going offline withdraws approvals, and a reconnect issues a fresh nonce", async () => {
  const { backend, bridge, request, sent } = await setup();
  backend.emit({ type: "connection", online: false });
  assert.ok(sent.some((m) => m.type === "approval.cancel" && m.reason === "offline"));
  await bridge.handleDeviceMessage({ type: "approval.response", id: "r1", nonce: request.nonce, decision: "approve" });
  assert.equal(backend.answers.length, 0);
  backend.emit({ type: "connection", online: true });
  backend.emit({ type: "snapshot", threads: [thread()] });
  await new Promise((r) => setImmediate(r));
  await new Promise((r) => setImmediate(r));
  const fresh = sent.filter((m) => m.type === "approval.request").at(-1);
  assert.notEqual(fresh.nonce, request.nonce);
});

test("if details cannot be read it is high risk and phone only", async () => {
  const { backend, bridge, sent } = await setup("placeholder");
  backend.details.delete("r2");
  backend.emit({ type: "thread", thread: thread({ id: "t2", pending: { id: "r2", kind: "command", blocking: true } }) });
  await new Promise((r) => setTimeout(r, 10));
  const request = sent.filter((m) => m.type === "approval.request").at(-1);
  assert.equal(request.id, "r2");
  assert.equal(request.risk, "high");
  assert.equal(bridge.phoneQueue().find((a) => a.id === "r2").text, null);
});

test("alerts fire on state changes, not on the first snapshot", async () => {
  const { backend, sent } = await setup();
  assert.equal(sent.some((m) => m.type === "task.update"), false);
  backend.emit({ type: "thread", thread: thread({ state: "failed", pending: null, summary: "bad model" }) });
  const update = sent.find((m) => m.type === "task.update");
  assert.equal(update.alert, "failed");
  assert.equal(update.task.summary, "bad model");
});

test("a finished task with no summary fetches the agent's reply", async () => {
  const { backend, sent } = await setup();
  backend.emit({ type: "thread", thread: thread({ state: "done", pending: null, summary: "" }) });
  await new Promise((r) => setImmediate(r));
  assert.equal(sent.filter((m) => m.type === "task.update").at(-1).task.summary, "All done");
});

test("starting a task uses the profile and rejects bad input", async () => {
  const { backend, bridge, sent } = await setup();
  await bridge.handleDeviceMessage({ type: "task.start", ref: "1", profile: "c", preset: "List" });
  assert.equal(backend.started[0].prompt, "run ls");
  assert.ok(sent.some((m) => m.type === "task.started" && m.ref === "1"));
  await bridge.handleDeviceMessage({ type: "task.start", ref: "2", profile: "nope", text: "x" });
  await bridge.handleDeviceMessage({ type: "task.start", ref: "3", profile: "c", text: " " });
  assert.equal(sent.filter((m) => m.type === "task.error").length, 2);
  assert.equal(backend.started.length, 1);
});

test("unknown device messages are ignored", async () => {
  const { backend, bridge, activity } = await setup();
  await bridge.handleDeviceMessage({ type: "shell.run", command: "rm -rf /" });
  await bridge.handleDeviceMessage(null);
  assert.equal(backend.answers.length, 0);
  assert.ok(activity.some((a) => a.event === "rejected"));
});
