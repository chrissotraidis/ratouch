#!/usr/bin/env python3
"""Reject public-repository asset leaks and broken local documentation links."""

from pathlib import Path
import re
import struct
import subprocess
import sys
from typing import Optional, Tuple
from urllib.parse import unquote
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN_SUFFIXES = {".mix", ".iso", ".vqa", ".vqp", ".aud", ".shp", ".wsa", ".sav", ".save"}
ALLOWED_PUBLIC_IMAGES = {
    "docs/images/platforms.svg",
    "docs/images/ratouch-banner.png",
    "docs/images/ratouch-gameplay.png",
    "docs/images/ratouch-gameplay-concept.png",
    "docs/images/touch-controls.svg",
}
PUBLIC_RASTER_MINIMUMS = {
    "docs/images/ratouch-banner.png": (2000, 650),
    "docs/images/ratouch-gameplay.png": (2000, 1400),
    "docs/images/ratouch-gameplay-concept.png": (1600, 900),
}
PUBLIC_SVGS = {
    "docs/images/platforms.svg",
    "docs/images/touch-controls.svg",
}
IOS_APP_ICON = "apple/ios/Assets.xcassets/AppIcon.appiconset/AppIcon-1024.png"


def repository_files() -> list[str]:
    result = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
        cwd=ROOT,
        check=True,
        capture_output=True,
    )
    return [path for path in result.stdout.decode().split("\0") if path]


def local_targets(text: str) -> list[str]:
    markdown = re.findall(r"!?\[[^\]]*\]\(([^)]+)\)", text)
    html = re.findall(r"(?:src|href)=[\"']([^\"']+)[\"']", text)
    return markdown + html


def png_dimensions(path: Path) -> Optional[Tuple[int, int]]:
    header = path.read_bytes()[:24]
    if len(header) != 24 or header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
        return None
    return struct.unpack(">II", header[16:24])


def main() -> int:
    files = repository_files()
    errors: list[str] = []

    for relative in files:
        path = Path(relative)
        lowered = relative.lower()
        if path.suffix.lower() in FORBIDDEN_SUFFIXES or path.name.lower().startswith("savegame."):
            errors.append(f"commercial/runtime payload must not be public: {relative}")
        if lowered.startswith("ref/") and relative not in {"ref/.gitignore", "ref/README.md"}:
            errors.append(f"ref payload escaped its ignore policy: {relative}")
        if lowered.startswith("docs/images/") and relative not in ALLOWED_PUBLIC_IMAGES:
            errors.append(f"unreviewed public image: {relative}")

    for relative in sorted(ALLOWED_PUBLIC_IMAGES):
        if relative not in files or not (ROOT / relative).exists():
            errors.append(f"missing reviewed public image: {relative}")

    for relative, minimum in PUBLIC_RASTER_MINIMUMS.items():
        path = ROOT / relative
        if not path.exists():
            continue
        dimensions = png_dimensions(path)
        if dimensions is None:
            errors.append(f"public image is not a valid PNG: {relative}")
        elif dimensions[0] < minimum[0] or dimensions[1] < minimum[1]:
            errors.append(
                f"public image is too small: {relative} is {dimensions[0]}x{dimensions[1]}, "
                f"minimum {minimum[0]}x{minimum[1]}"
            )

    for relative in sorted(PUBLIC_SVGS):
        path = ROOT / relative
        if not path.exists():
            continue
        try:
            root = ET.parse(path).getroot()
        except ET.ParseError as error:
            errors.append(f"public SVG is invalid: {relative}: {error}")
            continue
        children = {child.tag.rsplit("}", 1)[-1]: child for child in root}
        if not (children.get("title") is not None and (children["title"].text or "").strip()):
            errors.append(f"public SVG is missing a title: {relative}")
        if not (children.get("desc") is not None and (children["desc"].text or "").strip()):
            errors.append(f"public SVG is missing a description: {relative}")

    icon = ROOT / IOS_APP_ICON
    if IOS_APP_ICON not in files or not icon.exists():
        errors.append(f"missing iOS app icon: {IOS_APP_ICON}")
    else:
        header = icon.read_bytes()[:26]
        dimensions = png_dimensions(icon)
        if dimensions is None or len(header) != 26:
            errors.append(f"iOS app icon is not a valid PNG: {IOS_APP_ICON}")
        else:
            width, height = dimensions
            color_type = header[25]
            if (width, height) != (1024, 1024):
                errors.append(f"iOS app icon must be 1024x1024: {width}x{height}")
            if color_type in {4, 6}:
                errors.append("iOS app icon must not contain an alpha channel")

    documents = [path for path in files if Path(path).suffix.lower() == ".md"]
    for relative in documents:
        document = ROOT / relative
        for raw_target in local_targets(document.read_text(encoding="utf-8")):
            target = raw_target.strip().split(maxsplit=1)[0].strip("<>")
            if not target or target.startswith(("#", "http://", "https://", "mailto:")):
                continue
            target = unquote(target.split("#", 1)[0])
            resolved = (document.parent / target).resolve()
            try:
                resolved.relative_to(ROOT)
            except ValueError:
                errors.append(f"link leaves repository: {relative} -> {raw_target}")
                continue
            if not resolved.exists():
                errors.append(f"broken local link: {relative} -> {raw_target}")

    if errors:
        print("Public repository verification failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(f"Public repository verification passed ({len(files)} files, {len(documents)} Markdown documents).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
