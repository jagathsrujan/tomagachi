// Local stand-in for the phone-to-device Bluetooth link: Server-Sent Events carry
// phone-to-device messages, and POST carries device-to-phone messages.
import { readFile } from "node:fs/promises";
import { createServer } from "node:http";
import { extname, join, normalize } from "node:path";

const TYPES = { ".html": "text/html; charset=utf-8", ".css": "text/css", ".js": "text/javascript" };
const MAX_BODY = 16_384;

export const createDeviceServer = ({ port, publicDir, getBridge, activity }) => {
  const clients = new Set();
  const origin = `http://127.0.0.1:${port}`;
  const allowedHosts = new Set([`127.0.0.1:${port}`, `localhost:${port}`]);

  const broadcast = (message) => {
    const frame = `data: ${JSON.stringify(message)}\n\n`;
    for (const res of clients) res.write(frame);
  };

  const readJson = (req) =>
    new Promise((resolve, reject) => {
      let body = "";
      req.on("data", (chunk) => {
        body += chunk;
        if (body.length > MAX_BODY) reject(new Error("body too large"));
      });
      req.on("end", () => {
        try {
          resolve(JSON.parse(body));
        } catch {
          reject(new Error("invalid JSON"));
        }
      });
      req.on("error", reject);
    });

  const json = (res, status, value) => {
    res.writeHead(status, { "content-type": "application/json" });
    res.end(JSON.stringify(value ?? {}));
  };

  const serveStatic = async (pathname, res) => {
    const relative = pathname === "/" ? "index.html" : normalize(pathname).replace(/^[/\\]+/, "");
    if (relative.startsWith("..")) return json(res, 404, { error: "not found" });
    try {
      const body = await readFile(join(publicDir, relative));
      res.writeHead(200, {
        "content-type": TYPES[extname(relative)] ?? "application/octet-stream",
        "cache-control": "no-store",
      });
      res.end(body);
    } catch {
      json(res, 404, { error: "not found" });
    }
  };

  const openEvents = (res) => {
    res.writeHead(200, {
      "content-type": "text/event-stream",
      "cache-control": "no-store",
      connection: "keep-alive",
    });
    clients.add(res);
    res.on("close", () => clients.delete(res));
    for (const message of getBridge().snapshotMessages()) res.write(`data: ${JSON.stringify(message)}\n\n`);
  };

  const server = createServer(async (req, res) => {
    // Loopback only, and other websites must not be able to drive approvals.
    if (!allowedHosts.has(req.headers.host)) return json(res, 403, { error: "bad host" });
    const url = new URL(req.url, origin);
    try {
      if (req.method === "GET" && url.pathname === "/events") return openEvents(res);
      if (req.method === "GET" && url.pathname === "/phone/state") {
        return json(res, 200, { queue: getBridge().phoneQueue(), activity: activity.slice(-30) });
      }
      if (req.method === "GET") return await serveStatic(url.pathname, res);
      if (req.method !== "POST") return json(res, 405, { error: "method not allowed" });

      const sameOrigin = [`http://${req.headers.host}`].includes(req.headers.origin);
      if (!sameOrigin || !req.headers["content-type"]?.startsWith("application/json")) {
        return json(res, 403, { error: "bad origin" });
      }
      const body = await readJson(req);
      if (url.pathname === "/device") {
        await getBridge().handleDeviceMessage(body);
        return json(res, 200, { ok: true });
      }
      if (url.pathname === "/phone/answer") {
        return json(res, 200, await getBridge().answerFromPhone(body));
      }
      json(res, 404, { error: "not found" });
    } catch (error) {
      json(res, 400, { error: error.message });
    }
  });

  return {
    broadcast,
    listen: () => new Promise((resolve) => server.listen(port, "127.0.0.1", resolve)),
    close: () => {
      for (const res of clients) res.end();
      server.close();
    },
  };
};
