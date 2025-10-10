// src/pages/Home.jsx
import React, { useEffect, useState } from "react";
import axios from "axios";
import DeviceTable from "../components/DeviceTable";
import { socket } from "../services/socket";

const API = import.meta.env.VITE_API_URL || "http://192.168.1.122:3000";

export default function Home() {
  const [devices, setDevices] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    let mounted = true;
    axios
      .get(`${API}/api/devices`)
      .then((res) => {
        if (mounted) setDevices(res.data || []);
      })
      .catch((err) => console.error("GET /devices error", err))
      .finally(() => {
        if (mounted) setLoading(false);
      });

    const onBatch = (batch) => {
      setDevices((prev) => {
        const map = new Map(prev.map((d) => [d.mac, d]));
        batch.forEach((it) => {
          if (!it || !it.mac) return;
          const existing = map.get(it.mac) || {};
          map.set(it.mac, {
            ...existing,
            ...it,
            last_seen: it.ts || Date.now(),
          });
        });
        return Array.from(map.values()).sort(
          (a, b) => (b.last_seen || 0) - (a.last_seen || 0)
        );
      });
    };

    socket.on("scan:batch", onBatch);

    return () => {
      mounted = false;
      socket.off("scan:batch", onBatch);
    };
  }, []);

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between">
        <div>
          <h2 className="text-2xl font-semibold">Devices</h2>
          <p className="text-sm text-gray-400">
            Live devices seen by your ESP32 scanners
          </p>
        </div>
        <div className="text-sm text-gray-300">{devices.length} devices</div>
      </div>

      <div className="card">
        {loading ? (
          <div className="py-8 text-gray-400">Loading…</div>
        ) : (
          <DeviceTable devices={devices} />
        )}
      </div>
    </div>
  );
}
