#!/usr/bin/env bash
# Sign and notarize built macOS plugins in CI. Expects the Apple secrets in env (see build.yml).
set -euo pipefail
ARTEFACTS="$1"

KEYCHAIN=build.keychain
KEYCHAIN_PW=$(uuidgen)
echo "$APPLE_CERT_P12" | base64 --decode > cert.p12
security create-keychain -p "$KEYCHAIN_PW" $KEYCHAIN
security default-keychain -s $KEYCHAIN
security unlock-keychain -p "$KEYCHAIN_PW" $KEYCHAIN
security import cert.p12 -k $KEYCHAIN -P "$APPLE_CERT_PASSWORD" -T /usr/bin/codesign
security set-key-partition-list -S apple-tool:,apple: -s -k "$KEYCHAIN_PW" $KEYCHAIN
IDENTITY=$(security find-identity -v -p codesigning $KEYCHAIN | grep "Developer ID Application" | head -1 | awk -F'"' '{print $2}')

for bundle in "$ARTEFACTS"/VST3/*.vst3 "$ARTEFACTS"/AU/*.component; do
  [ -e "$bundle" ] || continue
  codesign --force --deep --options runtime --timestamp --sign "$IDENTITY" "$bundle"
  ditto -c -k --keepParent "$bundle" "${bundle}.zip"
  xcrun notarytool submit "${bundle}.zip" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_PASSWORD" --wait
  xcrun stapler staple "$bundle"
  rm "${bundle}.zip"
done
echo "Signed and notarized."
