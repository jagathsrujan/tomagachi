// Local risk rules. Anything not clearly understood is high risk, because high
// risk can only be approved with a stronger check on the phone or laptop.

export const DEVICE_TEXT_LIMIT = 140;

const SECRET =
  /(^|[\s/'"=])\.(env|npmrc|netrc|pgpass)\b|id_(rsa|ed25519)|\.pem\b|\.ssh\/|\.aws\/|\.config\/gh|credential|secret|token/i;
const SHELL_SYNTAX = /[;&|`$<>\\\n()]/;
const OUTSIDE_PROJECT = /^(\/|~|\.\.(\/|$))|\/\.\.(\/|$)/;
const UNSAFE_FLAG = /^--?(output|exec|delete|pre|force|hard|amend|global|system)\b|^-[a-zA-Z]*[fD]$/;

const LOW = [
  ["pwd"], ["ls"], ["cat"], ["head"], ["tail"], ["wc"], ["rg"], ["grep"],
  ["git", "status"], ["git", "diff"], ["git", "log"], ["git", "show"],
  ["npm", "test"], ["npm", "run", "test"], ["npm", "run", "lint"],
  ["pnpm", "test"], ["pnpm", "lint"], ["yarn", "test"],
  ["pytest"], ["cargo", "test"], ["go", "test"],
];
const MEDIUM = [["touch"], ["mkdir"], ["git", "add"], ["git", "commit"], ["git", "switch", "-c"]];

const high = (reason) => ({ risk: "high", reason });

/** Codex wraps commands as `/bin/zsh -lc '...'`. Returns the inner command. */
export const unwrapShell = (text) => {
  const match = text.match(/^(?:\/[\w/.-]*\/)?(?:zsh|bash|sh)\s+-l?c\s+(?:(['"])([\s\S]*)\1|(\S+))$/);
  return match ? (match[2] ?? match[3]) : text;
};

const tokenize = (command) => {
  const tokens = command.match(/"[^"]*"|'[^']*'|\S+/g) ?? [];
  return tokens.map((t) => t.replace(/^(["'])([\s\S]*)\1$/, "$2"));
};

const startsWith = (tokens, prefix) => prefix.every((word, i) => tokens[i] === word);

const classifyCommand = (raw) => {
  const command = unwrapShell(raw).trim();
  if (!command) return high("Empty command");
  if (SECRET.test(command)) return high("Touches secrets");
  if (SHELL_SYNTAX.test(command)) return high("Complex shell command");

  const tokens = tokenize(command);
  const args = tokens.slice(1);
  if (args.some((a) => OUTSIDE_PROJECT.test(a))) return high("Path outside the project");
  if (args.some((a) => UNSAFE_FLAG.test(a))) return high("Unsafe flag");

  if (LOW.some((p) => startsWith(tokens, p))) return { risk: "low", reason: "Read or test only" };
  if (MEDIUM.some((p) => startsWith(tokens, p))) return { risk: "medium", reason: "Local change" };
  return high("Unknown command");
};

export const classifyApproval = ({ kind, text }) => {
  const body = (text ?? "").trim();
  if (!body) return high("No details available");
  if (kind === "command") return classifyCommand(body);
  if (SECRET.test(body)) return high("Touches secrets");
  if (OUTSIDE_PROJECT.test(body)) return high("Path outside the project");
  if (kind === "file-read") return { risk: "low", reason: "Read file" };
  if (kind === "file-change") return { risk: "medium", reason: "Edit file" };
  return high("Unknown request type");
};

// The board's font is plain ASCII, so anything else could not be shown faithfully.
const PRINTABLE = /^[\x20-\x7e]*$/;

/** The device may only answer what is safe and fits on screen in full. */
export const canApproveOnDevice = ({ risk, text }) =>
  risk !== "high" && typeof text === "string" && text.length <= DEVICE_TEXT_LIMIT && PRINTABLE.test(text);
