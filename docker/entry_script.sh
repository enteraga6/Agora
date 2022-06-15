#!/bin/bash

echo -e 'intelpython=exclude' | tee /opt/intel/oneapi/renew-config.txt
echo -e '#!/usr/bin/bash\nsource /opt/intel/oneapi/setvars.sh --config="/opt/intel/oneapi/renew-config.txt"' | tee /etc/profile.d/10-inteloneapivars.sh

source /opt/intel/oneapi/setvars.sh --config="/opt/intel/oneapi/renew-config.txt"

cd /opt/FlexRAN-FEC-SDK-19-04/sdk
rm -rf build-avx* 
WIRELESS_SDK_TARGET_ISA="avx2"
export WIRELESS_SDK_TARGET_ISA
./create-makefiles-linux.sh
cd build-avx2-icc
make -j
make install

exec "$@"