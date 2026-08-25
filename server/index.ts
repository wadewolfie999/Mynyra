import express from "express";
import { createServer } from "http";
import path from "path";
import { fileURLToPath, pathToFileURL } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

export const CONTROL_ROOM_HOST = "127.0.0.1";

export function createControlRoomServer() {
  const app = express();
  const server = createServer(app);

  // Serve static files from dist/public in production
  const staticPath =
    process.env.NODE_ENV === "production"
      ? path.resolve(__dirname, "public")
      : path.resolve(__dirname, "..", "dist", "public");

  app.use(express.static(staticPath));

  // Handle client-side routing - serve index.html for all routes
  app.get("*", (_req, res) => {
    res.sendFile(path.join(staticPath, "index.html"));
  });

  return server;
}

export async function startServer() {
  const server = createControlRoomServer();
  const port = Number(process.env.PORT || 3000);

  await new Promise<void>((resolve, reject) => {
    server.once("error", reject);
    server.listen(port, CONTROL_ROOM_HOST, () => {
      server.off("error", reject);
      resolve();
    });
  });
  console.log(`Server running on http://${CONTROL_ROOM_HOST}:${port}/`);
  return server;
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  startServer().catch((error) => {
    console.error(error);
    process.exitCode = 1;
  });
}
