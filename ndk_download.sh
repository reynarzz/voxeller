#!/usr/bin/env bash
set -euo pipefail

# ── 1) Hard-coded NDK version (latest stable as of mid-2025)
NDK_VERSION="r28b"

# ── 2) Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEST_DIR="$SCRIPT_DIR/third/android-ndk"

# ── 3) Detect OS for filename
UNAME="$(uname -s)"
case "$UNAME" in
  Linux*)       PLATFORM="linux"   ;;
  Darwin*)      PLATFORM="darwin"  ;;
  CYGWIN*|MINGW*|MSYS*|MINGW64*) PLATFORM="windows" ;;
  *)            echo "❌ Unsupported OS: $UNAME" >&2; exit 1 ;;
esac

# ── 4) Choose archive extension
EXT="zip"
if [[ "$PLATFORM" == "darwin" ]]; then
  EXT="dmg"
fi

# ── 5) Build filename & URL
NDK_FILE="android-ndk-${NDK_VERSION}-${PLATFORM}.${EXT}"
NDK_URL="https://dl.google.com/android/repository/${NDK_FILE}"
ARCHIVE_PATH="$SCRIPT_DIR/$NDK_FILE"

# ── 6) Skip if already installed
if [[ -d "$DEST_DIR" ]]; then
  echo "✅ NDK already installed at $DEST_DIR"
  exit 0
fi

# ── 7) Download (disable revocation check on Windows/Schannel)
echo "⬇️  Downloading Android NDK ${NDK_VERSION} for ${PLATFORM}..."
if [[ "$PLATFORM" == "windows" ]]; then
  curl --ssl-no-revoke -fL -o "$ARCHIVE_PATH" "$NDK_URL"
else
  curl -fL -o "$ARCHIVE_PATH" "$NDK_URL"
fi

# ── 8) Extract to ./android-ndk
echo "🗜️  Extracting into $DEST_DIR..."
if [[ "$EXT" == "zip" ]]; then
  unzip -q "$ARCHIVE_PATH" -d "$SCRIPT_DIR"
  mv "$SCRIPT_DIR/android-ndk-${NDK_VERSION}" "$DEST_DIR"
else
  # macOS .dmg handling
 	MNT="/Volumes/android-ndk-${REVISION}"
  hdiutil attach "$ARCHIVE_PATH" -mountpoint "$MNT" -nobrowse
  
  # create our target and copy everything from the root of the volume
  mkdir -p "$DEST_DIR"
  cp -R "$MNT/"* "$DEST_DIR"
  
  hdiutil detach "$MNT"
fi

# ── 9) Cleanup
rm "$ARCHIVE_PATH"

# ── 10) Done
cat <<EOF

🎉 Installed Android NDK ${NDK_VERSION} to:
    $DEST_DIR

👉 To use with CMake:
    export ANDROID_NDK=\$DEST_DIR
EOF
