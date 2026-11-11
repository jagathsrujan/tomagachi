import assert from "node:assert/strict";
import { test } from "node:test";
import { assertBackend } from "../src/backend.mjs";
import { ClaudeBackend, describeTool } from "../src/claudeBackend.mjs";
import { classifyApproval } from "../src/risk.mjs";

const projects = [{ id: "p", title: "p", path: "/work/p" }];

// A scripted session: asks for one tool if given, then ends with a result.
const fakeQuery = ({ tool = null, result = { type: "result", subtype: "success", result: "All done" } } = {}) => {
  const calls = [];
  const query = ({ prompt, options }) => {
    calls.push({ options });
    const stream = async function* () {
      for await (const message of prompt) {
        calls.at(-1).prompt = message.message.content;
        yield { type: "system", session_id: "s1" };
        if (tool) calls.at(-1).decision = await options.canUseTool(tool.name, tool.input, {});
        yield { ...result, session_id: "s1" };
        return;
      }
    };
    return Object.assign(stream(), { interrupt: async () => {}, close: () => {} });
  };
  return { query, calls };
};

const setup = (script) => {
  const { query, calls } = fakeQuery(script);
  const backend = assertBackend(new ClaudeBackend({ projects, query, holdMs: 30 }));
  const threads = [];
  backend.subscribe((e) => { if (e.type === "thread") threads.push(e.thread); });
  const start = () => backend.startTask({ projectId: "p", harness: "claudeAgent", model: "m", title: "T", prompt: "go" });
  return { backend, calls, threads, start };
};

const until = async (check) => {
  for (let i = 0; i < 200; i += 1) {
    const value = check();
    if (value) return value;
    await new Promise((r) => setTimeout(r, 2));
  }
  throw new Error("timed out");
};

test("paths inside the project are relative, others stay absolute", () => {
  assert.deepEqual(describeTool("Edit", { file_path: "/work/p/src/a.js" }, "/work/p"), { kind: "file-change", text: "src/a.js" });
  assert.deepEqual(describeTool("Read", { file_path: "/etc/passwd" }, "/work/p"), { kind: "file-read", text: "/etc/passwd" });
  assert.equal(classifyApproval(describeTool("Edit", { file_path: "/etc/hosts" }, "/work/p")).risk, "high");
  assert.equal(classifyApproval(describeTool("Edit", { file_path: "/work/p/a.js" }, "/work/p")).risk, "medium");
});

test("unknown tools are high risk", () => {
  assert.equal(classifyApproval(describeTool("WebFetch", { url: "http://x" }, "/work/p")).risk, "high");
});

test("a session ends done with the answer, and keeps device approval rules on", async () => {
  const { calls, threads, start } = setup();
  await start();
  const last = await until(() => threads.find((t) => t.state === "done"));
  assert.equal(last.summary, "All done");
  assert.equal(calls[0].prompt, "go");
  assert.equal(calls[0].options.permissionMode, "default");
  assert.deepEqual(calls[0].options.settingSources, []);
});

test("each update is a new object, so the bridge can see changes", async () => {
  const { threads, start } = setup();
  await start();
  await until(() => threads.find((t) => t.state === "done"));
  assert.notEqual(threads[0], threads.at(-1));
  assert.equal(threads[0].state, "working");
});

test("approve lets the tool run with its original input", async () => {
  const tool = { name: "Bash", input: { command: "touch a" } };
  const { backend, calls, threads, start } = setup({ tool });
  const id = await start();
  const waiting = await until(() => threads.find((t) => t.state === "needs_you"));
  assert.deepEqual(await backend.getApproval(id, waiting.pending.id), {
    id: waiting.pending.id, kind: "command", text: "touch a", reason: null,
  });
  await backend.answerApproval(id, waiting.pending.id, "approve");
  await until(() => threads.find((t) => t.state === "done"));
  assert.deepEqual(calls[0].decision, { behavior: "allow", updatedInput: tool.input });
});

test("deny blocks the tool", async () => {
  const { backend, calls, threads, start } = setup({ tool: { name: "Bash", input: { command: "rm a" } } });
  const id = await start();
  const waiting = await until(() => threads.find((t) => t.state === "needs_you"));
  await backend.answerApproval(id, waiting.pending.id, "deny");
  await until(() => threads.find((t) => t.state === "done"));
  assert.equal(calls[0].decision.behavior, "deny");
});

test("no answer ends in a denial, never an approval", async () => {
  const { calls, threads, start } = setup({ tool: { name: "Bash", input: { command: "rm a" } } });
  await start();
  await until(() => threads.find((t) => t.state === "done"));
  assert.equal(calls[0].decision.behavior, "deny");
  assert.match(calls[0].decision.message, /in time/);
});

test("answering twice fails the second time", async () => {
  const { backend, threads, start } = setup({ tool: { name: "Bash", input: { command: "ls" } } });
  const id = await start();
  const waiting = await until(() => threads.find((t) => t.state === "needs_you"));
  await backend.answerApproval(id, waiting.pending.id, "deny");
  await assert.rejects(backend.answerApproval(id, waiting.pending.id, "approve"));
});

test("a failed result shows as failed", async () => {
  const { threads, start } = setup({ result: { type: "result", subtype: "success", is_error: true, result: "Not logged in" } });
  await start();
  const failed = await until(() => threads.find((t) => t.state === "failed"));
  assert.equal(failed.summary, "Not logged in");
});

test("a follow-up resumes the same Claude Code session", async () => {
  const { backend, calls, threads, start } = setup();
  const id = await start();
  await until(() => threads.find((t) => t.state === "done"));
  await backend.sendMessage(id, "more");
  await until(() => calls.length === 2 && calls[1].prompt);
  assert.equal(calls[1].options.resume, "s1");
  assert.equal(calls[1].prompt, "more");
});

test("a busy session refuses a second message", async () => {
  const { backend, threads, start } = setup({ tool: { name: "Bash", input: { command: "ls" } } });
  const id = await start();
  await until(() => threads.find((t) => t.state === "needs_you"));
  await assert.rejects(backend.sendMessage(id, "more"), /busy/);
});
