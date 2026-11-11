// Laptop side of version 1: the board connects here over Wi-Fi, with no phone.
//   node scripts/link.mjs            Claude Code directly, through the Agent SDK
//   node scripts/link.mjs --t3       Claude Code and Codex through T3
// The page at http://127.0.0.1:3918 stays on the laptop and answers high-risk approvals.
import { randomBytes } from "node:crypto";
import { appendFileSync, existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { networkInterfaces } from "node:os";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { DeviceBridge } from "../src/bridge.mjs";
import { ClaudeBackend } from "../src/claudeBackend.mjs";
import { createDeviceLink } from "../src/deviceLink.mjs";
import { ensureSandbox, startLocalT3 } from "../src/localT3.mjs";
import { createDeviceServer } from "../src/server.mjs";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const sandbox = join(root, "sandbox-repo");
const dataDir = join(root, "data");
const secretsPath = join(root, "../firmware/agent_link/secrets.h");
const useT3 = process.argv.includes("--t3");
const LINK_PORT = Number(process.env.LINK_PORT ?? 3919);
const PAGE_PORT = Number(process.env.PORT ?? 3918);
const T3_PORT = Number(process.env.T3_PORT ?? 3927);
const TTL_MS = Number(process.env.APPROVAL_TTL_MS ?? 120_000);

const laptopAddress = () =>
  Object.values(networkInterfaces()).flat().find((n) => n.family === "IPv4" && !n.internal)?.address ?? "0.0.0.0";

const loadToken = () => {
  const path = join(dataDir, "device-token");
  if (!existsSync(path)) writeFileSync(path, randomBytes(32).toString("hex"), { mode: 0o600 });
  return readFileSync(path, "utf8").trim();
};

// Only the Wi-Fi name and password are left for you to fill in.
const writeSecrets = (token) => {
  if (existsSync(secretsPath)) return;
  writeFileSync(secretsPath, [
    "#pragma once",
    "// Local only. Fill in the Wi-Fi name and password. The ESP32 needs 2.4 GHz.",
    '#define WIFI_SSID "your-wifi-name"',
    '#define WIFI_PASSWORD "your-wifi-password"',
    `#define BRIDGE_HOST "${laptopAddress()}"`,
    `#define BRIDGE_PORT ${LINK_PORT}`,
    `#define DEVICE_TOKEN "${token}"`,
    "",
  ].join("\n"), { mode: 0o600 });
  console.log(`Wrote ${secretsPath}. Add your Wi-Fi name and password to it.`);
};

mkdirSync(dataDir, { recursive: true });
await ensureSandbox(sandbox);
const token = loadToken();
writeSecrets(token);

const t3 = useT3 ? await startLocalT3({ root, sandbox, dataDir, port: T3_PORT }) : null;
const backend = t3?.backend ?? new ClaudeBackend({
  projects: [{ id: "sandbox-repo", title: "sandbox-repo", path: sandbox }],
});
const profiles = JSON.parse(readFileSync(join(root, "profiles.json"), "utf8"))
  .filter((p) => useT3 || p.harness === "claudeAgent");

const activity = [];
const onActivity = (entry) => {
  activity.push(entry);
  if (activity.length > 200) activity.shift();
  appendFileSync(join(dataDir, "activity.jsonl"), `${JSON.stringify(entry)}\n`);
  console.log(`${entry.event}${entry.text ? `: ${entry.text}` : ""}`);
};

let bridge;
const getBridge = () => bridge;
const page = createDeviceServer({ port: PAGE_PORT, publicDir: join(root, "public"), getBridge, activity });
const link = createDeviceLink({ port: LINK_PORT, token, getBridge, log: console.log });
bridge = new DeviceBridge({
  backend,
  send: (message) => {
    page.broadcast(message);
    link.send(message);
  },
  profiles,
  onActivity,
  ttlMs: TTL_MS,
});

const shutdown = () => {
  backend.close();
  link.close();
  t3?.stop();
  process.exit(0);
};
process.on("SIGINT", shutdown);
process.on("SIGTERM", shutdown);

bridge.start();
await backend.connect();
await page.listen();
await link.listen();
console.log(`Backend: ${useT3 ? "T3 Code" : "Claude Code direct"}`);
console.log(`Waiting for the board at ${laptopAddress()}:${LINK_PORT}`);
console.log(`Approvals page: http://127.0.0.1:${PAGE_PORT}`);
