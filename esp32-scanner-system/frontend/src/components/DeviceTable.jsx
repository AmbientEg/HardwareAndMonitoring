// src/components/DeviceTable.jsx
import React from "react";
import { useNavigate } from "react-router-dom";

function timeAgo(ms) {
  if (!ms) return "—";
  const d = Date.now() - Number(ms);
  if (d < 5000) return "just now";
  const s = Math.floor(d / 1000);
  if (s < 60) return `${s}s`;
  const m = Math.floor(s / 60);
  if (m < 60) return `${m}m`;
  const h = Math.floor(m / 60);
  return `${h}h`;
}

export default function DeviceTable({ devices = [] }) {
  const nav = useNavigate();

  return (
    <div className="overflow-auto">
      <table className="min-w-full divide-y divide-gray-800">
        <thead className="sticky-header">
          <tr className="bg-[#06111a]">
            <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400 uppercase tracking-wider">
              Name
            </th>
            <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400 uppercase tracking-wider">
              MAC
            </th>
            <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400 uppercase tracking-wider">
              Avg RSSI
            </th>
            <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400 uppercase tracking-wider">
              Protocol
            </th>
            <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400 uppercase tracking-wider">
              Major
            </th>
            <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400 uppercase tracking-wider">
              Minor
            </th>
            <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400 uppercase tracking-wider">
              Tx Power
            </th>
            <th className="px-4 py-3 text-left text-xs font-semibold text-gray-400 uppercase tracking-wider">
              Last seen
            </th>
          </tr>
        </thead>
        <tbody className="divide-y divide-gray-800">
          {devices.map((d, i) => (
            <tr
              key={d.mac ?? i}
              onClick={() => nav(`/device/${encodeURIComponent(d.mac)}`)}
              className="hover:bg-gray-800 cursor-pointer transition"
            >
              <td className="px-4 py-3">
                <div className="font-semibold text-sm">
                  {d.name || "Unknown"}
                </div>
                <div className="text-xs text-gray-400">{d.type || "BLE"}</div>
              </td>
              <td className="px-4 py-3 text-sm text-gray-300">{d.mac}</td>
              <td className="px-4 py-3 text-sm">
                {d.avg_rssi !== undefined && d.avg_rssi !== null
                  ? `${d.avg_rssi} dBm`
                  : "—"}
              </td>
              <td className="px-4 py-3 text-sm text-gray-300">
                {d.protocol || "BLE"}
              </td>
              <td className="px-4 py-3 text-sm">{d.major ?? "—"}</td>
              <td className="px-4 py-3 text-sm">{d.minor ?? "—"}</td>
              <td className="px-4 py-3 text-sm">{d.tx_power ?? "—"}</td>
              <td className="px-4 py-3 text-sm text-gray-400">
                {timeAgo(d.last_seen)}
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
