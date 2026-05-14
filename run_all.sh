#!/bin/bash
# ============================================================================
# run_all.sh - 运行全部13个算子并收集性能数据
#   4 Vector | 3 Cube | 3 Fusion | 3 CPU
#
# 用法:
#   bash run_all.sh              # 普通运行 + 计时
#   bash run_all.sh profile      # msprof trace
# ============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DATA_DIR="${SCRIPT_DIR}/data"
PROF_DIR="${SCRIPT_DIR}/profiling"
RESULTS_FILE="${SCRIPT_DIR}/perf_results.txt"

# 13个算子: bin_name:dir_name:description:type
OPS=(
    "op_add:01_vector_add:VectorAdd C=A+B:Vector"
    "op_relu:02_vector_relu:ReLU B=max(0,A):Vector"
    "op_sigmoid:03_vector_sigmoid:Sigmoid B=1/(1+e^-A):Vector"
    "op_abs:04_vector_abs:Abs B=|A|:Vector"
    "op_matmul:05_cube_matmul:MatMul 64x64x64:Cube"
    "op_matmul_bias:06_cube_matmul_bias:MatMulTransB 64x64x64:Cube"
    "op_matmul_large:07_cube_matmul_large:MatMul 128x128x128(tiled):Cube"
    "op_matmul_relu:08_fusion_matmul_relu:MatMul+ReLU:Fusion"
    "op_matmul_bias_relu:09_fusion_matmul_bias_relu:MatMul+Bias+ReLU:Fusion"
    "op_matmul_add:10_fusion_matmul_add:MatMul+ResidualAdd:Fusion"
    "op_topk:11_cpu_topk:Top-K (CPU):CPU"
    "op_sort:12_cpu_sort:Sort (CPU):CPU"
    "op_nms:13_cpu_nms:NMS IoU=0.5 (CPU):CPU"
)

MODE="${1:-run}"

echo "============================================"
echo "  Ascend Simple Ops - Run All (13 ops)"
echo "  Mode: ${MODE}"
echo "  Time: $(date)"
echo "============================================"

# 检查数据文件
if [ ! -f "${DATA_DIR}/input_a.bin" ]; then
    echo "[ERROR] Data files not found. Run: python3 scripts/gen_data.py"
    exit 1
fi
if [ ! -f "${BUILD_DIR}/op_add" ]; then
    echo "[ERROR] Binaries not found. Run: bash build.sh"
    exit 1
fi

echo "# Ascend Simple Ops Performance Results" > "${RESULTS_FILE}"
echo "# Date: $(date)" >> "${RESULTS_FILE}"
echo "# Format: op_name | core_type | time_us | status" >> "${RESULTS_FILE}"
echo "" >> "${RESULTS_FILE}"

PASS=0; FAIL=0

for op_entry in "${OPS[@]}"; do
    IFS=':' read -r bin_name dir_name desc otype <<< "$op_entry"

    echo "--- ${dir_name}: ${desc} [${otype}] ---"
    cd "${BUILD_DIR}"

    if [ "${MODE}" == "profile" ] && [ "${otype}" != "CPU" ]; then
        mkdir -p "${PROF_DIR}/${dir_name}"
        msprof --application="./${bin_name}" --output="${PROF_DIR}/${dir_name}" \
            --aic-metrics=Memory,MemoryL0,MemoryUB,L2Cache,PipeUtilization \
            --aicpu-profiling=on --sys-cpu-profiling=on 2>&1 | tail -3
        echo "  [PROFILE] -> ${PROF_DIR}/${dir_name}/"
    else
        OUTPUT=$(./"${bin_name}" 2>&1) || true
        RET=$?
        echo "${OUTPUT}"
        KT=$(echo "${OUTPUT}" | grep -E "Kernel time:|CPU time:" | awk '{print $(NF-1)}' | head -1)
        if [ -n "${KT}" ]; then
            echo "${dir_name} | ${otype} | ${KT} | $([ $RET -eq 0 ] && echo 'PASS' || echo 'FAIL')" >> "${RESULTS_FILE}"
        fi
        [ $RET -eq 0 ] && PASS=$((PASS+1)) || FAIL=$((FAIL+1))
    fi
    echo ""
done

echo "============================================"
echo "  Summary: ${PASS} PASS, ${FAIL} FAIL"
echo "  Results: ${RESULTS_FILE}"
echo "============================================"
