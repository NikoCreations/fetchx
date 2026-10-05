# fetchx

A fast C++ terminal downloader with URL monitoring.

## Included commands

### fetchx

Downloads files from HTTP and HTTPS URLs.

```bash
fetchx https://example.com/file.zip
```

Resume a partial download:

```bash
fetchx https://example.com/file.zip --resume
```

Choose the output filename:

```bash
fetchx https://example.com/file.zip --output myfile.zip
```

Download and start monitoring:

```bash
fetchx https://example.com/file.zip --watch
```

Change the monitoring interval:

```bash
fetchx https://example.com/file.zip --watch --interval 30
```

### watchurl

Monitors a remote URL for changes.

```bash
watchurl https://example.com/file.zip
```

Check once:

```bash
watchurl https://example.com/file.zip --once
```

Choose the output file:

```bash
watchurl https://example.com/file.zip --output file.zip
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Test

```bash
./build/fetchx --version
./build/watchurl --version
```

## Homebrew

The Homebrew formula installs both commands with one package:

```bash
brew install fetchx
```
