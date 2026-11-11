import assert from "node:assert/strict";
import { test } from "node:test";
import { isAuthoritativeSnapshot, normalizeThread, stateOf } from "../src/t3Backend.mjs";

test("maps T3 run status to device states", () => {
  assert.equal(stateOf({ status: "running" }), "working");
  assert.equal(stateOf({ status: "waiting" }), "working");
  assert.equal(stateOf({ status: "completed" }), "done");
  assert.equal(stateOf({ status: "failed" }), "failed");
  assert.equal(stateOf({ status: "cancelled" }), "failed");
  assert.equal(stateOf({ status: "idle" }), "idle");
});

test("a pending request always means needs_you", () => {
  assert.equal(stateOf({ status: "running", pendingRuntimeRequest: { id: "r" } }), "needs_you");
});

test("a failed task shows the error, not the last message", () => {
  const thread = normalizeThread({
    id: "t", title: "x", providerInstanceId: "codex", projectId: "p", status: "failed",
    lastError: "model not supported", latestVisibleMessage: { text: "hello" }, pendingRuntimeRequest: null,
  });
  assert.equal(thread.summary, "model not supported");
});

test("T3's project-only follow-up snapshot must not wipe the task list", () => {
  assert.equal(isAuthoritativeSnapshot({ kind: "snapshot", snapshot: { threads: [] } }), true);
  assert.equal(
    isAuthoritativeSnapshot({ kind: "snapshot", snapshot: { threads: [] }, resolvedRepositoryIdentityRoots: ["/x"] }),
    false,
  );
});
