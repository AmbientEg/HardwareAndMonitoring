# ESP32 BLE Scanner System

This project is a full-stack system for scanning BLE/iBeacon devices using an ESP32, storing the data in a backend database, and visualizing it in a real-time web dashboard. It includes:

- **ESP32 Firmware** for scanning BLE beacons
- **Backend (Node.js + Express + SQLite)** for data ingestion and real-time updates
- **Frontend (React + Tailwind CSS)** for real-time BLE monitoring dashboard

---

## 📂 Project Structure

```
esp32-scanner-system/
├── backend/        # Node.js + Express + SQLite API & Socket.IO
├── frontend/       # React Dashboard with TailwindCSS
├── .gitignore      # Ignoring node_modules, logs, env files
└── README.md       # Project Documentation
```

---

## 🚀 Features Implemented

- ✅ Real-time BLE device scanning
- ✅ iBeacon (Major, Minor, Tx Power) support
- ✅ SQLite database for storing historical readings
- ✅ Average RSSI per device display
- ✅ Device details page with history
- ✅ Tailwind-styled dashboard UI

---

## 🛠 Backend (Node.js)

- REST API: `/api/devices`, `/api/devices/:mac/readings`
- Receives ESP32 data via `/api/scan`
- Stores devices and readings in SQLite
- Emits real-time updates via Socket.IO

**Key Tables:**

- `devices` – MAC, name, type, protocol, timestamps
- `readings` – RSSI, major, minor, tx_power, timestamp

---

## 🎨 Frontend (React + Tailwind)

- Displays list of devices with:

  - Name, MAC, Protocol
  - **Average RSSI**
  - Major / Minor / Tx Power
  - Last Seen Time (auto-updated)

- Device details page with reading history

---

## 📡 ESP32 Role

- Scans BLE devices & iBeacons
- Sends JSON data via HTTP POST to backend `/api/scan`
- Sends: `mac`, `name`, `rssi`, `major`, `minor`, `tx_power`

---

## 🧪 Example JSON Payload (From ESP32)

```json
{
  "mac": "AA:BB:CC:DD:EE:FF",
  "name": "Beacon-1",
  "rssi": -68,
  "major": 100,
  "minor": 1,
  "tx_power": -59
}
```

---

## 📁 .gitignore Highlights

```
node_modules/
.env
logs/
build/
.DS_Store
```

---

## 🛳 Run Project Locally

### 1️⃣ Backend

```bash
cd backend
npm install
npm run dev
```

### 2️⃣ Frontend

```bash
cd frontend
npm install
npm run dev
```

---

## 📌 Next Improvements

- 🔍 Filters / Search for MAC
- 📊 RSSI Graph per device
- 📍 Indoor Positioning (Future)

---

**Science is Eleganto**
**Dr. M.**
