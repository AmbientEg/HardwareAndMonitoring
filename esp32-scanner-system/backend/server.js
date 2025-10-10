/************************************************************
 *  ESP32 BLE Scanner Backend (iBeacon + BLE Metadata)
 *  Node.js + Express + Socket.IO + SQLite (better-sqlite3)
 *  Hardened against aborted/bad requests
 ************************************************************/

const express = require("express");
const http = require("http");
const socketIO = require("socket.io");
const cors = require("cors");
const dotenv = require("dotenv");
const Database = require("better-sqlite3");

// Load .env
dotenv.config();

const app = express();
app.use(express.json());
app.use(express.urlencoded({ extended: true }));
const server = http.createServer(app);
const io = socketIO(server, { cors: { origin: "*" } });

// Middleware: CORS + JSON body parser with limit
app.use(cors());
app.use(express.json({ limit: "1mb" })); // protects against very large/partial bodies

// Small helper logger for aborted requests
app.use((req, res, next) => {
  req.on("aborted", () => {
    console.warn(
      `⚠️ Request aborted by client: ${req.method} ${req.originalUrl}`
    );
  });
  next();
});

// Initialize DB
const db = new Database(process.env.DB_PATH || "./db/beacons.db");

// Schema (idempotent)
db.exec(`
CREATE TABLE IF NOT EXISTS devices (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  mac TEXT UNIQUE NOT NULL,
  name TEXT,
  type TEXT,
  protocol TEXT,
  first_seen INTEGER,
  last_seen INTEGER
);

CREATE TABLE IF NOT EXISTS readings (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  device_id INTEGER NOT NULL,
  rssi INTEGER NOT NULL,
  adv_data TEXT,
  ts INTEGER NOT NULL,
  tx_beacon_name TEXT,
  major INTEGER,
  minor INTEGER,
  tx_power INTEGER,
  FOREIGN KEY(device_id) REFERENCES devices(id) ON DELETE CASCADE
);
`);

// Socket.IO connection logging
io.on("connection", (socket) => {
  console.log("🔌 Frontend Connected:", socket.id);
});

// Utility: safe number parsing that returns null for missing/invalid values
function parseNumberOrNull(v) {
  if (v === undefined || v === null || v === "") return null;
  const n = Number(v);
  return Number.isFinite(n) ? n : null;
}

// API to receive scan data
app.post("/api/scan", async (req, res) => {
  // Basic validation & normalization
  let scans = req.body;

  if (!scans) {
    console.warn("Received empty body for /api/scan");
    return res.status(400).json({ error: "Empty body" });
  }

  // Accept a single object or an array
  if (!Array.isArray(scans)) {
    if (typeof scans === "object") scans = [scans];
    else {
      return res
        .status(400)
        .json({ error: "Payload must be an object or an array" });
    }
  }

  // Defensive: limit number of items accepted in one request
  const MAX_BATCH = 500;
  if (scans.length > MAX_BATCH) {
    return res
      .status(413)
      .json({ error: `Batch too large (max ${MAX_BATCH})` });
  }

  const now = Date.now();

  const insertDevice = db.prepare(`
    INSERT OR IGNORE INTO devices (mac, name, type, protocol, first_seen, last_seen)
    VALUES (?, ?, ?, ?, ?, ?)
  `);

  const updateDevice = db.prepare(`
    UPDATE devices SET last_seen = ?, protocol = ? WHERE mac = ?
  `);

  const insertReading = db.prepare(`
    INSERT INTO readings (device_id, rssi, adv_data, ts, tx_beacon_name, major, minor, tx_power)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
  `);

  const getDeviceId = db.prepare(`SELECT id FROM devices WHERE mac = ?`);

  try {
    // transaction to keep DB consistent
    const txn = db.transaction((items) => {
      items.forEach((device) => {
        // Basic device shape validation
        if (!device || !device.mac) {
          // skip malformed entries but continue processing the batch
          console.warn(
            "Skipping malformed device entry (missing mac):",
            device
          );
          return;
        }

        // Normalize fields
        const mac = String(device.mac);
        const name = device.name ? String(device.name) : "Unknown";
        const type = device.type ? String(device.type) : "BLE";
        const protocol = device.protocol ? String(device.protocol) : null;

        // Upsert device basic info
        insertDevice.run(mac, name, type, protocol, now, now);
        updateDevice.run(now, protocol, mac);

        const dev = getDeviceId.get(mac);
        if (!dev) {
          // This should not happen because insertDevice.run + get should return, but guard anyway
          console.warn("Device inserted but not found afterwards:", mac);
          return;
        }

        // Reading fields
        const rssi = parseNumberOrNull(device.rssi);
        // rssi is required in schema; if missing/invalid, skip this reading
        if (rssi === null) {
          // skip reading insert but continue with next device
          console.warn(
            `Skipping reading for ${mac} because rssi is invalid:`,
            device.rssi
          );
          return;
        }

        const adv_data = device.adv_data !== undefined ? device.adv_data : {};
        const ts = device.ts ? parseNumberOrNull(device.ts) : now;

        // Transmitter metadata (from ESP), keep null if not provided
        const major = parseNumberOrNull(device.major);
        const minor = parseNumberOrNull(device.minor);
        const tx_power = parseNumberOrNull(device.tx_power);
        const tx_beacon_name = device.tx_beacon_name
          ? String(device.tx_beacon_name)
          : null;

        insertReading.run(
          dev.id,
          rssi,
          JSON.stringify(adv_data),
          ts || now,
          tx_beacon_name,
          major,
          minor,
          tx_power
        );
      });
    });

    // Run transaction with sanitized scans
    txn(scans);

    // Emit realtime update (emit sanitized minimal payload to listeners)
    const emitPayload = scans.map((d) => ({
      mac: d.mac,
      name: d.name || "Unknown",
      type: d.type || "BLE",
      protocol: d.protocol || null,
      rssi: parseNumberOrNull(d.rssi),
      adv_data: d.adv_data || {},
      ts: d.ts || now,
      major: parseNumberOrNull(d.major),
      minor: parseNumberOrNull(d.minor),
      tx_power: parseNumberOrNull(d.tx_power),
      tx_beacon_name: d.tx_beacon_name || null,
    }));
    io.emit("scan:batch", emitPayload);

    return res.json({ status: "OK", received: scans.length });
  } catch (err) {
    console.error("Error processing /api/scan:", err);
    return res.status(500).json({ error: "Internal server error" });
  }
});

// REST APIs
app.get("/api/devices", (req, res) => {
  try {
    const rows = db
      .prepare(
        `
        SELECT 
          d.mac,
          d.name,
          d.type,
          d.protocol,
          d.first_seen,
          d.last_seen,
          ROUND(AVG(r.rssi), 1) AS avg_rssi,
          r.major,
          r.minor,
          r.tx_power
        FROM devices d
        LEFT JOIN readings r ON d.id = r.device_id
        GROUP BY d.id
        ORDER BY d.last_seen DESC
      `
      )
      .all();

    res.json(rows);
  } catch (err) {
    console.error("GET /api/devices error:", err);
    res.status(500).json({ error: "Internal server error" });
  }
});

app.get("/api/devices/:mac/readings", (req, res) => {
  try {
    const mac = req.params.mac;
    const device = db.prepare("SELECT id FROM devices WHERE mac = ?").get(mac);

    if (!device) return res.status(404).json({ error: "Device not found" });

    const readings = db
      .prepare(
        "SELECT rssi, ts, tx_beacon_name, major, minor, tx_power, adv_data FROM readings WHERE device_id = ? ORDER BY ts DESC LIMIT 5000"
      )
      .all(device.id);

    res.json(readings);
  } catch (err) {
    console.error("GET /api/devices/:mac/readings error:", err);
    res.status(500).json({ error: "Internal server error" });
  }
});

// Catch JSON parse errors and other body parser issues
app.use((err, req, res, next) => {
  if (err && err.type === "entity.parse.failed") {
    console.warn("Invalid JSON received:", err);
    return res.status(400).json({ error: "Invalid JSON" });
  }
  // For other errors, delegate to default error handler
  next(err);
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`🚀 Server running on http://localhost:${PORT}`);
});
