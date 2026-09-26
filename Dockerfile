# Use Alpine Linux for a minimal base image
FROM alpine

# The Mojibake library
#
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
COPY cmake/ cmake/

# Copy test files
COPY tests/ tests/
COPY unicode-data/collation/CollationTest/CollationTest_NON_IGNORABLE.txt unicode-data/collation/CollationTest/CollationTest_NON_IGNORABLE.txt
COPY unicode-data/collation/CollationTest/CollationTest_SHIFTED.txt unicode-data/collation/CollationTest/CollationTest_SHIFTED.txt
COPY unicode-data/emoji/emoji-test.txt unicode-data/emoji/emoji-test.txt
COPY unicode-data/idna/IdnaTestV2.txt unicode-data/idna/IdnaTestV2.txt
COPY unicode-data/security/intentional.txt unicode-data/security/intentional.txt
COPY unicode-data/security/confusables.txt unicode-data/security/confusables.txt
COPY unicode-data/UCD/auxiliary/GraphemeBreakTest.txt unicode-data/UCD/auxiliary/GraphemeBreakTest.txt
COPY unicode-data/UCD/auxiliary/LineBreakTest.txt unicode-data/UCD/auxiliary/LineBreakTest.txt
COPY unicode-data/UCD/auxiliary/SentenceBreakTest.txt unicode-data/UCD/auxiliary/SentenceBreakTest.txt
COPY unicode-data/UCD/auxiliary/WordBreakTest.txt unicode-data/UCD/auxiliary/WordBreakTest.txt
COPY unicode-data/UCD/BidiCharacterTest.txt unicode-data/UCD/BidiCharacterTest.txt
COPY unicode-data/UCD/BidiTest.txt unicode-data/UCD/BidiTest.txt
COPY unicode-data/UCD/CaseFolding.txt unicode-data/UCD/CaseFolding.txt
COPY unicode-data/UCD/DerivedNormalizationProps.txt unicode-data/UCD/DerivedNormalizationProps.txt
COPY unicode-data/UCD/NormalizationTest.txt unicode-data/UCD/NormalizationTest.txt
COPY unicode-data/UCD/PropertyValueAliases.txt unicode-data/UCD/PropertyValueAliases.txt
COPY unicode-data/UCD/ScriptExtensions.txt unicode-data/UCD/ScriptExtensions.txt
COPY unicode-data/UCD/SpecialCasing.txt unicode-data/UCD/SpecialCasing.txt

# Build the project
RUN cmake -S . -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DMJB_WARNINGS_AS_ERRORS=ON \
    && cmake --build ${BUILD_DIR}

# Default command to run tests
CMD ["sh", "-c", "${BUILD_DIR}/tests/mojibake-test"]
