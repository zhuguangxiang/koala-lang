#!/usr/bin/env python3
"""kl-html-doc.py - Generate Rust-style HTML API documentation for Koala.

Usage:
    python3 tools/kl-html-doc.py                              # generate all workspaces
    python3 tools/kl-html-doc.py --workspace std              # only std workspace
    python3 tools/kl-html-doc.py --output tools/playground/html # custom output dir
"""

import argparse
import html as html_mod
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple

# Import parsing logic from kl-doc.py
sys.path.insert(0, str(Path(__file__).parent))
kl_doc_mod = __import__("kl-doc")
parse_source = kl_doc_mod.parse_source
DocItem = kl_doc_mod.DocItem
first_paragraph = kl_doc_mod.first_paragraph
bases_of = kl_doc_mod.bases_of


def esc(text: str) -> str:
    """Escape HTML special characters."""
    return html_mod.escape(text, quote=False)


def render_md(text: str) -> str:
    """Minimal Markdown to HTML for doc comments."""
    if not text:
        return ""
    lines = text.split("\n")
    out: list[str] = []
    in_code = False
    code_lang = ""
    code_buf: list[str] = []
    in_indented_code = False
    indented_code_buf: list[str] = []

    for line in lines:
        if line.startswith("```"):
            # End indented code block if any
            if in_indented_code:
                code = "\n".join(indented_code_buf)
                out.append(
                    f'<pre><code class="language-koala">{esc(code)}</code></pre>'
                )
                in_indented_code = False
                indented_code_buf = []
            
            if in_code:
                code = "\n".join(code_buf)
                # Change language identifier from kl to koala
                if code_lang == "kl":
                    code_lang = "koala"
                out.append(
                    f'<pre><code class="language-{esc(code_lang)}">{esc(code)}</code></pre>'
                )
                in_code = False
                code_buf = []
            else:
                in_code = True
                code_lang = line[3:].strip() or "text"
        elif in_code:
            code_buf.append(line)
        elif line.startswith("    ") or line.startswith("\t"):
            # Indented code block (4 spaces or tab)
            if not in_indented_code:
                in_indented_code = True
                indented_code_buf = []
            # Remove the leading 4 spaces or tab
            if line.startswith("    "):
                indented_code_buf.append(line[4:])
            else:
                indented_code_buf.append(line[1:])
        else:
            # End indented code block if any
            if in_indented_code:
                code = "\n".join(indented_code_buf)
                out.append(
                    f'<pre><code class="language-koala">{esc(code)}</code></pre>'
                )
                in_indented_code = False
                indented_code_buf = []
            
            line = re.sub(r"`([^`]+)`", r"<code>\1</code>", line)
            line = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r'<a href="\2">\1</a>', line)
            line = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", line)
            if line.strip():
                out.append(f"<p>{line}</p>")
            else:
                out.append("")
    
    # End any remaining code blocks
    if in_indented_code:
        code = "\n".join(indented_code_buf)
        out.append(
            f'<pre><code class="language-koala">{esc(code)}</code></pre>'
        )
    if in_code:
        code = "\n".join(code_buf)
        if code_lang == "kl":
            code_lang = "koala"
        out.append(
            f'<pre><code class="language-{esc(code_lang)}">{esc(code)}</code></pre>'
        )
    
    return "\n".join(out)


# ============================================================================
# HTML Templates
# ============================================================================

CSS = """
:root {
    --sidebar-width: 280px;
    --content-max-width: 900px;
    --bg: #0d1117;
    --sidebar-bg: #161b22;
    --border: #30363d;
    --text: #c9d1d9;
    --text-light: #8b949e;
    --link: #58a6ff;
    --code-bg: #1f2428;
    --heading: #f0f6fc;
    --badge-bg: #30363d;
    --badge-text: #c9d1d9;
    --accent: #238636;
    --accent-hover: #2ea043;
}

* { box-sizing: border-box; margin: 0; padding: 0; }

/* Modern Scrollbar */
::-webkit-scrollbar {
    width: 8px;
    height: 8px;
}

::-webkit-scrollbar-track {
    background: transparent;
}

::-webkit-scrollbar-thumb {
    background: #30363d;
    border-radius: 4px;
}

::-webkit-scrollbar-thumb:hover {
    background: #484f58;
}

::-webkit-scrollbar-thumb:active {
    background: #6e7681;
}

/* Firefox */
* {
    scrollbar-width: thin;
    scrollbar-color: #30363d transparent;
}

body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
    font-size: 17px;
    line-height: 1.7;
    color: var(--text);
    background: var(--bg);
    display: flex;
    min-height: 100vh;
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
}

#sidebar {
    width: var(--sidebar-width);
    background: var(--sidebar-bg);
    border-right: 1px solid var(--border);
    padding: 20px 0;
    position: fixed;
    top: 0;
    left: 0;
    bottom: 0;
    overflow-y: auto;
    backdrop-filter: blur(10px);
    z-index: 100;
    scrollbar-width: none; /* Firefox */
    -ms-overflow-style: none; /* IE and Edge */
}

#sidebar::-webkit-scrollbar {
    display: none; /* Chrome, Safari, Opera */
}

#sidebar h2 {
    font-size: 40px;
    font-weight: 700;
    padding: 0 20px 15px;
    border-bottom: 1px solid var(--border);
    margin-bottom: 15px;
    color: var(--text-light);
}

#sidebar .workspace { margin-bottom: 20px; }

#sidebar .workspace-title {
    font-size: 13px;
    font-weight: 600;
    padding: 8px 20px;
    color: var(--text-light);
    text-transform: uppercase;
    letter-spacing: 0.8px;
    cursor: pointer;
    display: flex;
    justify-content: space-between;
    align-items: center;
    user-select: none;
}

#sidebar .workspace-title:hover {
    color: var(--text);
}

#sidebar .ws-arrow {
    font-size: 10px;
    transition: transform 0.2s;
}

#sidebar .workspace-title.collapsed .ws-arrow {
    transform: rotate(-90deg);
}

#sidebar .workspace-items.hidden {
    display: none;
}

#sidebar .package { margin-bottom: 2px; }

#sidebar .package > a {
    display: block;
    padding: 8px 20px 8px 30px;
    color: var(--text);
    text-decoration: none;
    font-size: 16px;
    transition: all 0.2s ease;
    border-left: 2px solid transparent;
}

#sidebar .package > a:hover {
    background: rgba(88, 166, 255, 0.08);
    border-left-color: var(--link);
}

#sidebar .package > a.active {
    background: rgba(88, 166, 255, 0.15);
    color: var(--link);
    border-left-color: var(--link);
    font-weight: 500;
}

#search-box {
    width: calc(100% - 40px);
    margin: 0 20px 15px;
    padding: 10px 14px;
    border: 1px solid var(--border);
    border-radius: 6px;
    font-size: 15px;
    background: var(--code-bg);
    color: var(--text);
    transition: border-color 0.2s ease, box-shadow 0.2s ease;
}

#search-box:focus {
    outline: none;
    border-color: var(--link);
    box-shadow: 0 0 0 3px rgba(88, 166, 255, 0.1);
}

#search-box::placeholder {
    color: var(--text-light);
}

#content {
    margin-left: var(--sidebar-width);
    margin-right: auto;
    flex: 1;
    padding: 40px;
    max-width: var(--content-max-width);
}

h1 {
    font-size: 40px;
    font-weight: 700;
    color: var(--text-light);
    margin-bottom: 10px;
    padding-bottom: 12px;
    border-bottom: 2px solid var(--border);
    letter-spacing: -0.5px;
}

h2 {
    font-size: 28px;
    font-weight: 600;
    color: var(--text-light);
    margin-top: 40px;
    margin-bottom: 15px;
    padding-bottom: 8px;
    border-bottom: 1px solid var(--border);
    letter-spacing: -0.3px;
}

h3 {
    font-size: 22px;
    font-weight: 600;
    color: var(--text-light);
    margin-top: 30px;
    margin-bottom: 10px;
}

h4.section-title {
    font-size: 20px;
    font-weight: 600;
    color: var(--text-light);
    margin-top: 25px;
    margin-bottom: 12px;
}

.item {
    margin-bottom: 40px;
    padding-bottom: 40px;
    border-bottom: 1px solid var(--border);
    scroll-margin-top: 20px;
}

.member-item {
    margin-bottom: 30px;
    scroll-margin-top: 20px;
}

.collapsible-section {
    margin-top: 20px;
}

.collapse-toggle {
    background: none;
    border: none;
    color: var(--text-light);
    font-size: 18px;
    font-weight: 600;
    cursor: pointer;
    padding: 8px 0;
    display: flex;
    align-items: center;
    gap: 8px;
    transition: color 0.2s;
}

.collapse-toggle:hover {
    color: var(--text);
}

.collapse-toggle .arrow {
    font-size: 12px;
    transition: transform 0.2s;
}

.collapse-toggle.collapsed .arrow {
    transform: rotate(-90deg);
}

.collapsible-content {
    margin-top: 15px;
    padding-left: 10px;
    border-left: 2px solid var(--border);
}

.collapsible-content.hidden {
    display: none;
}

.item-header {
    display: flex;
    align-items: center;
    gap: 10px;
    margin-bottom: 10px;
}

.item-header h3 { margin: 0; font-family: "SF Mono", Monaco, monospace; }

.badge {
    display: inline-block;
    padding: 2px 8px;
    border-radius: 3px;
    font-size: 12px;
    font-weight: 600;
    background: var(--badge-bg);
    color: var(--badge-text);
}

.badge.trait { background: #1a3a4a; color: #5bc0de; }
.badge.class { background: #3a1a4a; color: #c77dff; }
.badge.func { background: #1a3a1a; color: #7ddf7d; }
.badge.const { background: #4a3a1a; color: #ffb347; }

.signature {
    background: var(--code-bg);
    padding: 14px 18px;
    border-radius: 8px;
    border-left: 3px solid var(--link);
    margin-bottom: 15px;
    overflow-x: auto;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
}

.signature code {
    font-family: "SF Mono", Monaco, monospace;
    font-size: 15px;
    line-height: 1.5;
}

.bases {
    font-size: 14px;
    color: var(--text-light);
    margin-bottom: 15px;
}

.bases strong { color: var(--text); }

.doc-content { margin-bottom: 15px; }
.doc-content p { margin-bottom: 12px; line-height: 1.7; }

.doc-content code {
    background: var(--code-bg);
    padding: 2px 8px;
    border-radius: 4px;
    font-size: 14px;
    border: 1px solid var(--border);
}

.doc-content pre {
    background: var(--code-bg);
    padding: 16px 20px;
    border-radius: 8px;
    overflow-x: auto;
    margin: 12px 0;
    border: 1px solid var(--border);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
}

.doc-content pre code { background: none; padding: 0; border: none; font-size: 14px; }

.member {
    margin-left: 20px;
    margin-bottom: 20px;
    padding-left: 15px;
    border-left: 2px solid var(--border);
}

.member-sig {
    font-family: "SF Mono", Monaco, monospace;
    font-size: 15px;
    font-weight: 600;
    color: var(--heading);
    margin-bottom: 5px;
}

.member-tag {
    font-size: 12px;
    color: var(--text-light);
    font-style: italic;
}

table {
    width: 100%;
    border-collapse: collapse;
    margin: 15px 0;
    border-radius: 8px;
    overflow: hidden;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
}

th, td {
    text-align: left;
    padding: 12px 16px;
    border-bottom: 1px solid var(--border);
    font-size: 15px;
}

th {
    background: var(--sidebar-bg);
    font-weight: 600;
    color: var(--text-light);
}

td a { color: var(--link); text-decoration: none; transition: color 0.2s; }
td a:hover { color: #79c0ff; }

.summary { color: var(--text-light); font-size: 15px; }
a { color: var(--link); text-decoration: none; transition: color 0.2s; }
a:hover { color: #79c0ff; }

.manual-toc {
    background: var(--sidebar-bg);
    padding: 24px;
    border-radius: 12px;
    margin-bottom: 30px;
    border: 1px solid var(--border);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
}

.manual-toc h2 {
    margin-top: 0;
    font-size: 20px;
    border-bottom: none;
    padding-bottom: 0;
}

.manual-toc ul {
    list-style: none;
    padding-left: 0;
}

.manual-toc ul ul {
    padding-left: 20px;
    margin-top: 5px;
}

.manual-toc li {
    margin-bottom: 8px;
}

.manual-toc a {
    color: var(--text);
    text-decoration: none;
    transition: all 0.2s;
    padding: 4px 8px;
    border-radius: 4px;
    display: inline-block;
    font-size: 16px;
}

.manual-toc a:hover {
    color: var(--link);
    background: rgba(88, 166, 255, 0.08);
}

.manual-content {
    margin-bottom: 20px;
}

.manual-content pre {
    background: var(--code-bg);
    padding: 16px 20px;
    border-radius: 8px;
    overflow-x: auto;
    margin: 15px 0;
    border: 1px solid var(--border);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
}

.manual-content code {
    background: var(--code-bg);
    padding: 2px 8px;
    border-radius: 4px;
    font-size: 14px;
    border: 1px solid var(--border);
}

.manual-content pre code {
    background: none;
    padding: 0;
    border: none;
    font-size: 14px;
}

.doc-cards {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 24px;
    margin: 30px 0;
}

.doc-card {
    background: var(--sidebar-bg);
    border: 1px solid var(--border);
    border-radius: 12px;
    padding: 28px;
    transition: all 0.3s ease;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
}

.doc-card:hover {
    border-color: var(--link);
    transform: translateY(-4px);
    box-shadow: 0 8px 24px rgba(88, 166, 255, 0.15);
}

.doc-card h3 {
    margin: 0 0 12px;
    font-size: 22px;
    color: var(--heading);
    font-weight: 600;
}

.doc-card p {
    color: var(--text-light);
    margin-bottom: 16px;
    line-height: 1.7;
}

.doc-card-link {
    display: inline-block;
    color: var(--link);
    text-decoration: none;
    font-weight: 600;
    transition: all 0.2s;
    padding: 6px 12px;
    border-radius: 6px;
    background: rgba(88, 166, 255, 0.1);
}

.doc-card-link:hover {
    color: #79c0ff;
    background: rgba(88, 166, 255, 0.2);
}

/* Playground styles */
.playground-section {
    background: #1a1a2e;
    border: 1px solid #2d2d44;
    border-radius: 8px;
    padding: 24px;
    margin: 24px 0;
}

.playground-section h2 {
    margin-top: 0;
    color: #7bb8ff;
}

.playground-container {
    display: flex;
    flex-direction: column;
    gap: 12px;
}

.playground-editor {
    position: relative;
}

.playground-editor textarea {
    width: 100%;
    min-height: 200px;
    padding: 16px;
    background: #0d0d1a;
    color: #e0e0e0;
    border: 1px solid #2d2d44;
    border-radius: 6px;
    font-family: 'SF Mono', Monaco, Consolas, monospace;
    font-size: 14px;
    line-height: 1.5;
    resize: vertical;
    tab-size: 4;
}

.playground-editor textarea:focus {
    outline: none;
    border-color: #7bb8ff;
}

.playground-controls {
    display: flex;
    gap: 8px;
}

.playground-btn {
    padding: 8px 20px;
    background: #2d5a3d;
    color: #a0e0a0;
    border: 1px solid #3a7a4a;
    border-radius: 4px;
    font-size: 14px;
    cursor: pointer;
    transition: background 0.2s;
}

.playground-btn:hover {
    background: #3a7a4a;
}

.playground-btn:disabled {
    opacity: 0.6;
    cursor: not-allowed;
}

.playground-btn.secondary {
    background: #2d2d44;
    color: #a0a0c0;
    border-color: #3d3d5c;
}

.playground-btn.secondary:hover {
    background: #3d3d5c;
}

.playground-result {
    padding: 12px 16px;
    background: #1a2a1a;
    border: 1px solid #2d5a3d;
    border-radius: 6px;
    font-family: 'SF Mono', Monaco, Consolas, monospace;
    font-size: 13px;
    color: #a0e0a0;
    white-space: pre-wrap;
    word-break: break-word;
    min-height: 60px;
}

.playground-result.error {
    background: #2a1a1a;
    border-color: #5a2d2d;
    color: #e0a0a0;
}

.playground-result .result-label {
    display: block;
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    opacity: 0.7;
    margin-bottom: 8px;
}

@media (max-width: 768px) {
    #sidebar {
        width: 100%;
        position: static;
        border-right: none;
        border-bottom: 1px solid var(--border);
    }
    #content { margin-left: 0; padding: 20px; }
    body { flex-direction: column; }
}

.github-link {
    display: block;
    margin: 0 20px 15px;
    padding: 8px 12px;
    background: var(--code-bg);
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--text);
    text-decoration: none;
    font-size: 13px;
    text-align: center;
    transition: border-color 0.2s, background 0.2s;
}

.github-link:hover {
    border-color: var(--link);
    background: rgba(91, 163, 245, 0.1);
}

.github-link::before {
    content: "🐙 ";
}

/* Table of Contents (Right Sidebar) */
#toc {
    position: fixed;
    right: 30px;
    top: 80px;
    width: 280px;
    max-height: calc(100vh - 100px);
    overflow-y: auto;
    padding: 20px;
    background: var(--sidebar-bg);
    border-radius: 8px;
    border: 1px solid var(--border);
    font-size: 14px;
    scrollbar-width: none; /* Firefox */
    -ms-overflow-style: none; /* IE and Edge */
}

#toc::-webkit-scrollbar {
    display: none; /* Chrome, Safari, Opera */
}

#toc-title {
    font-weight: 600;
    color: var(--text-light);
    margin-bottom: 12px;
    font-size: 16px;
}

#toc ul {
    list-style: none;
    padding: 0;
    margin: 0;
}

#toc li {
    margin: 8px 0;
}

#toc a {
    color: var(--text-light);
    text-decoration: none;
    display: block;
    padding: 6px 12px;
    border-radius: 4px;
    transition: all 0.2s;
    line-height: 1.5;
}

#toc a:hover {
    background: var(--code-bg);
    color: var(--link);
}

#toc a.active {
    background: rgba(88, 166, 255, 0.1);
    color: var(--link);
    border-left: 2px solid var(--link);
}

#toc ul ul {
    margin-left: 16px;
    margin-top: 6px;
}

#toc ul ul a {
    font-size: 13px;
    padding: 4px 12px;
}

#toc li.toc-h3 a {
    padding-left: 24px;
    font-size: 13px;
}

/* Home button at top */
#toc-home {
    display: block;
    padding: 10px 16px;
    margin-bottom: 12px;
    background: var(--code-bg);
    border: 1px solid var(--border);
    border-radius: 6px;
    color: var(--text);
    text-decoration: none;
    font-size: 14px;
    font-weight: 500;
    text-align: center;
    transition: all 0.2s;
}

#toc-home:hover {
    background: var(--link);
    color: #fff;
    border-color: var(--link);
}

/* Back to top button */
#back-to-top {
    display: none;
    width: 100%;
    padding: 10px 16px;
    margin-top: 12px;
    background: var(--code-bg);
    border: 1px solid var(--border);
    border-radius: 6px;
    color: var(--text);
    font-size: 14px;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.2s;
}

#back-to-top:hover {
    background: var(--link);
    color: #fff;
    border-color: var(--link);
}

#back-to-top.visible {
    display: block;
}

/* TOC always visible */
#content {
    margin-right: 320px;
}

/* Scrollbar for TOC */
#toc::-webkit-scrollbar {
    width: 6px;
}

#toc::-webkit-scrollbar-track {
    background: transparent;
}

#toc::-webkit-scrollbar-thumb {
    background: var(--border);
    border-radius: 3px;
}

#toc::-webkit-scrollbar-thumb:hover {
    background: var(--text-light);
}
"""

JS = """
document.addEventListener('DOMContentLoaded', function() {
    const searchBox = document.getElementById('search-box');
    const sidebar = document.getElementById('sidebar');
    const content = document.getElementById('content');
    
    // Restore sidebar scroll position
    if (sidebar) {
        const savedScroll = localStorage.getItem('sidebar-scroll');
        if (savedScroll) {
            sidebar.scrollTop = parseInt(savedScroll, 10);
        }
        
        // Save sidebar scroll position on scroll
        sidebar.addEventListener('scroll', function() {
            localStorage.setItem('sidebar-scroll', sidebar.scrollTop.toString());
        });
        
        // Save scroll position before navigating
        sidebar.querySelectorAll('a').forEach(link => {
            link.addEventListener('click', function() {
                localStorage.setItem('sidebar-scroll', sidebar.scrollTop.toString());
            });
        });
    }
    
    // Generate Table of Contents
    const tocList = document.getElementById('toc-list');
    if (tocList && content) {
        const headings = content.querySelectorAll('h2, h3');
        if (headings.length > 0) {
            headings.forEach(function(heading, index) {
                // Generate ID if not exists
                if (!heading.id) {
                    heading.id = 'section-' + index;
                }
                
                // Create TOC item
                const li = document.createElement('li');
                const a = document.createElement('a');
                a.href = '#' + heading.id;
                a.textContent = heading.textContent;
                a.dataset.target = heading.id;
                
                // Add class for h3 (nested)
                if (heading.tagName === 'H3') {
                    li.className = 'toc-h3';
                }
                
                li.appendChild(a);
                tocList.appendChild(li);
            });
            
            // Scroll spy - highlight current section
            const tocLinks = tocList.querySelectorAll('a');
            let lastActive = null;
            
            function updateActiveLink() {
                const scrollPos = window.scrollY + 100;
                let current = null;
                
                headings.forEach(function(heading) {
                    if (heading.offsetTop <= scrollPos) {
                        current = heading;
                    }
                });
                
                if (current && current.id) {
                    const activeLink = tocList.querySelector('a[data-target="' + current.id + '"]');
                    if (activeLink && activeLink !== lastActive) {
                        if (lastActive) lastActive.classList.remove('active');
                        activeLink.classList.add('active');
                        lastActive = activeLink;
                    }
                }
            }
            
            window.addEventListener('scroll', updateActiveLink);
            updateActiveLink();
        }
    }
    
    // Back to top button functionality
    const backToTopBtn = document.getElementById('back-to-top');
    if (backToTopBtn) {
        // Show button when user scrolls down
        window.addEventListener('scroll', function() {
            if (window.scrollY > 300) {
                backToTopBtn.classList.add('visible');
            } else {
                backToTopBtn.classList.remove('visible');
            }
        });
        
        // Scroll to top when clicked
        backToTopBtn.addEventListener('click', function() {
            window.scrollTo({
                top: 0,
                behavior: 'smooth'
            });
        });
    }
    
    if (!searchBox) return;
    const packages = document.querySelectorAll('.package');
    searchBox.addEventListener('input', function() {
        const query = this.value.toLowerCase();
        packages.forEach(pkg => {
            const link = pkg.querySelector('a');
            if (link) {
                const text = link.textContent.toLowerCase();
                pkg.style.display = text.includes(query) ? '' : 'none';
            }
        });
    });
});
"""


INDEX_TEMPLATE = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Koala Documentation</title>
    <style>{css}</style>
</head>
<body>
    <div id="sidebar">
        <h2>🐨 Koala</h2>
        <a href="https://github.com/zhuguangxiang/koala-lang" class="github-link" target="_blank">GitHub Repository</a>
        <input type="text" id="search-box" placeholder="Search...">
        {sidebar_content}
    </div>
    <div id="content">
        <h1>Koala Programming Language</h1>
        <p>A simple, fast, and expressive programming language.</p>
        
        <!-- Playground Section -->
        <div class="playground-section">
            <h2>🎮 Playground</h2>
            <p>Try Koala code right in your browser! Edit the code below and click <strong>Run</strong>.</p>
            <div class="playground-container">
                <div class="playground-editor">
                    <textarea id="playground-code" spellcheck="false">func main() {{
    print("Hello, Koala!")
    let x = 10
    let y = 20
    print("Sum: " + str(x + y))
}}</textarea>
                </div>
                <div class="playground-controls">
                    <button id="playground-run" class="playground-btn">▶ Run</button>
                    <button id="playground-clear" class="playground-btn secondary">Clear</button>
                </div>
                <div id="playground-result" class="playground-result">
                    <span class="result-label">Output</span>
                    <div id="playground-output">Click "Run" to execute your code</div>
                </div>
            </div>
        </div>
        
        <div class="doc-cards">
            <div class="doc-card">
                <h3>📖 Language Manual</h3>
                <p>Learn the Koala language from scratch. Covers syntax, types, functions, classes, generics, and more.</p>
                <a href="manual.html" class="doc-card-link">Read the Manual →</a>
            </div>
            <div class="doc-card">
                <h3>📦 API Reference</h3>
                <p>Detailed documentation for the standard library and all packages.</p>
                <a href="std_prelude.html" class="doc-card-link">Browse API →</a>
            </div>
        </div>
    </div>
    <nav id="toc">
        <a href="index.html" id="toc-home">🏠 Home</a>
        <ul id="toc-list"></ul>
        <button id="back-to-top" title="Back to top">↑ Top</button>
    </nav>
    <script>
    // Toggle collapsible sections
    function toggleSection(button) {{
        const content = button.nextElementSibling;
        const arrow = button.querySelector('.arrow');
        
        if (content.classList.contains('hidden')) {{
            content.classList.remove('hidden');
            button.classList.remove('collapsed');
        }} else {{
            content.classList.add('hidden');
            button.classList.add('collapsed');
        }}
    }}
    
    // Toggle workspace sections in sidebar
    function toggleWorkspace(title) {{
        const items = title.nextElementSibling;
        
        if (items.classList.contains('hidden')) {{
            items.classList.remove('hidden');
            title.classList.remove('collapsed');
        }} else {{
            items.classList.add('hidden');
            title.classList.add('collapsed');
        }}
    }}
    
    // Playground functionality
    document.addEventListener('DOMContentLoaded', function() {{
        const codeEditor = document.getElementById('playground-code');
        const runBtn = document.getElementById('playground-run');
        const clearBtn = document.getElementById('playground-clear');
        const resultDiv = document.getElementById('playground-result');
        const outputDiv = document.getElementById('playground-output');
        
        if (!codeEditor || !runBtn) return;  // Only on index page
        
        // Run code
        runBtn.onclick = async function() {{
            const code = codeEditor.value;
            runBtn.disabled = true;
            runBtn.textContent = '⏳ Running...';
            resultDiv.className = 'playground-result';
            outputDiv.textContent = 'Running...';
            
            try {{
                const resp = await fetch('/run', {{
                    method: 'POST',
                    headers: {{'Content-Type': 'application/json'}},
                    body: JSON.stringify({{code: code}})
                }});
                const data = await resp.json();
                
                if (data.error) {{
                    resultDiv.className = 'playground-result error';
                    outputDiv.textContent = data.error;
                }} else {{
                    const output = data.output || '(no output)';
                    const isError = output.includes('error:') || output.includes('failed');
                    resultDiv.className = 'playground-result' + (isError ? ' error' : '');
                    outputDiv.textContent = output;
                }}
            }} catch (e) {{
                resultDiv.className = 'playground-result error';
                outputDiv.textContent = 'Cannot connect to server.\\n\\nStart with:\\npython3 tools/kl-playground-server.py';
            }} finally {{
                runBtn.disabled = false;
                runBtn.textContent = '▶ Run';
            }}
        }};
        
        // Clear output
        clearBtn.onclick = function() {{
            outputDiv.textContent = 'Click "Run" to execute your code';
            resultDiv.className = 'playground-result';
        }};
        
        // Ctrl+Enter to run
        codeEditor.addEventListener('keydown', function(e) {{
            if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {{
                runBtn.click();
            }}
        }});
    }});
    </script>
    <script>{js}</script>
</body>
</html>
"""

PACKAGE_TEMPLATE = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title} - Koala API</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/styles/github-dark.min.css">
    <style>{css}</style>
</head>
<body>
    <div id="sidebar">
        <h2><a href="index.html" style="color:inherit;text-decoration:none;">Koala Docs</a></h2>
        <a href="https://github.com/zhuguangxiang/koala-lang" class="github-link" target="_blank">GitHub Repository</a>
        <input type="text" id="search-box" placeholder="Search...">
        {sidebar_content}
    </div>
    <div id="content">
        {content}
    </div>
    <nav id="toc">
        <a href="index.html" id="toc-home">🏠 Home</a>
        <ul id="toc-list"></ul>
        <button id="back-to-top" title="Back to top">↑ Top</button>
    </nav>
    <script>
    // Toggle collapsible sections
    function toggleSection(button) {{
        const content = button.nextElementSibling;
        
        if (content.classList.contains('hidden')) {{
            content.classList.remove('hidden');
            button.classList.remove('collapsed');
        }} else {{
            content.classList.add('hidden');
            button.classList.add('collapsed');
        }}
    }}
    
    // Toggle workspace sections in sidebar
    function toggleWorkspace(title) {{
        const items = title.nextElementSibling;
        
        if (items.classList.contains('hidden')) {{
            items.classList.remove('hidden');
            title.classList.remove('collapsed');
        }} else {{
            items.classList.add('hidden');
            title.classList.add('collapsed');
        }}
    }}
    </script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/highlight.min.js"></script>
    <script>{js}</script>
</body>
</html>
"""


# ============================================================================
# HTML Rendering Functions
# ============================================================================

def render_member(member: DocItem) -> str:
    """Render a class/trait member."""
    sig = esc(member.signature)
    tags = ""
    if member.tags:
        tags = " — " + ", ".join(f"<em>@{esc(t)}</em>" for t in member.tags)
    doc_html = render_md(member.doc) if member.doc else ""
    return f"""
<div class="member-item" id="{member.name.lower()}">
    <div class="signature"><code>{sig}</code></div>
    <div class="doc-content">{doc_html}</div>
</div>
"""


def render_item(item: DocItem) -> str:
    """Render a top-level item (class/trait/func/const)."""
    badge_class = item.kind
    badge_text = item.kind.capitalize()
    sig = esc(item.signature)
    doc_html = render_md(item.doc) if item.doc else ""

    bases_html = ""
    # No need to show bases separately since signature already contains them

    members_html = ""
    if item.members:
        fields = [m for m in item.members if m.kind in ("var", "let")]
        methods = [m for m in item.members if m.kind == "func"]
        if fields:
            members_html += '<div class="collapsible-section">'
            members_html += '<button class="collapse-toggle collapsed" onclick="toggleSection(this)">Fields <span class="arrow">▼</span></button>'
            members_html += '<div class="collapsible-content hidden">'
            for m in fields:
                members_html += render_member(m)
            members_html += '</div></div>'
        if methods:
            members_html += '<div class="collapsible-section">'
            members_html += '<button class="collapse-toggle collapsed" onclick="toggleSection(this)">Methods <span class="arrow">▼</span></button>'
            members_html += '<div class="collapsible-content hidden">'
            for m in methods:
                members_html += render_member(m)
            members_html += '</div></div>'

    # For func, class, trait items, don't show the header (signature already contains the name)
    if item.kind in ("func", "class", "trait"):
        return f"""
<div class="item" id="{item.name.lower()}">
    <div class="signature"><code>{sig}</code></div>
    {bases_html}
    <div class="doc-content">{doc_html}</div>
    {members_html}
</div>
"""
    
    return f"""
<div class="item" id="{item.name.lower()}">
    <div class="item-header">
        <h3>{esc(item.name)}</h3>
        <span class="badge {badge_class}">{badge_text}</span>
    </div>
    <div class="signature"><code>{sig}</code></div>
    {bases_html}
    <div class="doc-content">{doc_html}</div>
    {members_html}
</div>
"""


def render_package_page(pkg_name: str, files: List[Tuple[str, List[DocItem]]]) -> str:
    """Render a complete package page."""
    all_items = []
    for filename, items in files:
        all_items.extend(items)

    groups = [
        ("Traits", "trait"),
        ("Classes", "class"),
        ("Functions", "func"),
        ("Constants", "const"),
    ]

    parts = [f"<h1>{esc(pkg_name)}</h1>"]

    # Summary tables
    for title, kind in groups:
        items = [it for it in all_items if it.kind == kind]
        if not items:
            continue
        parts.append(f"<h2>{title}</h2>")
        parts.append("<table>")
        parts.append("<tr><th>Name</th><th>Summary</th></tr>")
        for it in items:
            summary = first_paragraph(it.doc) if it.doc else ""
            parts.append(
                f'<tr><td><a href="#{it.name.lower()}">{esc(it.name)}</a></td>'
                f'<td class="summary">{esc(summary)}</td></tr>'
            )
        parts.append("</table>")

    # Detailed sections
    for title, kind in groups:
        items = [it for it in all_items if it.kind == kind]
        if not items:
            continue
        parts.append(f"<h2>{title}</h2>")
        for item in items:
            parts.append(render_item(item))

    return "\n".join(parts)


def parse_manual_sections(md_path: Path) -> List[Dict]:
    """Parse a Markdown manual into sections with headings and content."""
    if not md_path.exists():
        return []
    
    text = md_path.read_text(encoding="utf-8")
    lines = text.split("\n")
    sections = []
    current_section = None
    current_content = []
    
    for line in lines:
        # Match ## or ### headings
        match = re.match(r"^(#{2,3})\s+(.+)$", line)
        if match:
            # Save previous section
            if current_section:
                current_section["content"] = "\n".join(current_content)
                sections.append(current_section)
            
            level = len(match.group(1))
            title = match.group(2).strip()
            anchor = title.lower().replace(" ", "-").replace("(", "").replace(")", "")
            current_section = {
                "level": level,
                "title": title,
                "anchor": anchor,
                "content": "",
            }
            current_content = []
        elif current_section:
            current_content.append(line)
    
    # Save last section
    if current_section:
        current_section["content"] = "\n".join(current_content)
        sections.append(current_section)
    
    return sections


def render_manual_page(sections: List[Dict]) -> str:
    """Render the language manual page."""
    parts = ["<h1>Koala Language Manual</h1>"]
    
    # Table of contents
    parts.append('<nav class="manual-toc">')
    parts.append("<h2>Contents</h2>")
    parts.append("<ul>")
    for sec in sections:
        if sec["level"] == 2:
            parts.append(f'<li><a href="#{sec["anchor"]}">{esc(sec["title"])}</a>')
            # Add sub-sections
            sub_items = [s for s in sections if s["level"] == 3 and sections.index(s) > sections.index(sec)]
            if sub_items:
                parts.append("<ul>")
                for sub in sub_items:
                    if sections.index(sub) > sections.index(sec):
                        # Check if we've hit the next ## section
                        next_h2 = next((s for s in sections if s["level"] == 2 and sections.index(s) > sections.index(sec)), None)
                        if next_h2 and sections.index(sub) < sections.index(next_h2):
                            parts.append(f'<li><a href="#{sub["anchor"]}">{esc(sub["title"])}</a></li>')
                        elif not next_h2:
                            parts.append(f'<li><a href="#{sub["anchor"]}">{esc(sub["title"])}</a></li>')
                parts.append("</ul>")
            parts.append("</li>")
    parts.append("</ul>")
    parts.append("</nav>")
    
    # Content sections
    for sec in sections:
        if sec["title"] == "Contents":
            continue
        
        tag = f"h{sec['level']}"
        parts.append(f'<{tag} id="{sec["anchor"]}">{esc(sec["title"])}</{tag}>')
        
        # Render content as Markdown
        content_html = render_md(sec["content"])
        parts.append(f'<div class="manual-content">{content_html}</div>')
    
    return "\n".join(parts)


def build_sidebar(workspaces: Dict[str, Dict[str, Path]], active_pkg: str = "") -> str:
    """Build sidebar navigation."""
    parts = []
    
    # Language Manual section
    parts.append(f'<div class="workspace">')
    parts.append(f'<div class="workspace-title" onclick="toggleWorkspace(this)">Guide <span class="ws-arrow">▼</span></div>')
    parts.append(f'<div class="workspace-items">')
    active_class = ' class="active"' if active_pkg == "manual" else ""
    parts.append(f'<div class="package">')
    parts.append(f'<a href="manual.html"{active_class}>Language Manual</a>')
    parts.append(f"</div>")
    parts.append(f"</div>")
    parts.append(f"</div>")
    
    # API sections
    for ws_name, packages in workspaces.items():
        parts.append(f'<div class="workspace">')
        parts.append(f'<div class="workspace-title" onclick="toggleWorkspace(this)">{esc(ws_name)} <span class="ws-arrow">▼</span></div>')
        parts.append(f'<div class="workspace-items">')
        for pkg_name in sorted(packages.keys()):
            html_file = f"{ws_name}_{pkg_name.replace('/', '_')}.html"
            active_class = ' class="active"' if f"{ws_name}/{pkg_name}" == active_pkg else ""
            parts.append(f'<div class="package">')
            parts.append(f'<a href="{html_file}"{active_class}>{esc(pkg_name)}</a>')
            parts.append(f"</div>")
        parts.append(f"</div>")
        parts.append(f"</div>")
    return "\n".join(parts)


def scan_workspace(ws_path: Path) -> Dict[str, Path]:
    """Scan a workspace directory for packages with .kl files."""
    packages = {}
    if not ws_path.exists():
        return packages
    for item in sorted(ws_path.iterdir()):
        if not item.is_dir():
            continue
        if item.name in ("dist", "native"):
            continue
        kl_files = list(item.glob("*.kl"))
        if kl_files:
            packages[item.name] = item
        else:
            for subitem in sorted(item.iterdir()):
                if subitem.is_dir():
                    sub_kl = list(subitem.glob("*.kl"))
                    if sub_kl:
                        packages[f"{item.name}/{subitem.name}"] = subitem
    return packages


def main():
    ap = argparse.ArgumentParser(
        description="Generate Rust-style HTML API documentation for Koala."
    )
    ap.add_argument(
        "--workspace",
        choices=["std", "koala"],
        help="Generate docs for a specific workspace only",
    )
    ap.add_argument(
        "--output",
        default="tools/playground/html",
        help="Output directory (default: tools/playground/html)",
    )
    args = ap.parse_args()

    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    libs_dir = project_root / "libs"

    if not libs_dir.exists():
        print(f"Error: libs directory not found at {libs_dir}", file=sys.stderr)
        sys.exit(1)

    workspaces_to_scan = ["std", "koala"] if not args.workspace else [args.workspace]
    workspaces = {}
    for ws_name in workspaces_to_scan:
        ws_path = libs_dir / ws_name
        packages = scan_workspace(ws_path)
        if packages:
            workspaces[ws_name] = packages

    if not workspaces:
        print("Error: no packages found", file=sys.stderr)
        sys.exit(1)

    out_dir = Path(args.output)
    out_dir.mkdir(parents=True, exist_ok=True)

    total_symbols = 0
    for ws_name, packages in workspaces.items():
        for pkg_name, pkg_path in packages.items():
            kl_files = sorted(pkg_path.glob("*.kl"))
            if not kl_files:
                continue

            files = []
            pkg_symbols = 0
            for path in kl_files:
                items = parse_source(path.read_text(encoding="utf-8"))
                pkg_symbols += len(items) + sum(len(it.members) for it in items)
                files.append((path.name, items))

            total_symbols += pkg_symbols

            full_pkg_name = f"{ws_name}/{pkg_name}"
            content = render_package_page(full_pkg_name, files)
            sidebar = build_sidebar(workspaces, full_pkg_name)

            page_html = PACKAGE_TEMPLATE.format(
                title=full_pkg_name,
                css=CSS,
                js=JS,
                sidebar_content=sidebar,
                content=content,
            )

            html_file = out_dir / f"{ws_name}_{pkg_name.replace('/', '_')}.html"
            html_file.write_text(page_html, encoding="utf-8")
            print(f"Generated {html_file.name}: {pkg_symbols} symbols")

    # Index page
    sidebar = build_sidebar(workspaces)
    ws_list = []
    for ws_name, packages in workspaces.items():
        links = ", ".join(
            f'<a href="{ws_name}_{p.replace("/", "_")}.html">{esc(p)}</a>'
            for p in sorted(packages.keys())
        )
        ws_list.append(f"<h3>{esc(ws_name)}</h3><p>{links}</p>")

    index_html = INDEX_TEMPLATE.format(
        css=CSS,
        js=JS,
        sidebar_content=sidebar,
        workspace_list="\n".join(ws_list),
    )
    index_file = out_dir / "index.html"
    index_file.write_text(index_html, encoding="utf-8")

    # Language Manual page
    manual_md = project_root / "docs" / "Koala_Language_Manual.md"
    if manual_md.exists():
        sections = parse_manual_sections(manual_md)
        manual_content = render_manual_page(sections)
        manual_sidebar = build_sidebar(workspaces, "manual")
        
        manual_html = PACKAGE_TEMPLATE.format(
            title="Language Manual",
            css=CSS,
            js=JS,
            sidebar_content=manual_sidebar,
            content=manual_content,
        )
        
        manual_file = out_dir / "manual.html"
        manual_file.write_text(manual_html, encoding="utf-8")
        print(f"Generated {manual_file.name}: {len(sections)} sections")

    print(f"\nGenerated {index_file.name}")
    print(f"Total: {total_symbols} symbols across {sum(len(p) for p in workspaces.values())} packages")
    print(f"Output: {out_dir}")


if __name__ == "__main__":
    main()
