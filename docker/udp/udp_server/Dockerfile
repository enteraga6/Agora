FROM ubuntu:20.04

RUN apt update
RUN apt upgrade -y

RUN apt-get update
RUN apt-get -y install vim nano
RUN apt-get update && apt-get -y install cmake protobuf-compiler

RUN (apt-get -y install -y \
    git cmake build-essential g++ gcc \
    libgtest-dev libgflags-dev libboost-dev libfftw3-dev \ 
    doxygen swig graphviz qtbase5-dev libcanberra-gtk-module \
    liblapack-dev libopenblas-base libopenblas-dev \ 
    python3 python3-dev libpython3-dev python3-distutils python3-dbg python3-pip python3-numpy python3-matplotlib python3-h5py python3-scipy python3-testresources python3-sip-dev python-is-python3 \
    software-properties-common avahi-daemon libavahi-client-dev \
    liblapack3 liblapack-doc liblapack-dev libsuperlu-dev libsuperlu-doc libsuperlu5 libarpack++2c2a libarpack2 libarpack2-dev libatlas-base-dev libatlas3-base libhdf5-dev \
    )

RUN apt-get install -y libgtest-dev
RUN cd /usr/src/gtest && \
    cmake . && \ 
    make && \
    mv lib/libgtest* /usr/lib/

EXPOSE 8080

COPY ./server.cc .
COPY ./CMakeLists.txt .

RUN cmake .

RUN make

CMD ["bin/bash"]
#CMD ["/bin/bash","-c","gdb udpserver"]