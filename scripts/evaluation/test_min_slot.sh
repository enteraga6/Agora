#! /bin/bash

set -e

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
hydra_root_dir=$( cd ${script_dir}/../.. >/dev/null 2>&1 && pwd )

source ${hydra_root_dir}/scripts/utils/utils.sh
source ${hydra_root_dir}/scripts/control/init_platform.sh

slot_us=1000

rm -f ${hydra_root_dir}/data/frame_latency_all_0.txt

over=0
run_limit=30
num_run=0
while [ "${over}" == "0" ]; do
    num_run=$(( ${num_run}+1 ))
    if [ "${num_run}" == "${run_limit}" ]; then
        echo "Run time over limit ${run_limit}"
        exit
    fi
    echo "Run Hydra ant $1 ue $2 core $3 for the ${slot_us} us slot size"
    cat ${HYDRA_SYSTEM_CONFIG_JSON} | jq --argjson slot ${slot_us} '.slot_us=$slot' > tmp.json
    mv tmp.json ${HYDRA_SYSTEM_CONFIG_JSON}
    ${hydra_root_dir}/scripts/control/run_all.sh -x || continue
    ${hydra_root_dir}/scripts/evaluation/latency_analysis.sh 0
    if [ ! -f ${hydra_root_dir}/data/frame_latency_all_0.txt ]; then
        echo "Run Hydra failed this time"
        step=$(( ${slot_us}/1000*100 ))
        slot_us=$(( ${slot_us}+${step} ))
        continue
    fi
    over=1
done

echo "Valid ${slot_us} us slot size"