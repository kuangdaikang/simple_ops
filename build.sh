#!/bin/bash
# ============================================================================
# build.sh - 编译全部13个算子 (4 Vector | 3 Cube | 3 Fusion | 3 CPU)
#
# 用法:
#   bash build.sh                    # 编译
#   bash build.sh clean              # 清理后重新编译
#   ASC_ARCH=dav-3510 bash build.sh  # 指定架构
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
ASC_ARCH="${ASC_ARCH:-dav-2201}"

echo "============================================"
echo "  Ascend Simple Ops Build (13 operators)"
echo "  Architecture: ${ASC_ARCH}"
echo "  Build Dir:    ${BUILD_DIR}"
echo "============================================"

# 检查环境: 探测 CANN toolkit 路径
if [ -z "${ASCEND_TOOLKIT_HOME}" ] && [ -z "${ASCEND_HOME}" ]; then
    echo "[WARN] ASCEND_TOOLKIT_HOME or ASCEND_HOME not set."
    for candidate in \
        "/usr/local/Ascend/ascend-toolkit/latest" \
        "/usr/local/Ascend/latest" \
        "$HOME/Ascend/ascend-toolkit/latest"; do
        if [ -f "${candidate}/set_env.sh" ]; then
            echo "[INFO] Found ${candidate}/set_env.sh, sourcing..."
            source "${candidate}/set_env.sh"
            break
        fi
    done
    if [ -z "${ASCEND_TOOLKIT_HOME}" ] && [ -z "${ASCEND_HOME}" ]; then
        echo "[ERROR] Cannot find CANN toolkit. Please source set_env.sh manually."
        echo "        e.g.: source /usr/local/Ascend/ascend-toolkit/latest/set_env.sh"
        exit 1
    fi
fi

# Clean
if [ "$1" == "clean" ]; then
    echo "[INFO] Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
fi

# Configure
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake .. \
    -DCMAKE_ASC_ARCHITECTURES="${ASC_ARCH}" \
    -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

echo ""
echo "============================================"
echo "  Build SUCCESS!"
echo "  Binaries in: ${BUILD_DIR}"
echo "============================================"
ls -lh "${BUILD_DIR}"/op_*
