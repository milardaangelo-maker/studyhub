/*global BareMux, __uv$config*/

const statusEl = document.getElementById("status");
const frame = document.getElementById("stage");
const input = document.getElementById("url");
const form = document.getElementById("go");

const connection = new BareMux.BareMuxConnection("/baremux/worker.js");
const wispUrl =
  (location.protocol === "https:" ? "wss" : "ws") +
  "://" +
  location.host +
  "/wisp/";
const bareUrl = location.origin + "/bare/";

// relay: "epoxy" (websocket, default) or "bare" (plain http fallback).
// flip with: localStorage.setItem("sh-relay", "bare")
const relay = localStorage.getItem("sh-relay") || "epoxy";

function setStatus(msg) {
  statusEl.textContent = msg;
}

// Boot: transport first, then service worker (order matters).
const ready = (async () => {
  if (!("serviceWorker" in navigator)) {
    setStatus("needs https or localhost to run");
    return;
  }
  setStatus("connecting…");
  if (relay === "bare") {
    await connection.setTransport("/baremod/index.mjs", [bareUrl]);
  } else {
    await connection.setTransport("/epoxy/index.mjs", [{ wisp: wispUrl }]);
  }
  await navigator.serviceWorker.register("/sw.js", { scope: "/" });
  await navigator.serviceWorker.ready;
  setStatus("");
})().catch((err) => setStatus("boot failed: " + err.message));

function normalize(q) {
  q = q.trim();
  if (!q) return null;
  if (/^https?:\/\//i.test(q)) return q;
  // domain-ish and no spaces -> URL, otherwise search
  if (!q.includes(" ") && /^[\w-]+(\.[\w-]+)+(:\d+)?([/?#]\S*)?$/.test(q))
    return "https://" + q;
  return "https://duckduckgo.com/?q=" + encodeURIComponent(q);
}

async function go(target) {
  const url = normalize(target);
  if (!url) return;
  setStatus("opening…");
  await ready;
  document.body.classList.add("browsing");
  frame.src = __uv$config.prefix + __uv$config.encodeUrl(url);
}

form.addEventListener("submit", (e) => {
  e.preventDefault();
  go(input.value);
});

// logo = home button
document.querySelector(".logo").addEventListener("click", () => {
  document.body.classList.remove("browsing");
  frame.src = "about:blank";
  input.value = "";
  input.focus();
});
