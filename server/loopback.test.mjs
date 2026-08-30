import assert from "node:assert/strict";
import test from "node:test";

import { CONTROL_ROOM_HOST, createControlRoomServer } from "../dist/index.js";

test("control room binds only to the loopback host", async (t) => {
  const server = createControlRoomServer();
  t.after(() => server.close());

  await new Promise((resolve, reject) => {
    server.once("error", reject);
    server.listen(0, CONTROL_ROOM_HOST, resolve);
  });
  const address = server.address();
  assert.equal(typeof address, "object");
  assert.equal(address.address, "127.0.0.1");

  const response = await fetch(`http://127.0.0.1:${address.port}/`);
  assert.equal(response.status, 200);
});
