// src/pages/DevicePage.jsx
import React, { useEffect, useState, useRef } from "react";
import { useParams, Link } from "react-router-dom";
import axios from "axios";
import RssiChart from "../components/RssiChart";
import DeviceDetails from "../components/DeviceDetails";
import { socket } from "../services/socket";

const API = import.meta.env.VITE_API_URL || "http://192.168.1.122:3000";

export default function DevicePage() {
  const { mac } = useParams();
  const decoded = decodeURIComponent(mac);
  const [device, setDevice] = useState(null);
  const [readings, setReadings] = useState([]);
  const mountedRef = useRef(true);

  useEffect(() => {
    mountedRef.current = true;
    axios
      .get(`${API}/api/devices`)
      .then((res) => {
        if (!mountedRef.current) return;
        const found = res.data.find((d) => d.mac === decoded);
        if (found) setDevice(found);
      })
      .catch(console.error);

    axios
      .get(`${API}/api/devices/${encodeURIComponent(decoded)}/readings`)
      .then((res) => {
        if (mountedRef.current) setReadings(res.data || []);
      })
      .catch((err) => {
        if (err && err.response && err.response.status !== 404)
          console.error(err);
      });

    const onBatch = (batch) => {
      batch.forEach((it) => {
        if (it && it.mac === decoded) {
          const r = {
            rssi: it.rssi ?? null,
            ts: it.ts || Date.now(),
            major: it.major ?? null,
            minor: it.minor ?? null,
            tx_power: it.tx_power ?? null,
            tx_beacon_name: it.tx_beacon_name ?? null,
            adv_data: it.adv_data ?? {},
          };
          setReadings((prev) => {
            const p = prev.slice();
            p.push(r);
            if (p.length > 2000) p.shift();
            return p;
          });
        }
      });
    };

    socket.on("scan:batch", onBatch);

    return () => {
      mountedRef.current = false;
      socket.off("scan:batch", onBatch);
    };
  }, [decoded]);

  const last = readings[readings.length - 1];

  return (
    <div className="space-y-6">
      <div className="flex items-start justify-between">
        <div>
          <h2 className="text-2xl font-semibold">{device?.name || decoded}</h2>
          <div className="text-sm text-gray-400">{decoded}</div>
        </div>
        <Link to="/" className="text-[var(--accent)]">
          ← Back
        </Link>
      </div>

      {/* Chart full width */}
      <div className="card w-full">
        <RssiChart readings={readings} />
      </div>

      {/* Device details full width */}
      <div className="card w-full">
        <DeviceDetails device={device} lastReading={last} />
      </div>

      {/* Readings table (scrollable) */}
      <div className="card w-full readings-list max-h-[420px] overflow-y-auto">
        <table className="min-w-full">
          <thead className="sticky top-0 bg-[#06111a]">
            <tr>
              <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400">
                Time
              </th>
              <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400">
                RSSI
              </th>
              <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400">
                Beacon
              </th>
              <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400">
                Major
              </th>
              <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400">
                Minor
              </th>
              <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400">
                Tx
              </th>
            </tr>
          </thead>
          <tbody className="divide-y divide-gray-800">
            {readings
              .slice()
              .reverse()
              .map((r, i) => (
                <tr key={i} className="hover:bg-gray-800">
                  <td className="px-4 py-2 text-sm">
                    {new Date(r.ts).toLocaleString()}
                  </td>
                  <td className="px-4 py-2 text-sm">{r.rssi ?? "—"}</td>
                  <td className="px-4 py-2 text-sm">
                    {r.tx_beacon_name ?? "—"}
                  </td>
                  <td className="px-4 py-2 text-sm">{r.major ?? "—"}</td>
                  <td className="px-4 py-2 text-sm">{r.minor ?? "—"}</td>
                  <td className="px-4 py-2 text-sm">{r.tx_power ?? "—"}</td>
                </tr>
              ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}
