FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive # ignore user input required
# Install required build dependencies
RUN apt-get -y update && apt-get install -y
RUN apt-get -y install \
    g++ \
    cmake \
    git \
    libjsoncpp-dev \
    zlib1g-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    curl \
    libxml2 \
    libc-dev \
    libpulse-dev \
    ca-certificates \
    build-essential \
    autoconf \
    g++ \
    gcc \
    make \
    apt-utils \
    vim \
    wget \
    mysql-client \
    xz-utils \
    file \
    mlocate \
    unzip \
    git \
    libopencv-dev
COPY . .
WORKDIR .
# Run cmake configure & build process
RUN mkdir ./build
RUN cmake -B ./build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_ONLY="sqs;s3;dynamodb"
RUN cmake -B ./build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_ONLY="sqs;s3;dynamodb"
RUN make -j4 -l4 -C ./build
# Launch built application
CMD ["./build/objectcut"]
