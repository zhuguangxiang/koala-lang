#!/usr/bin/env python3
"""A minimal REST server: in-memory item store with JSON API."""

import json
from http.server import HTTPServer, BaseHTTPRequestHandler

DB = {}
NEXT_ID = 1


class RestHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self):
        if self.path == "/items":
            self._json_response(200, list(DB.values()))
        elif self.path.startswith("/items/"):
            item_id = self.path.split("/")[2]
            item = DB.get(item_id)
            if item:
                self._json_response(200, item)
            else:
                self._json_response(404, {"error": "not found"})
        else:
            self._json_response(404, {"error": "unknown path"})

    def do_POST(self):
        global NEXT_ID
        if self.path == "/items":
            body = self._read_body()
            item_id = str(NEXT_ID)
            NEXT_ID += 1
            item = {"id": item_id, "name": body.get("name", ""), "value": body.get("value", 0)}
            DB[item_id] = item
            self._json_response(201, item)
        else:
            self._json_response(404, {"error": "unknown path"})

    def do_PUT(self):
        if self.path.startswith("/items/"):
            item_id = self.path.split("/")[2]
            if item_id not in DB:
                self._json_response(404, {"error": "not found"})
                return
            body = self._read_body()
            DB[item_id].update(body)
            self._json_response(200, DB[item_id])
        else:
            self._json_response(404, {"error": "unknown path"})

    def do_DELETE(self):
        if self.path.startswith("/items/"):
            item_id = self.path.split("/")[2]
            if item_id in DB:
                del DB[item_id]
                self._json_response(200, {"deleted": item_id})
            else:
                self._json_response(404, {"error": "not found"})
        else:
            self._json_response(404, {"error": "unknown path"})

    def _read_body(self):
        length = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(length) if length > 0 else b"{}"
        return json.loads(raw)

    def _json_response(self, code, data):
        body = json.dumps(data).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        print(f"[server] {format % args}")


if __name__ == "__main__":
    server = HTTPServer(("127.0.0.1", 8080), RestHandler)
    print("REST server on http://127.0.0.1:8080  (Ctrl-C to stop)")
    server.serve_forever()
