#!/usr/bin/env python3
"""
Koala Desktop Documentation App
A native desktop application for Koala documentation with integrated playground.
"""

import os
import sys
import threading
import time
from pathlib import Path
from http.server import HTTPServer

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

# Import the playground server components
try:
    from importlib import util as importlib_util
    
    # Load the server module
    server_path = Path(__file__).parent.parent / "kl-playground-server.py"
    spec = importlib_util.spec_from_file_location("playground_server", server_path)
    playground_server = importlib_util.module_from_spec(spec)
    spec.loader.exec_module(playground_server)
    
    PlaygroundHandler = playground_server.PlaygroundHandler
except Exception as e:
    print(f"❌ Error loading playground server: {e}")
    print("   Make sure kl-playground-server.py exists in tools/")
    sys.exit(1)


class KoalaDesktopApp:
    """Desktop application for Koala documentation."""
    
    def __init__(self):
        self.server = None
        self.server_thread = None
        self.window = None
        self.port = 8080
        
    def start_server(self):
        """Start the documentation server in a background thread."""
        print(f"🚀 Starting Koala documentation server on port {self.port}...")
        
        # Set environment variables
        os.environ['KOALA_PATH'] = os.environ.get(
            'KOALA_PATH',
            str(Path(__file__).parent.parent.parent)
        )
        
        # Create and start server
        try:
            self.server = HTTPServer(("localhost", self.port), PlaygroundHandler)
            
            # Start server in thread
            self.server_thread = threading.Thread(
                target=self.server.serve_forever,
                daemon=True
            )
            self.server_thread.start()
            
            # Wait a bit for server to start
            time.sleep(0.5)
            print(f"✅ Server started at http://localhost:{self.port}")
            
        except OSError as e:
            if "Address already in use" in str(e):
                print(f"⚠️  Port {self.port} already in use, trying {self.port + 1}...")
                self.port += 1
                self.start_server()
            else:
                raise
        
    def create_window(self):
        """Create the desktop window."""
        try:
            import webview
        except ImportError:
            print("❌ PyWebView not installed!")
            print("   Install with: pip3 install pywebview")
            print("")
            print("   Linux also needs:")
            print("   sudo apt install python3-gi python3-gi-cairo gir1.2-gtk-3.0 gir1.2-webkit2-4.0")
            sys.exit(1)
        
        print("📱 Creating desktop window...")
        
        # Create window
        self.window = webview.create_window(
            title='Koala Documentation',
            url=f'http://localhost:{self.port}',
            width=1400,
            height=900,
            min_size=(800, 600),
            resizable=True,
            text_select=True,
        )
        
    def run(self):
        """Run the desktop application."""
        print("🐨 Koala Desktop Documentation")
        print("=" * 40)
        print("")
        
        # Start server
        self.start_server()
        
        # Create window
        self.create_window()
        
        # Start webview
        print("✨ Launching application...")
        print("")
        
        try:
            import webview
            webview.start()
        except KeyboardInterrupt:
            print("\n👋 Shutting down...")
        finally:
            if self.server:
                self.server.shutdown()
        
        print("👋 Application closed")


def main():
    """Main entry point."""
    app = KoalaDesktopApp()
    app.run()


if __name__ == '__main__':
    main()
