import assert from "node:assert/strict";
import { connect } from "node:net";
import { test } from "node:test";
import { authCode, createDeviceLink } from "../src/deviceLink.mjs";

const token = "test-token";

const setup = async () => {
  const received = [];
  const bridge = {
    snapshotMessages: () => [{ type: "state.snapshot", tasks: Array(9).fill({ id: "t" }), counts: {} }],
    handleDeviceMessage: async (m) => received.push(m),
  };
  const link = createDeviceLink({ port: 0, host: "127.0.0.1", token, getBridge: () => bridge });
  const port = await link.listen();
  return { link, port, received };
};

const client = (port) => {
  const socket = connect(port, "127.0.0.1");
  const messages = [];
  let waiters = [];
  let buffer = "";
  let closed = false;
  const flush = () => { waiters.forEach((w) => w()); waiters = []; };
  socket.setEncoding("utf8");
  socket.on("data", (chunk) => {
    buffer += chunk;
    const lines = buffer.split("\n");
    buffer = lines.pop();
    lines.forEach((l) => messages.push(JSON.parse(l)));
    flush();
  });
  socket.on("error", () => {});
  socket.on("close", () => { closed = true; flush(); });
  const next = async (type) => {
    for (;;) {
      const hit = messages.find((m) => m.type === type);
      if (hit) return hit;
      if (closed) return null;
      await new Promise((r) => waiters.push(r));
    }
  };
  const untilClosed = async () => { while (!closed) await new Promise((r) => waiters.push(r)); };
  return { socket, next, untilClosed, send: (m) => socket.write(`${typeof m === "string" ? m : JSON.stringify(m)}\n`) };
};

const pair = async (port) => {
  const device = client(port);
  const { nonce } = await device.next("hello");
  device.send({ type: "auth", mac: authCode(token, nonce) });
  await device.next("auth.ok");
  return device;
};

test("a paired device gets a trimmed snapshot and can send messages", async () => {
  const { link, port, received } = await setup();
  const device = await pair(port);
  assert.equal((await device.next("state.snapshot")).tasks.length, 5);
  device.send({ type: "task.start", ref: "1" });
  await new Promise((r) => setTimeout(r, 20));
  assert.deepEqual(received, [{ type: "task.start", ref: "1" }]);
  link.send({ type: "task.update" });
  assert.ok(await device.next("task.update"));
  device.socket.destroy();
  link.close();
});

test("a wrong pairing code is dropped and sends nothing to the bridge", async () => {
  const { link, port, received } = await setup();
  const device = client(port);
  await device.next("hello");
  device.send({ type: "auth", mac: authCode("wrong", "nonce") });
  await device.untilClosed();
  assert.equal(received.length, 0);
  link.close();
});

test("messages before pairing are refused", async () => {
  const { link, port, received } = await setup();
  const device = client(port);
  await device.next("hello");
  device.send({ type: "approval.response", id: "a", nonce: "b", decision: "approve" });
  await device.untilClosed();
  assert.equal(received.length, 0);
  link.close();
});

test("a code from an old connection cannot be replayed", async () => {
  const { link, port } = await setup();
  const first = client(port);
  const { nonce } = await first.next("hello");
  first.socket.destroy();
  const second = client(port);
  await second.next("hello");
  second.send({ type: "auth", mac: authCode(token, nonce) });
  await second.untilClosed();
  link.close();
});

test("bad JSON and oversized lines close the connection", async () => {
  const { link, port } = await setup();
  const bad = await pair(port);
  bad.send("not json");
  await bad.untilClosed();
  const big = await pair(port);
  big.socket.write("x".repeat(5_000));
  await big.untilClosed();
  link.close();
});

test("the newest paired device replaces the old one", async () => {
  const { link, port } = await setup();
  const first = await pair(port);
  const second = await pair(port);
  await first.untilClosed();
  link.send({ type: "task.update" });
  assert.ok(await second.next("task.update"));
  second.socket.destroy();
  link.close();
});
