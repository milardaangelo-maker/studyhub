import express from "express";
import { createServer } from "node:http";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { uvPath } from "@titaniumnetwork-dev/ultraviolet";
import { epoxyPath } from "@mercuryworkshop/epoxy-transport";
import { baremuxPath } from "@mercuryworkshop/bare-mux/node";
import { bareModulePath } from "@mercuryworkshop/bare-as-module3";
import { createBareServer } from "@tomphttp/bare-server-node";
import wisp from "wisp-server-node";

const __dirname = dirname(fileURLToPath(import.meta.url));

const app = express();
const bare = createBareServer("/bare/");

// Our pages first. public/uv/uv.config.js intentionally shadows the vendor one.
app.use(express.static(join(__dirname, "public")));

// Vendor bundles.
app.use("/uv/", express.static(uvPath));
app.use("/epoxy/", express.static(epoxyPath));
app.use("/baremux/", express.static(baremuxPath));
app.use("/baremod/", express.static(bareModulePath));

// Anything unknown lands back on the console front page.
app.use((req, res) => {
  if (bare.shouldRoute(req)) return bare.routeRequest(req, res);
  res.redirect("/");
});

const server = createServer((req, res) => {
  // crossOriginIsolated: epoxy wants SharedArrayBuffer.
  res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
  res.setHeader("Cross-Origin-Embedder-Policy", "require-corp");
  if (bare.shouldRoute(req)) return bare.routeRequest(req, res);
  app(req, res);
});

server.on("upgrade", (req, socket, head) => {
  if (req.url.endsWith("/wisp/")) return wisp.routeRequest(req, socket, head);
  if (bare.shouldRoute(req)) return bare.routeUpgrade(req, socket, head);
  socket.end();
});

const port = Number(process.env.PORT) || 8090;
server.listen(port, () => {
  console.log(`study-hub up on http://localhost:${port}`);
});
