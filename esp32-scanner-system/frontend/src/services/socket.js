// src/services/socket.js
import { io } from "socket.io-client";

const SERVER = import.meta.env.VITE_API_URL || "http://192.168.1.122:3000";

export const socket = io(SERVER, {
  autoConnect: true,
  transports: ["websocket"],
  reconnectionAttempts: 5,
});

export default socket;
