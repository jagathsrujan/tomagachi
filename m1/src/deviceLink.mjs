// Wi-Fi link to the real board: one JSON message per line over TCP. The device proves it
// holds the pairing token with an HMAC of a fresh nonce, so the token never crosses the
// network. Traffic is not encrypted, so use this on a trusted network only.
import { createHmac, randomBytes, timingSafeEqual } from "node:crypto";
import { createServer } from "node:net";

const MAX_LINE = 4_096;
const AUTH_TIMEOUT_MS = 5_000;
const PING_MS = 5_000;
const SILENCE_MS = 15_000;
const DEVICE_TASKS = 5;

export const authCode = (token, nonce) => createHmac("sha256", token).update(nonce).digest("hex");

const sameCode = (a, b) => {
  const left = Buffer.from(String(a));
  const right = Buffer.from(b);
  return left.length === right.length && timingSafeEqual(left, right);
};

// The board has little memory, so it gets only the newest tasks.
const forDevice = (message) =>
  message.type === "state.snapshot" ? { ...message, tasks: message.tasks.slice(0, DEVICE_TASKS) } : message;

export const createDeviceLink = ({ port, host = "0.0.0.0", token, getBridge, log = () => {} }) => {
  let active = null;

  const write = (socket, message) => socket.write(`${JSON.stringify(forDevice(message))}\n`);

  const onMessage = (session, message) => {
    if (!session.authed) {
      if (message?.type !== "auth" || !sameCode(message.mac, authCode(token, session.nonce))) {
        log("device refused: bad pairing code");
        return session.socket.destroy();
      }
      session.authed = true;
      clearTimeout(session.authTimer);
      active?.socket.destroy(); // The newest paired device wins.
      active = session;
      log(`device connected from ${session.socket.remoteAddress}`);
      write(session.socket, { type: "auth.ok" });
      for (const m of getBridge().snapshotMessages()) write(session.socket, m);
      return;
    }
    if (message?.type === "pong") return;
    Promise.resolve(getBridge().handleDeviceMessage(message)).catch((error) => log(`device message failed: ${error.message}`));
  };

  const server = createServer((socket) => {
    socket.setNoDelay(true);
    const session = { socket, nonce: randomBytes(16).toString("hex"), authed: false, seen: Date.now(), buffer: "" };
    session.authTimer = setTimeout(() => socket.destroy(), AUTH_TIMEOUT_MS);
    session.pingTimer = setInterval(() => {
      if (Date.now() - session.seen > SILENCE_MS) return socket.destroy();
      if (session.authed) write(socket, { type: "ping" });
    }, PING_MS);

    socket.setEncoding("utf8");
    socket.on("data", (chunk) => {
      session.seen = Date.now();
      session.buffer += chunk;
      if (session.buffer.length > MAX_LINE) return socket.destroy();
      let end;
      while ((end = session.buffer.indexOf("\n")) >= 0) {
        const line = session.buffer.slice(0, end).trim();
        session.buffer = session.buffer.slice(end + 1);
        if (!line) continue;
        try {
          onMessage(session, JSON.parse(line));
        } catch {
          return socket.destroy();
        }
      }
    });
    socket.on("error", () => {});
    socket.on("close", () => {
      clearTimeout(session.authTimer);
      clearInterval(session.pingTimer);
      if (active === session) {
        active = null;
        log("device disconnected");
      }
    });
    write(socket, { type: "hello", nonce: session.nonce });
  });

  return {
    send: (message) => { if (active) write(active.socket, message); },
    listen: () => new Promise((resolve, reject) => {
      server.once("error", reject);
      server.listen(port, host, () => resolve(server.address().port));
    }),
    close: () => {
      active?.socket.destroy();
      server.close();
    },
  };
};
