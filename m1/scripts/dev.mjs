// Starts an isolated T3 server plus the device simulator.
import { appendFileSync, mkdirSync, readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { DeviceBridge } from "../src/bridge.mjs";
import { ensureSandbox, startLocalT3 } from "../src/localT3.mjs";
import { createDeviceServer } from "../src/server.mjs";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const sandbox = join(root, "sandbox-repo");
const dataDir = join(root, "data");
const T3_PORT = Number(process.env.T3_PORT ?? 3927);
const SIM_PORT = Number(process.env.PORT ?? 3918);
const TTL_MS = Number(process.env.APPROVAL_TTL_MS ?? 120_000);

mkdirSync(dataDir, { recursive: true });
await ensureSandbox(sandbox);

const { backend, stop } = await startLocalT3({ root, sandbox, dataDir, port: T3_PORT });
const shutdown = () => {
  stop();
  process.exit(0);
};
process.on("SIGINT", shutdown);
process.on("SIGTERM", shutdown);

const activity = [];
const logPath = join(dataDir, "activity.jsonl");
const onActivity = (entry) => {
  activity.push(entry);
  if (activity.length > 200) activity.shift();
  appendFileSync(logPath, `${JSON.stringify(entry)}\n`);
};

let bridge;
const sim = createDeviceServer({
  port: SIM_PORT,
  publicDir: join(root, "public"),
  getBridge: () => bridge,
  activity,
});
bridge = new DeviceBridge({
  backend,
  send: sim.broadcast,
  profiles: JSON.parse(readFileSync(join(root, "profiles.json"), "utf8")),
  onActivity,
  ttlMs: TTL_MS,
});
bridge.start();
await backend.connect();
await sim.listen();
console.log(`Simulator ready at http://127.0.0.1:${SIM_PORT}`);
