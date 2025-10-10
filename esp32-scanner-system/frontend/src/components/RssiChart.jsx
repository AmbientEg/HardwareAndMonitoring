// src/components/charts/RssiChart.jsx
import React, { useMemo } from "react";
import {
  ResponsiveContainer,
  LineChart,
  Line,
  XAxis,
  YAxis,
  Tooltip,
  CartesianGrid,
} from "recharts";

function niceTime(ts) {
  const d = new Date(ts);
  return `${d.getHours()}:${String(d.getMinutes()).padStart(2, "0")}:${String(
    d.getSeconds()
  ).padStart(2, "0")}`;
}

export default function RssiChart({ readings = [] }) {
  const data = useMemo(() => {
    return readings
      .map((r) => ({ ts: r.ts, time: niceTime(r.ts), rssi: Number(r.rssi) }))
      .filter((p) => !Number.isNaN(p.rssi))
      .slice(-200);
  }, [readings]);

  if (!data.length) {
    return (
      <div
        className="flex items-center justify-center text-gray-400"
        style={{ height: 320 }}
      >
        No data yet
      </div>
    );
  }

  return (
    <div style={{ height: 320 }}>
      <ResponsiveContainer width="100%" height="100%">
        <LineChart data={data}>
          <CartesianGrid
            strokeDasharray="3 3"
            stroke="rgba(255,255,255,0.03)"
          />
          <XAxis
            dataKey="time"
            minTickGap={30}
            tick={{ fontSize: 12, fill: "#9aa4b2" }}
          />
          <YAxis
            domain={["dataMin - 10", "dataMax + 5"]}
            tick={{ fontSize: 12, fill: "#9aa4b2" }}
          />
          <Tooltip wrapperStyle={{ background: "#021018", borderRadius: 6 }} />
          <Line
            type="monotone"
            dataKey="rssi"
            stroke="var(--accent)"
            strokeWidth={2}
            dot={false}
          />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
