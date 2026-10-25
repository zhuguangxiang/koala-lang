#!/bin/bash
# Koala Desktop - Install dependencies

set -e

echo "🐨 Koala Desktop - Installing Dependencies"
echo "==========================================="
echo ""

# Detect OS
OS="$(uname -s)"

# Install system dependencies for Linux
if [[ "$OS" == "Linux" ]]; then
    echo "📦 Installing system dependencies for Linux..."
    echo ""
    
    if command -v apt-get &> /dev/null; then
        sudo apt-get update -qq || true  # 忽略源错误
        
        # 检测 WebKit 版本（4.1 或 4.0）
        if apt-cache show gir1.2-webkit2-4.1 &>/dev/null; then
            WEBKIT_PKG="gir1.2-webkit2-4.1"
            echo "✓ 检测到 WebKit2 4.1"
        elif apt-cache show gir1.2-webkit2-4.0 &>/dev/null; then
            WEBKIT_PKG="gir1.2-webkit2-4.0"
            echo "✓ 检测到 WebKit2 4.0"
        else
            echo "⚠️  未找到 WebKit2 包，请手动安装"
            echo "   尝试: sudo apt install gir1.2-webkit2-4.1"
            exit 1
        fi
        
        sudo apt-get install -y -qq \
            python3-gi \
            python3-gi-cairo \
            gir1.2-gtk-3.0 \
            $WEBKIT_PKG
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y \
            python3-gobject \
            gtk3 \
            webkit2gtk3
    elif command -v pacman &> /dev/null; then
        sudo pacman -S --noconfirm \
            python-gobject \
            gtk3 \
            webkit2gtk
    else
        echo "⚠️  Unknown package manager. Please install manually:"
        echo "   - python3-gi"
        echo "   - gir1.2-gtk-3.0"
        echo "   - gir1.2-webkit2-4.1 (or 4.0)"
    fi
    echo ""
fi

# Install Python dependencies
echo "📦 Installing Python dependencies..."
pip3 install pywebview

echo ""
echo "✅ Installation complete!"
echo ""
echo "Run the desktop app with:"
echo "   ./tools/kl-desktop/kl-desktop.sh"
