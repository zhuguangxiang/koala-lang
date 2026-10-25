#!/bin/bash
# Koala Desktop Documentation - One-click launcher

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

echo "🐨 Koala Desktop Documentation"
echo "================================"
echo ""

# Check if pywebview is installed
if ! python3 -c "import webview" 2>/dev/null; then
    echo "⚠️  PyWebView not found. Installing..."
    pip3 install pywebview
    echo ""
fi

# Set environment variables
export KOALA_PATH="$PROJECT_ROOT"

# Generate HTML docs if not exists
if [ ! -f "$PROJECT_ROOT/docs/html/index.html" ]; then
    echo "📄 Generating HTML documentation..."
    python3 "$PROJECT_ROOT/tools/kl-html-doc.py" --output "$PROJECT_ROOT/docs/html"
    echo ""
fi

# Start desktop application
echo "🚀 Starting desktop application..."
python3 "$SCRIPT_DIR/kl-desktop-app.py"
