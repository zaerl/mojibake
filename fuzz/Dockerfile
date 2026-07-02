# Use Ubuntu for a base image with a libFuzzer-capable clang
FROM ubuntu:24.04

# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

# Build and run the libFuzzer harness. Build from the repository root:
#   docker build -f fuzz/Dockerfile -t mojibake-fuzz .
#   docker run mojibake-fuzz
#   docker run -e FUZZ_TIME=300 mojibake-fuzz

# Seconds to fuzz on each run.
ENV FUZZ_TIME=60

# clang provides the compiler and linker; libclang-rt-dev provides libFuzzer and the
# AddressSanitizer/UBSan runtime libraries.
RUN apt-get update \
    && apt-get install -y --no-install-recommends clang libclang-rt-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the library sources and the fuzz harness with its seed corpus.
COPY src/ src/
COPY fuzz/ fuzz/

# Build the fuzzer with the sanitizers enabled.
RUN clang -g -O1 -std=c17 -fsanitize=fuzzer,address,undefined \
    -fno-sanitize-recover=undefined -Isrc \
    -o mojibake-fuzz fuzz/fuzz.c src/*.c

# Fuzz for FUZZ_TIME seconds, seeded from the committed corpus.
CMD ["sh", "-c", "./mojibake-fuzz -max_total_time=${FUZZ_TIME} fuzz/corpus"]
