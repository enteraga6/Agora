### This dockerfile sets up an Ubuntu 20.04 environment from scratch 

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

#build gtest=
# RUN cd /usr/src/gtest
# RUN cmake CMakeLists.txt
# RUN make

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

# Set up SoapySDR
RUN ( \
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
RUN ( \
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
RUN ( \
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

#RUN source /opt/intel/oneapi/setvars.sh --config="/opt/intel/oneapi/renew-config.txt"
RUN echo -e 'intelpython=exclude' | tee /opt/intel/oneapi/renew-config.txt
RUN . /opt/intel/oneapi/setvars.sh --config="/opt/intel/oneapi/renew-config.txt"
RUN echo -e '#!/usr/bin/bash\nsource /opt/intel/oneapi/setvars.sh --config="/opt/intel/oneapi/renew-config.txt"' | tee /etc/profile.d/10-inteloneapivars.sh
#Enable the soapy sdr server for more robust device detection
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

# Install FlexRAN FEC SDK

# RUN (WIRELESS_SDK_TARGET_ISA="avx512" && \
#     export WIRELESS_SDK_TARGET_ISA &&\
#     cd /opt/FlexRAN-FEC-SDK-19-04/sdk && \
#     ./create-makefiles-linux.sh && \
#     cd build-avx512-icc &&\
#     make -j &&\
#     make install)
RUN (. /opt/intel/oneapi/setvars.sh --config="/opt/intel/oneapi/renew-config.txt" \
    cd /opt/FlexRAN-FEC-SDK-19-04/sdk \
    rm -rf build-avx* \ 
    WIRELESS_SDK_TARGET_ISA="avx2" \
    export WIRELESS_SDK_TARGET_ISA \
    . create-makefiles-linux.sh \
    cd build-avx2-icc \
    make -j \
    make install)

# RUN ldconfig


# RUN (cd ./agora \
#     chown ${USER}:${GROUP} . \
#     chmod 775 . \
#     mkdir ./agora/build)

#Clone Agora
RUN (git clone --branch develop https://github.com/enteraga6/Agora agora || \
    { echo "Failed to clone git repository: https://github.com/enteraga6/Agora" && exit 1; })

#Build Agora
RUN mkdir /agora/build 

RUN (cd agora \
    cd build \
    cmake .. \
    make -j \
    )

# #Make the saopy settings a lower priority
# RUN mv /usr/local/lib/sysctl.d/SoapySDRServer.conf /usr/local/lib/sysctl.d/98-SoapySDRServer.conf
# #Ethernet buffer sizes
# RUN echo -e '# Ethernet transport tuning\n# Socket Rx Buffer Max Size\nnet.core.rmem_max=536870912\n#Socket Send Buffer Max Size\nnet.core.wmem_max=536870912' | tee /etc/sysctl.d/99-agora.conf
# RUN sysctl --load /etc/sysctl.d/99-agora.conf

# RUN echo -e '#!/usr/bin/bash\nsource /opt/intel/oneapi/setvars.sh --config="/opt/intel/oneapi/renew-config.txt"' | sudo tee /etc/profile.d/10-inteloneapivars.sh
# RUN echo -e '#!/usr/bin/bash\nsudo chown -R $(id -u):$(id -g) /scratch/ > /dev/null 2>&1' | sudo tee /etc/profile.d/11-scratchowner.sh


# RUN echo -e 'intelpython=exclude' | tee /opt/intel/oneapi/renew-config.txt
# RUN echo -e '#!/usr/bin/bash\nsource /opt/intel/oneapi/setvars.sh --config="/opt/intel/oneapi/renew-config.txt"' | tee /etc/profile.d/10-inteloneapivars.sh

# Undo the noninteractive DEBIAN_FRONTEND from the beginning of this file
ENV DEBIAN_FRONTEND teletype

# # Set up the intel MKL compiler variables (shell environment vars) so that Agora can run.
# # This is only really needed when logging into the container via a remote bash shell and manually running commands.
# # Note that this will NOT take effect if a docker CMD or kubernetes command is used directly.
# RUN echo "source /opt/intel/compilers_and_libraries/linux/bin/compilervars.sh intel64" >> ~/.bashrc

