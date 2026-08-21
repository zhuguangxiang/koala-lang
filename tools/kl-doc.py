#!/usr/bin/env python3
"""kl-doc.py - Extract /// doc comments from Koala source files and render
them as a Markdown API reference.

Usage:

    python3 tools/kl-doc.py libs/std/builtin
    python3 tools/kl-doc.py libs/std/builtin/str.kl
    python3 tools/kl-doc.py libs/std/builtin --output docs/builtin_api.md

The source argument (a directory or a single .kl file) is mandatory;
the output file is optional and defaults to <package>_api.md in the
current directory. On invalid arguments the full help is printed.

What is extracted:
  - `///` doc comments attached to the declaration that follows them
  - top-level `pub class` / `pub trait` / `pub func` declarations
  - member `func` and `var`/`let` field declarations inside classes
    and traits
  - base types (`: A & B`) rendered as Supertraits/Bases lines
  - `@native` / `@intrinsic` annotations are reported as tags

What is ignored:
  - `/* ... */` block comments (including commented-out code)
  - plain `//` comments and non-declaration statements (e.g. `link "koala"`)
"""

import argparse
import re
import sys
import textwrap
from dataclasses import dataclass, field
from pathlib import Path

DOC_PREFIX = "///"
ANNOT_RE = re.compile(r"^\s*@(\w+)\s*$")
LABEL_RE = re.compile(r"^(Example|Notes?|Precondition|Panics|See):\s*(.*)$")
DECL_RE = re.compile(
    r"^\s*(?P<pub>pub\s+)?(?P<kind>class|trait|func)\s+(?P<name>\w+)(?P<rest>.*)$"
)
FIELD_RE = re.compile(
    r"^\s*(?P<pub>pub\s+)?(?P<kind>var|let)\s+(?P<name>\w+)(?P<rest>.*)$"
)
# only strip braces at the very end of a declaration header; an unanchored
# `\{\s*\}` would span the whole class body
BRACE_RE = re.compile(r"\s*\{\s*\}\s*$")
OPEN_BRACE_RE = re.compile(r"\s*\{\s*$")


def _split_at_toplevel(text, sep):
    """Split `text` on `sep` occurrences that sit outside [ ] and ( )."""
    parts = []
    depth = 0
    cur = ""
    for ch in text:
        if ch in "[(":
            depth += 1
        elif ch in "])":
            depth -= 1
        if ch == sep and depth == 0:
            parts.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        parts.append(cur.strip())
    return parts


def bases_of(signature):
    """Base types of a class/trait signature: the `A & B` list after the
    top-level ':' (type parameter sections are skipped)."""
    depth = 0
    seen_name = False
    colon = -1
    for i, ch in enumerate(signature):
        if ch in "[(":
            depth += 1
        elif ch in "])":
            depth -= 1
        elif ch.isalnum() or ch == "_":
            seen_name = True
        elif ch == ":" and depth == 0 and seen_name:
            colon = i
            break
    if colon == -1:
        return []
    rest = signature[colon + 1 :]
    brace = rest.find("{")
    if brace != -1:
        rest = rest[:brace]
    bases = []
    for part in _split_at_toplevel(rest, "&"):
        bases.extend(p for p in _split_at_toplevel(part, ",") if p)
    return bases


def first_sentence(text):
    """First sentence of a doc text, for Contents summaries."""
    para = text.split("\n\n", 1)[0].replace("\n", " ").strip()
    m = re.search(r"(?<=[.!?])(?=\s|$)", para)
    if m and m.start() > 0:
        return para[: m.start()]
    return para


@dataclass
class DocItem:
    kind: str  # 'class' | 'trait' | 'func' | 'field'
    name: str
    signature: str  # declaration header line, brace stripped
    doc: str
    tags: list = field(default_factory=list)
    members: list = field(default_factory=list)


def strip_block_comments(lines):
    """Remove /* ... */ comments; doc lines are passed through untouched."""
    out = []
    in_block = False
    for line in lines:
        if in_block:
            end = line.find("*/")
            if end == -1:
                out.append("")
            else:
                in_block = False
                out.append(line[end + 2 :])
            continue
        if line.lstrip().startswith(DOC_PREFIX):
            out.append(line)
            continue
        start = line.find("/*")
        if start == -1:
            out.append(line)
            continue
        end = line.find("*/", start + 2)
        if end != -1:
            out.append(line[:start] + line[end + 2 :])
        else:
            in_block = True
            out.append(line[:start])
    return out


def parse_source(text):
    """Parse one .kl file, return the list of top-level DocItems."""
    toplevel = []
    container = None  # current class/trait being filled with members
    depth = 0
    member_depth = None  # depth of the class/trait member section
    pending_doc = []
    pending_tags = []

    lines = strip_block_comments(text.splitlines())
    i = 0
    while i < len(lines):
        raw = lines[i]
        i += 1
        line = raw.rstrip()
        stripped = line.strip()

        if not stripped:
            # a real blank line breaks the doc-to-declaration binding
            pending_doc = []
            pending_tags = []
            continue

        if stripped.startswith(DOC_PREFIX):
            content = stripped[len(DOC_PREFIX) :]
            if content.startswith(" "):
                content = content[1:]
            pending_doc.append(content)
            continue

        if stripped.startswith("//"):
            continue

        m = ANNOT_RE.match(line)
        if m:
            pending_tags.append(m.group(1))
            continue

        m = DECL_RE.match(line)
        if m:
            header = stripped
            # braces must be counted on the raw line(s), the header is
            # stripped of them later
            opens = line.count("{") - line.count("}")
            # a declaration may span several lines while '(' stays open
            while header.count("(") > header.count(")") and i < len(lines):
                cont_line = lines[i]
                i += 1
                opens += cont_line.count("{") - cont_line.count("}")
                header = (header + " " + cont_line.strip()).strip()
            header = BRACE_RE.sub("", header)
            header = OPEN_BRACE_RE.sub("", header)
            header = re.sub(r"\s+", " ", header).strip()
            # tidy whitespace left over from joining multi-line signatures
            header = re.sub(r"\(\s+", "(", header)
            header = re.sub(r"\s+\)", ")", header)
            item = DocItem(
                kind=m.group("kind"),
                name=m.group("name"),
                signature=header,
                doc="\n".join(pending_doc).strip(),
                tags=list(pending_tags),
            )
            pending_doc = []
            pending_tags = []
            if container is None:
                toplevel.append(item)
            else:
                container.members.append(item)
            depth += opens
            if item.kind in ("class", "trait") and opens > 0:
                container = item
                member_depth = depth
            continue

        # class/trait member field declarations (var/let)
        f = FIELD_RE.match(line)
        if f and container is not None and member_depth is not None \
                and depth == member_depth:
            header = BRACE_RE.sub("", stripped)
            header = OPEN_BRACE_RE.sub("", header)
            container.members.append(
                DocItem(
                    kind="field",
                    name=f.group("name"),
                    signature=header,
                    doc="\n".join(pending_doc).strip(),
                    tags=list(pending_tags),
                )
            )
            pending_doc = []
            pending_tags = []
            depth += line.count("{") - line.count("}")
            continue

        # anything else: closing braces, `link "koala"`, locals, etc.
        depth += line.count("{") - line.count("}")
        if depth <= 0:
            depth = 0
            container = None
            member_depth = None
        elif container is None or depth < member_depth:
            container = None
            member_depth = None
        pending_doc = []
        pending_tags = []

    return toplevel


def render_doc(doc, links=None):
    """Render doc text as Markdown.

    Line breaks follow blank lines only: consecutive non-empty doc lines
    are joined into one paragraph line; an empty doc line starts a new
    paragraph. 4-space indented runs become standalone code blocks.
    Label lines (Example:/Note:/Notes:/Precondition:/Panics:/See:) are
    rendered bold; `See:` backticked references to known symbols become
    anchor links.
    """
    links = links or {}
    out = []
    para = []
    block = []

    def flush_para():
        if para:
            if out and out[-1] != "":
                out.append("")
            out.append(linkify(" ".join(para)))
            para.clear()

    def flush_block():
        if block:
            out.append("")
            out.append("```kl")
            out.extend(textwrap.dedent("\n".join(block)).splitlines())
            out.append("```")
            out.append("")
            block.clear()

    def linkify(text):
        def repl(m):
            name = m.group(1)
            # qualified `Type.member` first, then the top-level symbol;
            # an optional trailing `()` (call form) is ignored for lookup
            bare = name[:-2] if name.endswith("()") else name
            anchor = (
                links.get(name)
                or links.get(bare)
                or links.get(name.split(".")[0])
                or links.get(bare.split(".")[0])
            )
            if anchor:
                return f"[`{name}`](#{anchor})"
            return m.group(0)

        return re.sub(r"`([^`]+)`", repl, text)

    for line in doc.splitlines():
        if not line.strip():
            flush_para()
            flush_block()
        elif line.startswith("    "):
            flush_para()
            block.append(line)
        else:
            m = LABEL_RE.match(line.strip())
            if m:
                flush_para()
                flush_block()
                label = f"**{m.group(1)}:**"
                rest = m.group(2).strip()
                if rest:
                    label += " " + linkify(rest)
                if out and out[-1] != "":
                    out.append("")
                out.append(label)
            else:
                flush_block()
                para.append(line.strip())
    flush_para()
    flush_block()
    return "\n".join(out)


def short_sig(item):
    """Signature without the leading `pub class/trait/func` keywords."""
    return re.sub(r"^(pub\s+)?(class|trait|func)\s+", "", item.signature)


def decl_block(item):
    """Fenced declaration of a class/trait, with member signature stubs."""
    lines = ["```kl"]
    if item.members:
        lines.append(item.signature + " {")
        for mem in item.members:
            lines.append("    " + mem.signature)
        lines.append("}")
    else:
        lines.append(item.signature + " {}")
    lines.append("```")
    return lines


def render_member(mem, links, anchor=None):
    """One member: bold signature, tags, then doc text."""
    lines = []
    if anchor:
        lines.append(f'<a id="{anchor}"></a>')
        lines.append("")
    lines.append(f"**`{short_sig(mem)}`**")
    if mem.tags:
        lines[-1] += " — " + " ".join(f"*`@{t}`*" for t in mem.tags)
    lines.append("")
    if mem.doc:
        lines.append(render_doc(mem.doc, links))
        lines.append("")
    return lines


def render_bases(item, links):
    """Supertraits / Bases line with links, rustdoc style."""
    bases = bases_of(item.signature)
    if not bases:
        return []

    def link_base(b):
        bare = b.split("[", 1)[0].strip()
        anchor = links.get(bare)
        return f"[`{b}`](#{anchor})" if anchor else f"`{b}`"

    label = "Supertraits" if item.kind == "trait" else "Bases"
    joined = " & ".join(link_base(b) for b in bases)
    return [f"**{label}:** {joined}", ""]


def render_item(item, links, qname):
    """Render one top-level class/trait/func as Markdown."""
    lines = [f"### `{item.name}`", ""]
    if item.kind in ("class", "trait"):
        lines += decl_block(item)
        lines += render_bases(item, links)
    else:
        lines += ["```kl", item.signature, "```"]
        if item.tags:
            lines.append("")
            lines.append(" ".join(f"*`@{t}`*" for t in item.tags))
    lines.append("")
    if item.doc:
        lines.append(render_doc(item.doc, links))
        lines.append("")
    fields = [m for m in item.members if m.kind == "field"]
    methods = [m for m in item.members if m.kind != "field"]
    if fields or methods:
        lines.append("---")
        lines.append("")
    if fields:
        lines.append("**Fields**")
        lines.append("")
        for mem in fields:
            lines += render_member(mem, links, f"{qname}.{mem.name}")
    for mem in methods:
        lines += render_member(mem, links, f"{qname}.{mem.name}")
    return "\n".join(lines).rstrip()


def render_package(pkg_name, files):
    """files: list of (filename, [DocItem]); returns full Markdown text."""
    # symbol name -> anchor, used for cross references; members are
    # addressable as `Type.member` (anchors: `type` / `type.member`)
    links = {}
    for _, items in files:
        for it in items:
            anchor = it.name.lower()
            links[it.name] = anchor
            for mem in it.members:
                links[f"{it.name}.{mem.name}"] = f"{anchor}.{mem.name.lower()}"
    lines = []
    lines.append(f"# Koala `{pkg_name}` API Reference")
    lines.append("")
    lines.append("## Contents")
    lines.append("")
    groups = (("Traits", "trait"), ("Classes", "class"), ("Functions", "func"))
    for title, kind in groups:
        entries = [
            it for _, items in files for it in items if it.kind == kind
        ]
        if not entries:
            continue
        lines.append(f"**{title}**")
        lines.append("")
        lines.append("| Name | Summary |")
        lines.append("|------|---------|")
        for it in entries:
            summary = first_sentence(it.doc).replace("|", "\\|") if it.doc else ""
            lines.append(f"| [`{it.name}`](#{it.name.lower()}) | {summary} |")
        lines.append("")

    for title, kind in groups:
        entries = [
            it for _, items in files for it in items if it.kind == kind
        ]
        if not entries:
            continue
        lines.append(f"## {title}")
        lines.append("")
        for item in entries:
            lines.append(render_item(item, links, item.name.lower()))
            lines.append("")
    return "\n".join(lines).rstrip() + "\n"


class HelpOnErrorParser(argparse.ArgumentParser):
    """ArgumentParser that prints the full help on any argument error."""

    def error(self, message):
        self.print_help(sys.stderr)
        sys.stderr.write(f"\nerror: {message}\n")
        sys.exit(2)


def main():
    ap = HelpOnErrorParser(
        description="Extract /// doc comments from Koala sources into Markdown."
    )
    ap.add_argument(
        "src",
        metavar="input src file or src directory",
        help="source package directory containing .kl files, or a single "
        ".kl file, e.g. libs/std/builtin or libs/std/builtin/str.kl",
    )
    ap.add_argument(
        "--output",
        default=None,
        help="output Markdown file (default: <package>_api.md in the "
        "current directory)",
    )
    args = ap.parse_args()

    src = Path(args.src)
    if src.is_dir():
        kl_files = sorted(src.glob("*.kl"))
        if not kl_files:
            ap.error(f"no .kl files found in {src}")
    elif src.is_file():
        if src.suffix != ".kl":
            ap.error(f"input file must be a .kl file: {src}")
        kl_files = [src]
    else:
        ap.error(f"source directory or file not found: {src}")

    files = []
    total = 0
    for path in kl_files:
        items = parse_source(path.read_text(encoding="utf-8"))
        total += len(items) + sum(len(it.members) for it in items)
        files.append((path.name, items))

    base = src if src.is_dir() else src.parent
    pkg_name = base.as_posix().removeprefix("libs/")
    if not pkg_name or pkg_name == ".":
        pkg_name = src.stem if src.is_file() else src.name
    text = render_package(pkg_name, files)

    if args.output:
        out = Path(args.output)
    elif src.is_file():
        out = Path(src.stem + "_api.md")
    else:
        out = Path(pkg_name.replace("/", "_") + "_api.md")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(text, encoding="utf-8")

    print(
        f"kl-doc: {len(kl_files)} files, {total} documented symbols -> {out}"
    )


if __name__ == "__main__":
    main()
