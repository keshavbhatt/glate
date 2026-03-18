# Creating a Glate Release

This repository builds release artifacts automatically on tags using `.github/workflows/release.yml`.

## Trigger a release

```bash
git add .
git commit -m "Release v4.0.0"
git tag -a v4.0.0 -m "Release v4.0.0"
git push origin main v4.0.0
```

## Artifacts produced

- `Glate-Portable-vX.Y.Z-Win64.zip`
- `glate_*.snap`
- `glate.flatpak`
- `checksums.txt`

## Local package build helpers

### Snap

```bash
snapcraft
```

### Flatpak

```bash
./build-flatpak.sh
```

## Notes

- Tag format must be `v<major>.<minor>.<patch>` for release workflow trigger.
- Snap publishing to the Snap Store still requires `snapcraft upload` and store credentials.
- Flathub publishing requires a separate Flathub pull request with `com.ktechpit.glate.yml`.

