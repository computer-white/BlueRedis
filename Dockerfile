# 使用 Ubuntu 22.04 作为基础镜像
FROM ubuntu:22.04

# 设置环境变量
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    g++-12 \
    cmake \
    git \
    libboost-iostreams-dev \
    libboost-coroutine-dev \
    libboost-context-dev \
    libssl-dev \
    zlib1g-dev \
    libmysqlclient-dev \
    libhiredis-dev \
    libgumbo-dev \
    libidn2-dev \
    ragel \
    libyaml-cpp-dev \
    pkg-config \
    nlohmann-json3-dev \
    libevent-dev \
    libabsl-dev \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
    libyaml-cpp0.7 \
    libidn2-0 \
    libunistring2 \
    libboost-iostreams1.74.0 \
    libboost-coroutine1.74.0 \
    libboost-context1.74.0 \
    libhiredis0.14 \
    libevent-2.1-7 \
    libmysqlclient21 \
    libgumbo1 \
    zlib1g \
    && rm -rf /var/lib/apt/lists/*

ENV LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:/usr/local/lib

WORKDIR /app

COPY . .

RUN rm -rf build && mkdir build && cd build && cmake .. && make -j$(nproc)

# 创建数据目录
RUN mkdir -p /data

# 暴露端口
EXPOSE 6666 6667

WORKDIR /app/build

ENTRYPOINT ["/app/bin/test_commandHandler"]
CMD ["0.0.0.0", "6666"]