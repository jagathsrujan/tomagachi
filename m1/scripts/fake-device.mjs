// Stands in for the board: pairs with scripts/link.mjs, starts a preset and answers
// the approval the way a held button would. Usage: node scripts/fake-device.mjs [preset] [approve|deny]
import { readFileSync } from "node:fs";
import { connect } from "node:net";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { authCode } from "../src/deviceLink.mjs";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const token = readFileSync(join(root, "data/device-token"), "utf8").trim();
const preset = process.argv[2] ?? "Create note";
const decision = process.argv[3] ?? "approve";
const send = (m) => socket.write(`${JSON.stringify(m)}\n`);

const socket = connect(Number(process.env.LINK_PORT ?? 3919), process.env.LINK_HOST ?? "127.0.0.1");
let buffer = "";
let taskId = null;
const timeout = setTimeout(() => { console.log("TIMEOUT"); process.exit(1); }, 120_000);

socket.setEncoding("utf8");
socket.on("data", (chunk) => {
  buffer += chunk;
  let end;
  while ((end = buffer.indexOf("\n")) >= 0) {
    const message = JSON.parse(buffer.slice(0, end));
    buffer = buffer.slice(end + 1);
    if (message.type === "ping") { send({ type: "pong" }); continue; }
    console.log(JSON.stringify(message));
    if (message.type === "hello") send({ type: "auth", mac: authCode(token, message.nonce) });
    if (message.type === "profiles.sync") {
      send({ type: "task.start", ref: "fake-1", profile: message.profiles[0].id, preset });
    }
    if (message.type === "task.started") taskId = message.id;
    if (message.type === "approval.request") {
      send({ type: "approval.response", id: message.id, nonce: message.nonce, decision });
    }
    if (message.type === "task.update" && message.task.id === taskId && ["done", "failed"].includes(message.task.state)) {
      clearTimeout(timeout);
      socket.end();
    }
  }
});
