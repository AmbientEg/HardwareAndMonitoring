// src/App.jsx
import React from "react";
import { Routes, Route, Link } from "react-router-dom";
import Home from "./pages/Home";
import DevicePage from "./pages/DevicePage";
import logo from "./assets/ambient.png"; // <-- Add your logo here

export default function App() {
  return (
    <div className="min-h-screen flex flex-col">
      <header className="bg-[#071022] border-b border-gray-800 text-white flex items-center justify-between px-6 py-4">
        <div className="flex items-center gap-4">
          {/* Logo Image Instead of A */}
          <img
            src={logo}
            alt="Logo"
            className="w-10 h-10 rounded-lg object-contain"
          />

          <div>
            <div className="header-title">Ambient BLE Dashboard</div>
            <div className="header-sub">Real-time BLE & iBeacon monitoring</div>
          </div>
        </div>

        <nav>
          <Link
            className="text-gray-300 hover:text-white transition font-medium"
            to="/"
          >
            Home
          </Link>
        </nav>
      </header>

      <main className="flex-1 container mx-auto px-6 py-6 max-w-7xl">
        <Routes>
          <Route path="/" element={<Home />} />
          <Route path="/device/:mac" element={<DevicePage />} />
        </Routes>
      </main>
    </div>
  );
}
