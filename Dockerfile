FROM ubuntu:22.04 as builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-iostreams-dev \
    libboost-program-options-dev \
    libbz2-dev \
    libhts-dev \
    libz-dev
    
WORKDIR /app
COPY . .

RUN /bin/bash

RUN make clean
RUN make -j2 HTSLIB_LIB="-lhts" DYN_LIBS="-lm -lpthread"

FROM ubuntu:22.04 as runner
RUN apt-get update && apt-get install -y \
    libhts3

WORKDIR /app
COPY --from=builder /app/bin/shapeit4.2 /app/shapeit4

CMD /app/shapeit4
