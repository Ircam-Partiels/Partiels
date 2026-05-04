#!/usr/bin/env python3

import argparse
import json
import re
import subprocess
from pathlib import Path

MAX_CHUNK_SIZE = 900


def run_pandoc(pandoc: str, markdown: str) -> str:
    process = subprocess.run(
        [pandoc, "-f", "markdown-raw_html", "-t", "plain", "--wrap=none"],
        input=markdown,
        text=True,
        capture_output=True,
        check=False,
    )
    if process.returncode != 0:
        raise RuntimeError(process.stderr.strip() or "pandoc conversion failed")
    return process.stdout


def normalize_plain_text(text: str) -> str:
    lines = [line.rstrip() for line in text.splitlines()]
    compact = []
    previous_blank = False
    for line in lines:
        stripped = line.strip()
        if not stripped:
            if not previous_blank:
                compact.append("")
            previous_blank = True
            continue
        compact.append(stripped)
        previous_blank = False
    return "\n".join(compact).strip()


def split_markdown_sections(markdown: str):
    sections = []
    current_title = "Overview"
    current_lines = []

    for line in markdown.splitlines():
        heading = re.match(r"^#{1,6}\s+(.+?)\s*$", line)
        if heading:
            if current_lines:
                sections.append((current_title, "\n".join(current_lines).strip()))
            current_title = heading.group(1).strip()
            current_lines = []
            continue
        current_lines.append(line)

    if current_lines:
        sections.append((current_title, "\n".join(current_lines).strip()))

    return [(title, content) for title, content in sections if content.strip()]


def chunk_plain_text(text: str):
    paragraphs = [p.strip() for p in re.split(r"\n\s*\n", text) if p.strip()]
    if not paragraphs:
        return []

    chunks = []
    current = ""
    for paragraph in paragraphs:
        candidate = f"{current}\n\n{paragraph}" if current else paragraph
        if len(candidate) > MAX_CHUNK_SIZE and current:
            chunks.append(current)
            current = paragraph
        else:
            current = candidate

    if current:
        chunks.append(current)
    return chunks


def to_entries(pandoc: str, doc_name: str, section: str, markdown_content: str, entries):
    # Remove markdown images: ![alt](url) and ![alt][ref]
    markdown_content = re.sub(r"!\[[^\]]*\]\([^)]*\)", "", markdown_content)
    markdown_content = re.sub(r"!\[[^\]]*\]\[[^\]]*\]", "", markdown_content)
    plain = normalize_plain_text(run_pandoc(pandoc, markdown_content))
    slug = lambda v: (re.sub(r"[^a-z0-9]+", "-", v.strip().lower()).strip("-") or "section")
    for index, chunk in enumerate(chunk_plain_text(plain), start=1):
        section_label = section or "Overview"
        entries.append(
            {
                "id": f"{slug(doc_name)}-{slug(section_label)}-{index}",
                "content": chunk,
                "section": section_label,
                "document": doc_name,
            }
        )


def build_entries(pandoc: str, path: Path) -> list:
    doc_name = path.stem
    content = path.read_text(encoding="utf-8")
    entries = []

    if path.suffix.lower() in {".md", ".markdown"}:
        for section, section_markdown in split_markdown_sections(content):
            to_entries(pandoc, doc_name, section, section_markdown, entries)
    else:
        to_entries(pandoc, doc_name, "Overview", content, entries)

    return entries


def main():
    parser = argparse.ArgumentParser(description="Generate RAG JSON resources from a single doc.")
    parser.add_argument("--pandoc", required=True)
    parser.add_argument("--output-dir", required=True, help="Output directory for JSON files")
    parser.add_argument("input", help="Path to the input document")
    args = parser.parse_args()

    input_path = Path(args.input)
    entries = build_entries(args.pandoc, input_path)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / (input_path.stem + ".json")
    output_path.write_text(json.dumps(entries, ensure_ascii=False, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
