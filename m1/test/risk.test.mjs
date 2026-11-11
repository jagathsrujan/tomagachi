import assert from "node:assert/strict";
import { test } from "node:test";
import { canApproveOnDevice, classifyApproval, DEVICE_TEXT_LIMIT, unwrapShell } from "../src/risk.mjs";

const risk = (text, kind = "command") => classifyApproval({ kind, text }).risk;

test("unwraps the Codex shell wrapper", () => {
  assert.equal(unwrapShell("/bin/zsh -lc 'touch a.txt'"), "touch a.txt");
  assert.equal(unwrapShell("touch a.txt"), "touch a.txt");
  assert.equal(unwrapShell("/bin/zsh -lc ls"), "ls");
});

test("an unquoted wrapper is judged by its inner command", () => {
  assert.equal(risk("/bin/zsh -lc ls"), "low");
  assert.equal(risk("/bin/zsh -lc rm"), "high");
  assert.equal(risk("/bin/zsh -lc ls -la"), "high");
  assert.equal(risk("/bin/zsh -lc ls; rm a"), "high");
});

test("reads and tests are low risk", () => {
  for (const c of ["ls", "ls -la", "git status", "git diff", "npm test", "pytest", "/bin/zsh -lc 'ls'"]) {
    assert.equal(risk(c), "low", c);
  }
});

test("local changes are medium risk", () => {
  for (const c of ["touch a.txt", "mkdir src", "git add .", 'git commit -m "fix bug"']) {
    assert.equal(risk(c), "medium", c);
  }
});

test("destructive and outward actions are high risk", () => {
  for (const c of [
    "rm a.txt", "rm -rf /", "git push", "git push origin main", "sudo ls", "curl https://x.test",
    "npm publish", "git commit --amend", "git reset --hard", "chmod 777 a", "mv a b",
  ]) {
    assert.equal(risk(c), "high", c);
  }
});

test("unknown commands are high risk", () => assert.equal(risk("frobnicate --now"), "high"));

test("shell tricks cannot hide behind a safe first word", () => {
  for (const c of [
    "ls; rm -rf /", "ls && git push", "ls | sh", "cat a > b", "ls $(rm a)", "ls `rm a`",
    "touch a\nrm b", "/bin/zsh -lc 'ls; rm a'", "git status & rm a", "ls \\\nrm",
  ]) {
    assert.equal(risk(c), "high", c);
  }
});

test("secrets are high risk even with read commands", () => {
  for (const c of ["cat .env", "cat config/.env", "cat id_rsa", "grep key server.pem", "cat aws_credentials.json"]) {
    assert.equal(risk(c), "high", c);
  }
});

test("paths outside the project are high risk", () => {
  for (const c of ["cat /etc/passwd", "cat ~/notes", "ls ../other", "touch a/../../b"]) {
    assert.equal(risk(c), "high", c);
  }
});

test("unsafe flags on allowed commands are high risk", () => {
  for (const c of ["git diff --output=a", "git log --output=a", "git add --force a", "git add -f a"]) {
    assert.equal(risk(c), "high", c);
  }
});

test("missing details are high risk", () => {
  assert.equal(risk(null), "high");
  assert.equal(risk("   "), "high");
  assert.equal(risk("anything", "permission"), "high");
  assert.equal(risk("anything", "mcp-elicitation"), "high");
});

test("file requests", () => {
  assert.equal(risk("src/a.ts", "file-read"), "low");
  assert.equal(risk("src/a.ts", "file-change"), "medium");
  assert.equal(risk(".env", "file-read"), "high");
  assert.equal(risk("/etc/hosts", "file-change"), "high");
});

test("device can only approve safe text that fits", () => {
  assert.equal(canApproveOnDevice({ risk: "low", text: "ls" }), true);
  assert.equal(canApproveOnDevice({ risk: "medium", text: "x".repeat(DEVICE_TEXT_LIMIT) }), true);
  assert.equal(canApproveOnDevice({ risk: "medium", text: "x".repeat(DEVICE_TEXT_LIMIT + 1) }), false);
  assert.equal(canApproveOnDevice({ risk: "high", text: "rm a" }), false);
  assert.equal(canApproveOnDevice({ risk: "low", text: null }), false);
});

test("text the device font cannot show is not approvable on the device", () => {
  assert.equal(canApproveOnDevice({ risk: "low", text: "cat naïve.txt" }), false);
  assert.equal(canApproveOnDevice({ risk: "low", text: "cat plain.txt" }), true);
});
