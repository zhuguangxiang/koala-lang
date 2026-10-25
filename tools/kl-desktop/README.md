# Koala Desktop Documentation

A native desktop application for Koala language documentation with integrated playground.

## Features

- 🖥️ **Native Desktop App** - Standalone window, no browser needed
- 📚 **Full Documentation** - Complete Koala language manual and API reference
- 🎮 **Integrated Playground** - Run Koala code directly in the app
- ⚡ **Fast & Lightweight** - Built with PyWebView, minimal resource usage
- 🎨 **Modern UI** - Dark theme with smooth animations

## Installation

### Prerequisites

- Python 3.7+
- Koala language installed and built

### Install Dependencies

```bash
cd tools/kl-desktop
pip3 install -r requirements.txt
```

**Note:** PyWebView requires system dependencies:
- **Linux:** `sudo apt install python3-gi python3-gi-cairo gir1.2-gtk-3.0 gir1.2-webkit2-4.0`
- **macOS:** No additional dependencies (uses built-in WebKit)
- **Windows:** No additional dependencies (uses EdgeChromium or MSHTML)

## Usage

### Quick Start

```bash
./tools/kl-desktop/kl-desktop.sh
```

Or manually:

```bash
# Generate HTML docs (if not already done)
python3 tools/kl-html-doc.py --output docs/html

# Start desktop app
python3 tools/kl-desktop/kl-desktop-app.py
```

### What Happens

1. Desktop app starts the documentation server automatically
2. Opens a native window with the documentation
3. You can browse docs, run code examples, and use the playground
4. Close the window to exit the app

## Features

### Sidebar Navigation
- Collapsible sidebar (click the arrow button)
- Search functionality
- Scroll position remembered across pages

### Code Playground
- Edit and run code examples directly
- Standalone playground on the home page
- Ctrl+Enter shortcut to run code
- Real-time output display

### Desktop-Specific
- Native window controls (minimize, maximize, close)
- Resizable window (minimum 800x600)
- System tray integration (platform-dependent)
- No browser overhead

## Configuration

The desktop app uses the same configuration as the web version:

- **Port:** 8080 (default, auto-detected)
- **KOALA_PATH:** Auto-detected from project root
- **HTML Docs:** `docs/html/` (auto-generated if missing)

## Troubleshooting

### PyWebView Installation Issues

**Linux:**
```bash
sudo apt install python3-gi python3-gi-cairo gir1.2-gtk-3.0 gir1.2-webkit2-4.0
pip3 install pywebview
```

**macOS:**
```bash
pip3 install pywebview  # Should work out of the box
```

**Windows:**
```bash
pip3 install pywebview  # Uses EdgeChromium if available
```

### Server Port Already in Use

If port 8080 is in use, the app will automatically try 8081, 8082, etc.

### Documentation Not Found

The app automatically generates HTML docs if they don't exist. To manually regenerate:

```bash
python3 tools/kl-html-doc.py --output docs/html
```

## Architecture

```
kl-desktop/
├── kl-desktop-app.py      # Main desktop application
├── kl-desktop.sh          # One-click launcher
├── requirements.txt       # Python dependencies
└── README.md             # This file

The app integrates:
- kl-playground-server.py  # Documentation server
- docs/html/              # Generated HTML documentation
- koala-playground.js     # Interactive code blocks
```

## Development

To modify the desktop app:

1. Edit `kl-desktop-app.py` for window settings
2. Modify `kl-playground-server.py` for server behavior
3. Update CSS in `kl-html-doc.py` for styling
4. Regenerate HTML: `python3 tools/kl-html-doc.py --output docs/html`

## License

Same as the Koala language project.

## Support

For issues and questions:
- GitHub: https://github.com/zhuguangxiang/koala-lang
- Documentation: Run the app and explore!
