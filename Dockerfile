# Use Alpine Linux for a minimal base image
FROM alpine

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
COPY mojibake.db .

# Copy test files
COPY tests/ tests/
COPY utils/generate/UCD/NormalizationTest.txt utils/generate/UCD/NormalizationTest.txt
COPY utils/generate/UCD/SpecialCasing.txt utils/generate/UCD/SpecialCasing.txt

# Build the project
RUN cmake -S . -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    && cmake --build ${BUILD_DIR}

# Default command to run tests
CMD ["sh", "-c", "WRD_DB_PATH=./mojibake.db ${BUILD_DIR}/tests/mojibake-test"]
