// The contract between the app and any agent system. T3Backend is the v1
// implementation; hooks, Hermes or OpenClaw can be added behind the same shape.

/**
 * @typedef {"idle"|"working"|"needs_you"|"done"|"failed"} TaskState
 *
 * @typedef {object} PendingRequest
 * @property {string} id
 * @property {string} kind  command, file-read, file-change, user_input, ...
 * @property {boolean} blocking
 *
 * @typedef {object} Thread
 * @property {string} id
 * @property {string} title
 * @property {string} harness
 * @property {string} projectId
 * @property {TaskState} state
 * @property {string} summary
 * @property {string} updatedAt
 * @property {PendingRequest|null} pending
 *
 * @typedef {{type:"connection",online:boolean,detail?:string}
 *   |{type:"snapshot",threads:Thread[]}
 *   |{type:"thread",thread:Thread}
 *   |{type:"thread-removed",threadId:string}} AgentEvent
 *
 * @typedef {object} TaskRequest
 * @property {string} projectId
 * @property {string} harness
 * @property {string} model
 * @property {string} title
 * @property {string} prompt
 *
 * @typedef {object} ApprovalDetail
 * @property {string} id
 * @property {string} kind
 * @property {string|null} text  Exact action, or null when it could not be read.
 * @property {string|null} reason
 *
 * @typedef {object} AgentBackend
 * @property {() => Promise<void>} connect
 * @property {() => void} close
 * @property {(onEvent:(e:AgentEvent)=>void) => (()=>void)} subscribe
 * @property {() => Promise<Array<{id:string,title:string,path:string}>>} listProjects
 * @property {(t:TaskRequest) => Promise<string>} startTask
 * @property {(threadId:string,text:string) => Promise<void>} sendMessage
 * @property {(threadId:string) => Promise<void>} stopTask
 * @property {(threadId:string) => Promise<string>} getSummary  Latest agent reply, on one line.
 * @property {(threadId:string,requestId:string) => Promise<ApprovalDetail|null>} getApproval
 * @property {(threadId:string,requestId:string,decision:"approve"|"deny") => Promise<void>} answerApproval
 */

export const BACKEND_METHODS = [
  "connect",
  "close",
  "subscribe",
  "listProjects",
  "startTask",
  "sendMessage",
  "stopTask",
  "getSummary",
  "getApproval",
  "answerApproval",
];

export const assertBackend = (backend) => {
  const missing = BACKEND_METHODS.filter((m) => typeof backend[m] !== "function");
  if (missing.length) throw new TypeError(`Backend is missing: ${missing.join(", ")}`);
  return backend;
};
