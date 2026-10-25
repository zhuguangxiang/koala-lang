#!/bin/bash
# kl-playground.sh - One-click start for Koala Playground

cd "$(dirname "$0")/../.."

# Source environment if needed
[ -f env.sh ] && source env.sh

PORT=${PORT:-8080}

# Check if server is already running
if curl -s -o /dev/null -w '' "http://localhost:$PORT/" 2>/dev/null; then
    echo "🐨 Koala Playground already running at http://localhost:$PORT"
    echo "   Opening browser..."
    # Open browser
    if command -v xdg-open &>/dev/null; then
        xdg-open "http://localhost:$PORT" 2>/dev/null
    elif command -v open &>/dev/null; then
        open "http://localhost:$PORT" 2>/dev/null
    fi
    exit 0
fi

echo "🐨 Starting Koala Playground..."
echo ""

python3 tools/playground/kl-playground-server.py --port "$PORT" "$@"

