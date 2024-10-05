@fmt:
  clang-format -i main.c

@build:
  clang -O3 -o ferment main.c

@release:
  rm -f bin/ferment*
  clang -O3 -target x86_64-apple-macos10.12 -o bin/ferment-darwin-x86_64 main.c
  clang -O3 -target arm64-apple-macos11 -o bin/ferment-darwin-aarch64 main.c
  lipo -create -output bin/ferment-darwin-universal bin/ferment-darwin-x86_64 bin/ferment-darwin-aarch64
  codesign --force --verify --verbose --sign "${APPLE_DEV_ID}" bin/ferment-darwin-universal
  zig build
  zip bin/ferment-darwin-x86_64.zip bin/ferment-darwin-x86_64
  zip bin/ferment-darwin-aarch64.zip bin/ferment-darwin-aarch64
  zip bin/ferment-darwin-universal.zip bin/ferment-darwin-universal


@test: build
  ./ferment test-ips.txt > test.out
  diff test.out correct-output.txt || echo "Test failed"
  rm -rf test.out
