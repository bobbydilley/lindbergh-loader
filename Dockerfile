FROM ubuntu:22.04 AS lindbergh-build

RUN dpkg --add-architecture i386 \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        freeglut3-dev:i386 \
        freeglut3:i386 \
        gcc-multilib \
        libglew-dev \
        libopenal-dev:i386 \
        libopenal1:i386 \
        libstdc++5:i386 \
        libxmu6:i386 \
        wget \
        xorg-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /xenial-libs
RUN wget -q http://launchpadlibrarian.net/534757982/multiarch-support_2.23-0ubuntu11.3_i386.deb \
    && dpkg -i multiarch-support_2.23-0ubuntu11.3_i386.deb
RUN wget -q http://launchpadlibrarian.net/184146495/libalut0_1.1.0-5_i386.deb \
    && dpkg -i libalut0_1.1.0-5_i386.deb
RUN wget -q http://launchpadlibrarian.net/184146496/libalut-dev_1.1.0-5_i386.deb \
    && dpkg -i libalut-dev_1.1.0-5_i386.deb

WORKDIR /lindbergh-loader
COPY . .

RUN make

# Output binaries of build using the --output=PATH argument
FROM scratch AS binaries
COPY --from=lindbergh-build /lindbergh-loader/build/* /
