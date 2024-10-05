@fmt:
  clang-format -i main.c

@build:
  clang -o ferment main.c

@test: build
  ./ferment test-ips.txt > test.out && \
     wc --libxo json -l test.out | \
     jq --exit-status '.wc.file[0].lines == 1303'
