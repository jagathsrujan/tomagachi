// Simulated device firmware plus a thin phone panel. Device logic only uses the
// protocol messages, so the same screens can be ported to the real firmware.
const $ = (id) => document.getElementById(id);
const esc = (value) =>
  String(value ?? "").replace(/[&<>"']/g, (c) => `&#${c.charCodeAt(0)};`);

const LABELS = { working: "Working", needs_you: "Needs you", done: "Done", failed: "Failed", idle: "Idle" };
const FACES = { working: "◕ ◕", needs_you: "◉ ◉", done: "^ ^", failed: "x x", idle: "– –", offline: "· ·" };
const AGENTS = { claudeAgent: "Claude Code", codex: "Codex" };
const agentName = (id) => AGENTS[id] ?? id;

const device = {
  online: false,
  synced: false,
  tasks: [],
  counts: { working: 0, needs_you: 0, done: 0, failed: 0, idle: 0 },
  profiles: [],
  approvals: [],
  screen: "home",
  sel: 0,
  taskId: null,
  profileId: null,
  voice: null,
  note: "",
};

const wire = (direction, message) => {
  const li = document.createElement("li");
  li.className = direction;
  li.textContent = `${direction === "in" ? "← " : "→ "}${JSON.stringify(message)}`;
  $("wire").prepend(li);
  while ($("wire").children.length > 80) $("wire").lastChild.remove();
};

const sendToPhone = async (message) => {
  wire("out", message);
  await fetch("/device", {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify(message),
  });
};

const buzz = (label) => {
  const shell = $("shell");
  shell.classList.remove("buzzing");
  void shell.offsetWidth;
  shell.classList.add("buzzing");
  $("buzz").textContent = `Vibrate: ${label}`;
};

const setNote = (text) => {
  device.note = text;
  render();
};

let noteTimer = null;
const scheduleNoteClear = () => {
  clearTimeout(noteTimer);
  noteTimer = setTimeout(() => {
    device.note = "";
    render();
  }, 2500);
};

const go = (screen, extra = {}) => {
  Object.assign(device, { screen, sel: 0, ...extra });
  render();
};

const currentTask = () => device.tasks.find((t) => t.id === device.taskId);
const currentProfile = () => device.profiles.find((p) => p.id === device.profileId);
const topApproval = () => device.approvals[0];

const menuItems = () => {
  if (device.screen === "home") return [{ label: "Tasks", go: "tasks" }, { label: "New task", go: "delegate" }];
  if (device.screen === "tasks") return device.tasks.map((t) => ({ label: t.title, state: t.state, id: t.id }));
  if (device.screen === "delegate") return device.profiles.map((p) => ({ label: p.name, id: p.id }));
  if (device.screen === "presets") return (currentProfile()?.presets ?? []).map((label) => ({ label }));
  if (device.screen === "detail") return [{ label: "Stop task", action: "stop" }];
  return [];
};

const menuHtml = (items) =>
  `<div class="menu">${items
    .slice(Math.max(0, device.sel - 3), Math.max(0, device.sel - 3) + 5)
    .map((item) => {
      const index = items.indexOf(item);
      const dot = item.state ? `<i class="dot s-${esc(item.state)}"></i>` : "";
      return `<button class="item ${index === device.sel ? "sel" : ""}" data-i="${index}" type="button"><span>${esc(item.label)}</span>${dot}</button>`;
    })
    .join("")}</div>`;

const views = {
  home() {
    const c = device.counts;
    const mood = !device.online ? "offline" : c.needs_you ? "needs_you" : c.failed ? "failed" : c.working ? "working" : c.done ? "done" : "idle";
    return `<div class="face" aria-hidden="true">${FACES[mood]}</div>
      <div class="row">
        <div class="stat"><b>${c.working}</b>Working</div>
        <div class="stat"><b>${c.needs_you}</b>Needs you</div>
        <div class="stat"><b>${c.done}</b>Done</div>
        <div class="stat"><b>${c.failed}</b>Failed</div>
      </div>
      ${device.online ? "" : '<p class="small">Offline</p>'}
      ${menuHtml(menuItems())}`;
  },
  tasks() {
    const items = menuItems();
    return `<h4>Tasks</h4>${items.length ? menuHtml(items) : '<p class="small">No tasks yet</p>'}`;
  },
  detail() {
    const task = currentTask();
    if (!task) return '<p class="small">Task is gone</p>';
    const canStop = task.state === "working" || task.state === "needs_you";
    return `<h4>${esc(agentName(task.agent))} · ${esc(LABELS[task.state])}</h4>
      <p class="detail"><b>${esc(task.title)}</b></p>
      <p class="detail">${esc(task.needsPhone ? "Open the phone to answer." : task.summary || "No update yet")}</p>
      ${canStop ? menuHtml(menuItems()) : ""}`;
  },
  delegate() {
    return `<h4>New task: pick an agent</h4>${menuHtml(menuItems())}`;
  },
  presets() {
    return `<h4>${esc(currentProfile()?.name)}</h4>${menuHtml(menuItems())}`;
  },
  confirm() {
    return `<h4>Start this task?</h4>
      <p class="detail">${esc(device.voice?.text)}</p>
      <p class="small">With ${esc(device.profiles[device.sel]?.name ?? device.profiles[0]?.name)}. Press B to pick another agent.</p>
      <div class="keys"><span><b>B</b> Agent / cancel</span><span><b>A</b> Start</span></div>`;
  },
  confirmReply() {
    return `<h4>Reply to ${esc(currentTask()?.title)}?</h4>
      <p class="detail">${esc(device.voice?.text)}</p>
      <div class="keys"><span><b>B</b> Cancel</span><span><b>A</b> Send</span></div>`;
  },
  approval() {
    const a = topApproval();
    const queued = device.approvals.length > 1 ? `<p class="small">+${device.approvals.length - 1} more waiting</p>` : "";
    const body = a.deviceApprovable
      ? `<div class="cmd">${esc(a.text)}</div>`
      : `<p class="detail">${esc(a.reason)}. Open the phone to approve.</p>`;
    return `<span class="pill r-${esc(a.risk)}">${esc(a.risk.toUpperCase())}</span>
      <h4>${esc(agentName(a.agent))}</h4>${body}${queued}
      <div class="keys"><span><b>B</b> Deny</span><span>${a.deviceApprovable ? "<b>A</b> Approve" : "Approve on phone"}</span></div>`;
  },
};

function render() {
  const view = topApproval() ? "approval" : device.screen;
  const note = device.note ? `<p class="small" role="status">${esc(device.note)}</p>` : "";
  if (device.note) scheduleNoteClear();
  $("screen").innerHTML = views[view]() + note;
  $("btnA").disabled = view === "approval" && !topApproval().deviceApprovable;
  const link = $("link");
  link.textContent = device.online ? "Agents connected" : "Agents offline";
  link.className = `link ${device.online ? "online" : "offline"}`;
}

const answer = (decision) => {
  const a = topApproval();
  if (!a || a.sent) return;
  a.sent = true;
  setNote("Sending…");
  sendToPhone({ type: "approval.response", id: a.id, nonce: a.nonce, decision });
};

const startFromConfirm = () => {
  const profile = device.profiles[device.sel] ?? device.profiles[0];
  const ref = crypto.randomUUID();
  sendToPhone({ type: "task.start", ref, profile: profile.id, text: device.voice.text });
  device.voice = null;
  go("home", { note: "Starting…" });
};

const select = () => {
  if (topApproval()) return answer("approve");
  if (device.screen === "confirm") return startFromConfirm();
  if (device.screen === "confirmReply") {
    sendToPhone({ type: "task.reply", id: device.taskId, text: device.voice.text });
    device.voice = null;
    return go("detail", { note: "Reply sent" });
  }
  const item = menuItems()[device.sel];
  if (!item) return;
  if (device.screen === "home") return go(item.go);
  if (device.screen === "tasks") return go("detail", { taskId: item.id });
  if (device.screen === "delegate") return go("presets", { profileId: item.id });
  if (device.screen === "presets") {
    sendToPhone({ type: "task.start", ref: crypto.randomUUID(), profile: device.profileId, preset: item.label });
    return go("home", { note: "Starting…" });
  }
  if (item.action === "stop") {
    sendToPhone({ type: "task.stop", id: device.taskId });
    setNote("Stopping…");
  }
};

const back = () => {
  if (topApproval()) return answer("deny");
  if (device.screen === "confirm") {
    if (device.sel + 1 < device.profiles.length) {
      device.sel += 1;
      return render();
    }
    device.voice = null;
    return go("home");
  }
  const parents = { tasks: "home", delegate: "home", detail: "tasks", presets: "delegate", confirmReply: "detail" };
  go(parents[device.screen] ?? "home");
};

const upsertTask = (task) => {
  const index = device.tasks.findIndex((t) => t.id === task.id);
  if (index >= 0) device.tasks[index] = task;
  else device.tasks.unshift(task);
};

const handlers = {
  "state.snapshot"(m) {
    Object.assign(device, { online: m.online, synced: m.synced, tasks: m.tasks, counts: m.counts });
  },
  "task.update"(m) {
    upsertTask(m.task);
    device.counts = m.counts;
    if (m.alert === "done") buzz("task done");
    if (m.alert === "failed") buzz("task failed");
  },
  "profiles.sync"(m) {
    device.profiles = m.profiles;
  },
  "approval.request"(m) {
    if (device.approvals.some((a) => a.id === m.id)) return;
    device.approvals.push(m);
    buzz(`needs you (${m.risk})`);
  },
  "approval.cancel"(m) {
    device.approvals = device.approvals.filter((a) => a.id !== m.id);
    if (m.reason === "expired") device.note = "Timed out. Check the phone.";
  },
  "approval.result"(m) {
    const a = device.approvals.find((x) => x.id === m.id);
    if (m.status === "accepted") {
      device.approvals = device.approvals.filter((x) => x.id !== m.id);
      device.note = m.decision === "approve" ? "Approved" : "Denied";
    } else if (m.status === "failed") {
      if (a) a.sent = false;
      device.note = "Failed. Try again.";
    } else {
      device.approvals = device.approvals.filter((x) => x.id !== m.id);
      device.note = m.status === "needs_phone" ? "Use the phone." : "No longer valid";
    }
  },
  "task.started"() {
    device.note = "Task started";
  },
  "task.error"(m) {
    device.note = m.reason;
  },
};

const connect = () => {
  const events = new EventSource("/events");
  events.onmessage = (event) => {
    const message = JSON.parse(event.data);
    wire("in", message);
    handlers[message.type]?.(message);
    render();
    refreshPhone();
  };
  events.onerror = () => {
    device.online = false;
    device.approvals = [];
    render();
  };
};

let lastQueueHtml = "";
const refreshPhone = async () => {
  const state = await fetch("/phone/state").then((r) => r.json()).catch(() => null);
  if (!state) return;
  const phoneItems = state.queue.filter((a) => !a.deviceApprovable || a.state === "expired");
  const queueHtml = phoneItems
    .map((a) => `<div class="card" data-id="${esc(a.id)}">
        <span class="pill r-${esc(a.risk)}">${esc(a.risk.toUpperCase())}</span>
        <p>${esc(agentName(a.agent))} · ${esc(a.title)} · ${esc(a.reason)}</p>
        <pre>${esc(a.text ?? "No details available")}</pre>
        <div class="actions">
          <button class="primary" data-decision="approve" type="button">Approve with fingerprint (simulated)</button>
          <button data-decision="deny" type="button">Deny</button>
        </div></div>`)
    .join("");
  // Redrawing only on change, so a poll can never swallow a tap on these buttons.
  if (queueHtml !== lastQueueHtml) {
    lastQueueHtml = queueHtml;
    $("queue").innerHTML = queueHtml;
  }
  $("activity").innerHTML = state.activity
    .slice()
    .reverse()
    .map((e) => `<li>${esc(e.at.slice(11, 19))} ${esc(e.event)} ${esc(e.decision ?? "")} ${esc(e.source ?? "")} ${esc(e.text ?? e.prompt ?? e.detail ?? "")}</li>`)
    .join("");
};

$("btnA").addEventListener("click", select);
$("btnB").addEventListener("click", back);
$("screen").addEventListener("click", (event) => {
  const item = event.target.closest(".item");
  if (!item) return;
  device.sel = Number(item.dataset.i);
  select();
});
document.addEventListener("keydown", (event) => {
  if (event.target.tagName === "TEXTAREA") return;
  const items = menuItems();
  if (topApproval() || !items.length) return;
  if (event.key === "ArrowDown") device.sel = Math.min(items.length - 1, device.sel + 1);
  else if (event.key === "ArrowUp") device.sel = Math.max(0, device.sel - 1);
  else return;
  event.preventDefault();
  render();
});
$("queue").addEventListener("click", async (event) => {
  const button = event.target.closest("button[data-decision]");
  if (!button) return;
  const id = button.closest(".card").dataset.id;
  const decision = button.dataset.decision;
  button.disabled = true;
  await fetch("/phone/answer", {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify({ id, decision, biometricOk: decision === "approve" }),
  });
  refreshPhone();
});
$("voice").addEventListener("submit", (event) => {
  event.preventDefault();
  const text = $("voiceText").value.trim();
  if (!text || !device.profiles.length) return;
  $("voiceText").value = "";
  go(device.screen === "detail" && currentTask() ? "confirmReply" : "confirm", { voice: { text } });
});

render();
connect();
setInterval(refreshPhone, 3000);
