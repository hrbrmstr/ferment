@fmt:
  clang-format -i main.c

@build:
  clang -o ferment main.c

@test: build
  ./ferment test-ips.txt > test.out
  diff test.out correct-output.txt || echo "Test failed"
  rm -rf test.out
