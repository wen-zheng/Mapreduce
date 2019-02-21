FROM ubuntu:16.04

RUN apt update -qq && apt upgrade -y
# some base packages
RUN apt update -qq && apt install -y git python lsb-release sudo wget mercurial build-essential
# Install and build the ns-3-dev environment
RUN wget https://www.nsnam.org/release/ns-allinone-3.27.tar.bz2
RUN bzip2 -d ns-allinone-3.27.tar.bz2
RUN tar -xvf ns-allinone-3.27.tar
# make sure ns-3-dev compiles successfully
WORKDIR ns-allinone-3.27/ns-3.27
# Download TCP Cubic NS-3 module
RUN wget http://perform.wpi.edu/downloads/cubic/ns-3.27.tar.gz
RUN tar -xvf ns-3.27.tar.gz
RUN cp ns-3.27_cubic/src/internet/model/* src/internet/model/
RUN cp ns-3.27_cubic/src/internet/wscript src/internet/wscript
RUN cp ns-3.27_cubic/src/internet/test/tcp-cubic-test-suite.cc src/internet/test/
RUN ./waf configure --enable-tests --enable-examples
RUN ./waf build

# build deps from proto-quic repository
RUN mkdir -p /.ssh/
RUN echo -e "Host gitlab.com\n\tStrictHostKeyChecking no\n" >> /.ssh/config
RUN git clone --depth 1 https://gitlab.com/diegoamc/proto-quic.git /proto-quic
ENV PROTO_QUIC_ROOT "/proto-quic/src"
ENV PATH "$PATH:/proto-quic/depot_tools"
RUN bash -c "/proto-quic/proto_quic_tools/sync.sh"
RUN echo "deb-src http://archive.ubuntu.com/ubuntu/ xenial multiverse" >> /etc/apt/sources.list
RUN echo "deb-src http://archive.ubuntu.com/ubuntu/ xenial-updates multiverse" >> /etc/apt/sources.list
# running separate because the user has to agree to an EULA
RUN echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | debconf-set-selections
RUN apt update -qq && (yes | apt install --assume-yes -y ttf-mscorefonts-installer)
RUN apt-get clean && apt-get update && apt-get install -y locales
RUN locale-gen en_US.UTF-8
RUN bash -c "printf 'N\n' | /proto-quic/src/build/install-build-deps.sh"

RUN git clone https://gitlab.com/diegoamc/ns-3-quic-module.git src/quic
RUN ./waf clean
RUN ./waf configure --enable-examples --enable-tests
# absolute path to the quic model libs
RUN ./waf build
ENV LD_LIBRARY_PATH "/ns-allinone-3.27/ns-3.27/src/quic/model/libs"
ENTRYPOINT ["./waf", "--run", "quic-example"]
