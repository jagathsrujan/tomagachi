// Starts an isolated T3 server for local use. The token stays in memory.
import { execFile, spawn } from "node:child_process";
import { createWriteStream, existsSync, mkdirSync, writeFileSync } from "node:fs";
import { join } from "node:path";
import { promisify } from "node:util";
import { T3Backend } from "./t3Backend.mjs";

const run = promisify(execFile);

export const ensureSandbox = async (sandbox) => {
  if (existsSync(join(sandbox, ".git"))) return;
  mkdirSync(sandbox, { recursive: true });
  writeFileSync(join(sandbox, "README.md"), "# Sandbox\nThrowaway repo for the M1 simulator.\n");
  const git = (...args) => run("git", args, { cwd: sandbox });
  await git("init", "-q");
  await git("add", "-A");
  await git("-c", "user.name=m1", "-c", "user.email=m1@example.test", "commit", "-qm", "init");
};

export const startLocalT3 = async ({ root, sandbox, dataDir, port }) => {
  const t3Bin = join(root, "node_modules/.bin/t3");
  const t3Home = join(root, "t3home");
  const t3Url = `http://127.0.0.1:${port}`;
  const t3 = (...args) => run(t3Bin, [...args, "--base-dir", t3Home], { cwd: root });

  const t3Log = createWriteStream(join(dataDir, "t3.log"), { flags: "a" });
  const server = spawn(
    t3Bin,
    ["serve", "--base-dir", t3Home, "--host", "127.0.0.1", "--port", String(port), sandbox],
    { cwd: root, stdio: ["ignore", "pipe", "pipe"] },
  );
  server.stdout.pipe(t3Log);
  server.stderr.pipe(t3Log);
  server.on("exit", (code) => console.log(`T3 exited with code ${code}`));

  let ready = false;
  for (let i = 0; i < 100 && !ready; i += 1) {
    ready = await fetch(`${t3Url}/.well-known/t3/environment`).then((r) => r.ok).catch(() => false);
    if (!ready) await new Promise((r) => setTimeout(r, 200));
  }
  if (!ready) {
    server.kill("SIGTERM");
    throw new Error("T3 did not start. See data/t3.log");
  }

  await t3("project", "add", "--title", "sandbox-repo", sandbox).catch(() => {});
  const { stdout } = await t3(
    "auth", "session", "issue",
    "--scope", "orchestration:read", "--scope", "orchestration:operate",
    "--ttl", "12h", "--label", "m1-simulator", "--token-only",
  );
  return {
    backend: new T3Backend({ baseUrl: t3Url, token: stdout.trim() }),
    stop: () => server.kill("SIGTERM"),
  };
};
