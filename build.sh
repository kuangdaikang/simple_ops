#!/bin/bash
# ============================================================================
# build.sh - 编译所有10个简单昇腾算子
#
# 用法:
#   bash build.sh              # 编译
#   bash build.sh clean        # 清理后重新编译
#   ASC_ARCH=dav-3510 bash build.sh  # 指定架构
# ============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
ASC_ARCH="${ASC_ARCH:-dav-2201}"

echo "============================================"
echo "  Ascend Simple Ops Build"
echo "  Architecture: ${ASC_ARCH}"
echo "  Build Dir:    ${BUILD_DIR}"
echo "============================================"

# 检查环境
if [ -z "${ASCEND_TOOLKIT_HOME}" ] && [ -z "${ASCEND_HOME}" ]; then
    echo "[WARN] ASCEND_TOOLKIT_HOME or ASCEND_HOME not set."
    echo "       Trying default paths..."
    if [ -d "/usr/local/Ascend/latest" ]; then
        export ASCEND_TOOLKIT_HOME="/usr/local/Ascend/latest"
        echo "[INFO] Found /usr/local/Ascend/latest"
    elif [ -d "$HOME/Ascend/ascend-toolkit/latest" ]; then
        export ASCEND_TOOLKIT_HOME="$HOME/Ascend/ascend-toolkit/latest"
        echo "[INFO] Found $HOME/Ascend/ascend-toolkit/latest"
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
