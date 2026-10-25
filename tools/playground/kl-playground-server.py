#!/usr/bin/env python3
"""kl-playground-server.py - Local HTTP server for running Koala code from browser.

Usage:
    python3 tools/playground/kl-playground-server.py [--port 8080]

This server:
1. Serves static HTML documentation from docs/html/
2. Executes Koala code via POST /run endpoint
3. Opens browser automatically on startup

Visit http://localhost:8080 to see the documentation with runnable code examples.
"""

import argparse
import json
import mimetypes
import os
import subprocess
import sys
import tempfile
import webbrowser
from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path
from threading import Timer


class PlaygroundHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        """Serve static files from tools/playground/html/"""
        # Find project root and docs directory
        script_dir = Path(__file__).parent
        project_root = script_dir.parent.parent
        docs_dir = project_root / "tools" / "playground" / "html"
        
        # Default to index.html
        path = self.path
        if path == "/":
            path = "/index.html"
        
        # Remove query string
        path = path.split("?")[0]
        
        # Security: prevent directory traversal
        if ".." in path or path.startswith("//"):
            self.send_error(403)
            return
        
        # Build file path
        file_path = docs_dir / path.lstrip("/")
        
        # Check if file exists
        if not file_path.exists() or not file_path.is_file():
            self.send_error(404)
            return
        
        # Serve the file
        try:
            content_type, _ = mimetypes.guess_type(str(file_path))
            if content_type is None:
                content_type = "application/octet-stream"
            
            with open(file_path, "rb") as f:
                content = f.read()
            
            self.send_response(200)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(content)))
            self.end_headers()
            self.wfile.write(content)
        except Exception as e:
            self.send_error(500, str(e))
    
    def do_POST(self):
        if self.path != "/run":
            self.send_error(404)
            return

        content_length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(content_length)

        try:
            data = json.loads(body)
            code = data.get("code", "")
        except json.JSONDecodeError:
            self.send_error(400, "Invalid JSON")
            return

        # Find project root
        script_dir = Path(__file__).parent
        project_root = script_dir.parent.parent
        koala_bin = project_root / "build" / "release" / "bin" / "koala"

        if not koala_bin.exists():
            self.send_response(500)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"error": "koala binary not found"}).encode())
            return

        # Set up environment
        import os
        env = os.environ.copy()
        env["KOALA_PATH"] = f"{project_root}/libs/std/dist/:{project_root}/libs/koala/dist/:{project_root}/test/:{project_root}/"

        # Write code to temp file
        with tempfile.NamedTemporaryFile(mode="w", suffix=".kl", delete=False) as f:
            f.write(code)
            temp_file = f.name

        try:
            # Execute koala
            result = subprocess.run(
                [str(koala_bin), temp_file],
                capture_output=True,
                text=True,
                timeout=5,
                env=env,
                cwd=str(project_root),
            )

            output = result.stdout
            if result.returncode != 0:
                output = result.stderr

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps({"output": output}).encode())

        except subprocess.TimeoutExpired:
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps({"output": "Error: execution timeout (5s)"}).encode())

        finally:
            Path(temp_file).unlink()

    def do_OPTIONS(self):
        """Handle CORS preflight requests."""
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def log_message(self, format, *args):
        """Suppress default logging."""
        pass


def main():
    ap = argparse.ArgumentParser(description="Koala Playground Server")
    ap.add_argument("--port", type=int, default=8080, help="Port to listen on (default: 8080)")
    ap.add_argument("--no-browser", action="store_true", help="Don't open browser automatically")
    args = ap.parse_args()

    server = HTTPServer(("localhost", args.port), PlaygroundHandler)
    url = f"http://localhost:{args.port}"
    
    print(f"🐨 Koala Playground Server")
    print(f"   URL: {url}")
    print(f"   Press Ctrl+C to stop\n")
    
    # Open browser automatically after a short delay
    if not args.no_browser:
        Timer(0.5, lambda: webbrowser.open(url)).start()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n🛑 Shutting down...")
        server.server_close()


if __name__ == "__main__":
    main()
