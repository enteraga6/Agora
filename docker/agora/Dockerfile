### This dockerfile sets up an Ubuntu 20.04 environment from scratch 
# escape=\
FROM ubuntu:20.04

# Basic set up for ubuntu image
ENV DEBIAN_FRONTEND noninteractive
RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get install -y --no-install-recommends apt-utils gnupg

# Just for convenience purposes
RUN apt-get install -y nano vim gdb

#RUN apt-get -y update && apt-get  install build-essential cmake --no-install-recommends

# Required for downloading and building the source packages below
RUN apt-get -y install -y g++ cmake make wget git

RUN apt update
RUN apt upgrade -y

RUN (apt-get -y install -y \
    git cmake build-essential g++ gcc \
    libgtest-dev libgflags-dev libboost-dev libfftw3-dev \ 
    doxygen swig graphviz qtbase5-dev libcanberra-gtk-module \
    liblapack-dev libopenblas-base libopenblas-dev \ 
    python3 python3-dev libpython3-dev python3-distutils python3-dbg python3-pip python3-numpy python3-matplotlib python3-h5py python3-scipy python3-testresources python3-sip-dev python-is-python3 \
    software-properties-common avahi-daemon libavahi-client-dev \
    liblapack3 liblapack-doc liblapack-dev libsuperlu-dev libsuperlu-doc libsuperlu5 libarpack++2c2a libarpack2 libarpack2-dev libatlas-base-dev libatlas3-base libhdf5-dev \
    )

#Remove Warnings
RUN pip3 install --upgrade secretstorage

# Install gtest and gflags
RUN apt-get install -y libgtest-dev
RUN cd /usr/src/gtest && \
    cmake . && \ 
    make && \
    mv lib/libgtest* /usr/lib/

#Install inteloneAPI
# use wget to fetch the Intel repository public key
RUN cd /tmp
RUN wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
# add to your apt sources keyring so that archives signed with this key will be trusted.
RUN apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB
# remove the public key
RUN rm GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB

RUN add-apt-repository "deb https://apt.repos.intel.com/oneapi all main"
RUN apt install -y intel-hpckit

# Build and install armadillo
RUN wget https://sourceforge.net/projects/arma/files/armadillo-11.1.1.tar.xz
RUN tar xf armadillo-11.1.1.tar.xz
RUN (cd armadillo-11.1.1; cmake .; make -j8; make install)
RUN rm -rf armadillo*

RUN mkdir Agora_dir;

# Set up SoapySDR
RUN ( cd Agora_dir; \
    git clone --branch soapy-sdr-0.8.1 --depth 1 --single-branch https://github.com/pothosware/SoapySDR.git;\
    cd SoapySDR; \
    mkdir build; \
    cd build; \
    cmake ..; \
    make -j8; \
    make install; \
    ldconfig; \
    )

#SoapyRemote
RUN ( cd Agora_dir;\
    git clone --branch soapy-remote-0.5.2 --depth 1 --single-branch https://github.com/pothosware/SoapyRemote.git; \
    cd SoapyRemote; \
    mkdir build; \
    cd build; \
    cmake ..; \
    make -j8; \
    make install; \
    ldconfig; \
    )

#Iris drivers
RUN ( cd Agora_dir;\
    git clone --branch rx-shorttimeout --depth 1 --single-branch https://github.com/Agora-wireless/sklk-soapyiris.git; \
    cd sklk-soapyiris; \
    mkdir -p build; \
    cd build; \
    cmake ..;\
    make -j`nproc`;\
    make install;\
    cd ../..;\
    ldconfig; \
    )

RUN systemctl enable SoapySDRServer

# Install FlexRAN FEC SDK
RUN (cd /opt/  \
    mkdir FlexRAN-FEC-SDK-19-04)

#Have to be in docker folder for it to work
COPY ./FlexRAN-FEC-SDK-19-04 ./opt/FlexRAN-FEC-SDK-19-04

RUN (cd /opt/ \
    chmod -R a+rwX FlexRAN-FEC-SDK-19-04/ \
    cd FlexRAN-FEC-SDK-19-04 \
    rm -rf .git \
    rm .gitignore)

#Clone Agora
RUN (cd Agora_dir; \
    git clone --branch develop https://github.com/enteraga6/Agora agora || \
    { echo "Failed to clone git repository: https://github.com/enteraga6/Agora" && exit 1; })

EXPOSE 8000-8007/udp


# # Install FlexRAN FEC SDK
COPY ./entry_script.sh ./opt
RUN ["chmod", "+x", "./opt/entry_script.sh"]

ENTRYPOINT ["/bin/bash", "-c", "./opt/entry_script.sh"]

# Undo the noninteractive DEBIAN_FRONTEND from the beginning of this file
ENV DEBIAN_FRONTEND teletype