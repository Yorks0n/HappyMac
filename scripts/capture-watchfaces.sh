#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${ROOT_DIR}/screenshots/generated"

ALL_PLATFORMS=(aplite basalt chalk diorite emery flint)
ALL_THEMES=(light dark color)

theme_value() {
  case "$1" in
    light) echo 0 ;;
    dark) echo 1 ;;
    color) echo 2 ;;
    *)
      echo "Unknown theme: $1" >&2
      exit 2
      ;;
  esac
}

usage() {
  cat <<'EOF'
Usage:
  scripts/capture-watchfaces.sh [--platform PLATFORM[,PLATFORM...]] [--theme THEME[,THEME...]] [--out DIR]

Defaults:
  platforms: aplite,basalt,chalk,diorite,emery,flint,gabbro
  themes:    light,dark,color
  out:       screenshots/generated

Examples:
  scripts/capture-watchfaces.sh
  scripts/capture-watchfaces.sh --platform basalt,chalk --theme color
EOF
}

split_csv() {
  local value="$1"
  local -n target="$2"
  IFS=',' read -r -a target <<< "$value"
}

PLATFORMS=("${ALL_PLATFORMS[@]}")
THEMES=("${ALL_THEMES[@]}")

while [[ $# -gt 0 ]]; do
  case "$1" in
    --platform)
      split_csv "$2" PLATFORMS
      shift 2
      ;;
    --theme)
      split_csv "$2" THEMES
      shift 2
      ;;
    --out)
      OUT_DIR="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

cd "$ROOT_DIR"
mkdir -p "$OUT_DIR"

for theme in "${THEMES[@]}"; do
  theme_code="$(theme_value "$theme")"
  echo "Building ${theme} theme..."
  HAPPYMAC_SCREENSHOT_THEME="$theme_code" pebble clean >/dev/null
  HAPPYMAC_SCREENSHOT_THEME="$theme_code" pebble build >/dev/null

  for platform in "${PLATFORMS[@]}"; do
    filename="${OUT_DIR}/${platform}-${theme}.png"
    echo "Capturing ${platform} ${theme} -> ${filename}"
    pebble install --emulator "$platform" build/HappyMac.pbw >/dev/null
    pebble screenshot --emulator "$platform" --no-open "$filename" >/dev/null
  done
done

echo "Done. Screenshots written to ${OUT_DIR}"
