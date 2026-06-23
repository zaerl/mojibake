# Use Alpine Linux for a minimal base image
FROM alpine

# The Mojibake library
# This file is distributed under the MIT License. See LICENSE for details.

# Set environment variables
ENV BUILD_DIR=/app/build
ENV BUILD_TYPE=Release

# Install system dependencies
RUN apk add --no-cache \
    build-base \
    cmake

# Set working directory
WORKDIR /app

# Copy source files
COPY src/ src/
COPY CMakeLists.txt .

# Copy test files
COPY tests/ tests/
COPY utils/generate/unicode-data/collation/CollationTest/CollationTest_NON_IGNORABLE_SHORT.txt utils/generate/unicode-data/collation/CollationTest/CollationTest_NON_IGNORABLE_SHORT.txt
COPY utils/generate/unicode-data/collation/CollationTest/CollationTest_SHIFTED_SHORT.txt utils/generate/unicode-data/collation/CollationTest/CollationTest_SHIFTED_SHORT.txt
COPY utils/generate/unicode-data/UCD/auxiliary/GraphemeBreakTest.txt utils/generate/unicode-data/UCD/auxiliary/GraphemeBreakTest.txt
COPY utils/generate/unicode-data/UCD/auxiliary/LineBreakTest.txt utils/generate/unicode-data/UCD/auxiliary/LineBreakTest.txt
COPY utils/generate/unicode-data/UCD/auxiliary/SentenceBreakTest.txt utils/generate/unicode-data/UCD/auxiliary/SentenceBreakTest.txt
COPY utils/generate/unicode-data/UCD/auxiliary/WordBreakTest.txt utils/generate/unicode-data/UCD/auxiliary/WordBreakTest.txt
COPY utils/generate/unicode-data/UCD/BidiCharacterTest.txt utils/generate/unicode-data/UCD/BidiCharacterTest.txt
COPY utils/generate/unicode-data/UCD/NormalizationTest.txt utils/generate/unicode-data/UCD/NormalizationTest.txt
COPY utils/generate/unicode-data/UCD/SpecialCasing.txt utils/generate/unicode-data/UCD/SpecialCasing.txt

# Build the project
RUN cmake -S . -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    && cmake --build ${BUILD_DIR}

# Default command to run tests
CMD ["sh", "-c", "${BUILD_DIR}/tests/mojibake-test"]
