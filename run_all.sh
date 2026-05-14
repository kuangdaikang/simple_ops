#!/bin/bash
# ============================================================================
# run_all.sh - 运行全部13个算子并收集性能数据
#   4 Vector | 3 Cube | 3 Fusion | 3 CPU
#
# 用法:
#   bash run_all.sh                    # 普通运行 (默认卡0)
#   bash run_all.sh profile            # msprof trace
#   bash run_all.sh --device 1         # 指定用卡1运行
#   bash run_all.sh profile --device 1 # 卡1 + profiling
#
# 多卡环境: 通过 ASCEND_RT_VISIBLE_DEVICES 隔离, 确保只用一张卡
# ============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
DATA_DIR="${SCRIPT_DIR}/data"
PROF_DIR="${SCRIPT_DIR}/profiling"
RESULTS_FILE="${SCRIPT_DIR}/perf_results.txt"

# 解析参数: device 默认为 0
DEVICE_ID=0
MODE="run"
for arg in "$@"; do
    case "$arg" in
        --device) DEVICE_ID=""; next_is_device=1 ;;
        --device=*) DEVICE_ID="${arg#*=}" ;;
        profile) MODE="profile" ;;
        *) 
            if [ "$next_is_device" = "1" ]; then
                DEVICE_ID="$arg"
                next_is_device=0
            fi
            ;;
    esac
done

# 隔离多卡: 只让 ACL 看到指定的一张卡
export ASCEND_RT_VISIBLE_DEVICES="${DEVICE_ID}"

echo "============================================"
echo "  Ascend Simple Ops - Run All (13 ops)"
echo "  Device: ${DEVICE_ID} (ASCEND_RT_VISIBLE_DEVICES=${ASCEND_RT_VISIBLE_DEVICES})"
echo "  Mode:   ${MODE}"
echo "  Time:   $(date)"
echo "============================================"

# 显示卡信息
if command -v npu-smi &>/dev/null; then
    echo ""
    npu-smi info -t phyID -i "${DEVICE_ID}" 2>/dev/null | head -10 || true
    echo ""
fi

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
