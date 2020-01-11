#!/bin/bash
#SBATCH --job-name=nbody_cali
#SBATCH --output=test_cali.txt
#SBATCH --time=8:00:00


module load use.own
module load caliper-x86

export CALI_CONFIG_FILE=$CALI_DIR/configs/papi_base.conf

export CALI_REPORT_FILENAME=${out_dir}/cali_${metric}.json
export CALI_PAPI_COUNTERS=${metric}

#declare -a metrics=("PAPI_TOT_CYC")

# L1
declare -a metrics=("PAPI_TOT_CYC" \
                    "IDQ_UOPS_NOT_DELIVERED:CORE"\
                    "UOPS_ISSUED:ANY"\
                    "UOPS_RETIRED:RETIRE_SLOTS"\
                    "INT_MISC:RECOVERY_CYCLES"\
                   )

# Backend L2                    
# declare -a metrics=("CYCLE_ACTIVITY:STALLS_TOTAL"\
#                    "CYCLE_ACTIVITY:STALLS_MEM_ANY"\
#                    "RESOURCE_STALLS:SB"\
#                   )

# Retiring L2
# declare -a metrics=("IDQ:MS_UOPS")

# Retiring L3
# declare -a metrics=("PAPI_NATIVE_INST_RETIRED:ALL"\
#                     "FP_ARITH_INST_RETIRED:128B_PACKED_DOUBLE"\
#                     "FP_ARITH_INST_RETIRED:256B_PACKED_DOUBLE"\
#                     "FP_ARITH_INST_RETIRED:512B_PACKED_DOUBLE"\
#                     "FP_ARITH_INST_RETIRED:SCALAR_DOUBLE"\
#                    )

#declare -a metrics=("PAPI_TOT_CYC" "PAPI_VEC_DP" "PAPI_DP_OPS")

out_dir=cali_original

mkdir ${out_dir}
exe="./build/pennant test/leblancbig/leblancbig.pnt"

export OMP_PROC_BIND=close
export OMP_PLACES=cores

for metric in "${metrics[@]}"
do    
        
    export CALI_REPORT_FILENAME=${out_dir}/cali_${metric}.json
    export CALI_PAPI_COUNTERS=${metric}
    ${exe}

done




