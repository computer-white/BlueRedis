FROM ubuntu:24.04 AS builder
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    gcc-13 g++-13 cmake make ragel pkg-config \
    libboost-all-dev libssl-dev libyaml-cpp-dev \
    libhiredis-dev libgtest-dev libgmock-dev \
    libabsl-dev nlohmann-json3-dev libidn2-dev \
    libgumbo-dev default-libmysqlclient-dev zlib1g-dev
WORKDIR /app
COPY . .
RUN cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++-13 \
    -DCMAKE_C_COMPILER=gcc-13
RUN cmake --build build -j$(nproc)

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    libhiredis1.1.0 \
    libboost-system1.83.0 libboost-filesystem1.83.0 \
    libboost-iostreams1.83.0 \
    libssl3t64 libyaml-cpp0.8 \
    libmysqlclient21 \
    libabsl20220623t64 libmariadb3 libgumbo2
COPY --from=builder /app/bin/httpserver /usr/local/bin/
EXPOSE 8082
CMD ["/usr/local/bin/httpserver"]