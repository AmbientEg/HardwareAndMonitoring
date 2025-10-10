// src/components/DeviceDetails.jsx
import React from "react";

export default function DeviceDetails({ device, lastReading }) {
  return (
    <div className="space-y-2">
      <h3 className="text-lg font-semibold">Device Info</h3>

      <div className="text-sm text-gray-400 space-y-1">
        <div>
          <span className="text-gray-300 font-medium">MAC:</span>{" "}
          <span className="text-gray-200 ml-2">{device?.mac ?? "—"}</span>
        </div>
        <div>
          <span className="text-gray-300 font-medium">Name:</span>{" "}
          <span className="text-gray-200 ml-2">
            {device?.name ?? "Unknown"}
          </span>
        </div>
        <div>
          <span className="text-gray-300 font-medium">Protocol:</span>{" "}
          <span className="text-gray-200 ml-2">
            {device?.protocol ?? "BLE"}
          </span>
        </div>
        <div>
          <span className="text-gray-300 font-medium">Last RSSI:</span>{" "}
          <span className="text-gray-200 ml-2">
            {lastReading?.rssi ?? "—"} dBm
          </span>
        </div>
        <div>
          <span className="text-gray-300 font-medium">Last seen:</span>{" "}
          <span className="text-gray-200 ml-2">
            {lastReading ? new Date(lastReading.ts).toLocaleString() : "—"}
          </span>
        </div>
        <div>
          <span className="text-gray-300 font-medium">Transmitter:</span>{" "}
          <span className="text-gray-200 ml-2">
            {lastReading?.tx_beacon_name ?? "—"}
          </span>
        </div>
      </div>
    </div>
  );
}
