#!/usr/bin/env bash
# Install this checkout's modern Qt app without root or global package changes.
set -euo pipefail
repo=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
build=${1:-"$repo/build-linux"}
prefix="$HOME/.local/lib/sony-device-center"
bin="$HOME/.local/bin"
config=${XDG_CONFIG_HOME:-"$HOME/.config"}
data=${XDG_DATA_HOME:-"$HOME/.local/share"}
for artifact in apps/device-center/sony-device-center apps/sonyd/sonyd apps/sonyctl/sonyctl; do
  test -x "$build/$artifact" || { echo "Build missing: $build/$artifact" >&2; exit 1; }
done
install -d "$prefix" "$bin" "$data/applications" "$config/systemd/user"
for artifact in apps/device-center/sony-device-center apps/sonyd/sonyd apps/sonyctl/sonyctl; do
  install -m 755 "$build/$artifact" "$prefix/${artifact##*/}"
done
install -m 644 "$repo/LICENSE" "$prefix/LICENSE"
install -m 644 "$repo/packaging/linux/sony-device-center.png" "$prefix/icon.png"
cat > "$config/systemd/user/sony-device-center-local.service" <<EOF
[Unit]
Description=Sony Device Center local Bluetooth companion
After=bluetooth.target

[Service]
ExecStart="$prefix/sonyd"
Restart=on-failure
RestartSec=5
UMask=0077
EOF
cat > "$bin/sony-device-center" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
systemctl --user start sony-device-center-local.service
exec "$HOME/.local/lib/sony-device-center/sony-device-center" "$@"
EOF
chmod 755 "$bin/sony-device-center"
ln -sfn "$prefix/sonyctl" "$bin/sonyctl"
cat > "$data/applications/sony-device-center.desktop" <<EOF
[Desktop Entry]
Name=Sony Device Center
Comment=Noise cancelling, Ambient Sound and equalizer for Sony headphones
Exec="$bin/sony-device-center"
Icon=$prefix/icon.png
Terminal=false
Type=Application
Categories=AudioVideo;Audio;
Keywords=Sony;WH-1000XM4;headphones;ambient;ANC;
EOF
systemctl --user daemon-reload
if command -v update-desktop-database >/dev/null; then
  update-desktop-database "$data/applications"
fi
printf 'Installed. Open Sony Device Center from the app launcher, or run %s/sony-device-center\n' "$bin"
