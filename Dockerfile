FROM ubuntu:22.04

# Install build and debug tools
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        g++ \
        make \
        gdb \
        valgrind && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy all project files
COPY . .

# Build the executable
RUN make -B

CMD ["./taskforge"]
