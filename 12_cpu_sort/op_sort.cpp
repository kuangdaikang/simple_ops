/**
 * @file op_sort.cpp
 * @brief [CPU-Only] 算子12: 向量排序 Sort — 对浮点数组升序排序
 *
 * 类别: 纯CPU — 排序需要全局比较，GPU/NPU上效率不如CPU。
 * 本算子在Host端完成O(N log N)排序。
 */

#include <algorithm>
#include "acl/acl.h"
#include "../common/data_utils.h"

int main()
{
    constexpr uint32_t N = 4096 * 9;
    size_t byteSize = N * sizeof(float);

    aclInit(nullptr);
    aclrtSetDevice(0);
    aclrtStream stream;
    aclrtCreateStream(&stream);

    float *hostData, *hostCopy;
    uint8_t *devBuf = nullptr;
    aclrtMallocHost((void**)&hostData, byteSize);
    aclrtMallocHost((void**)&hostCopy, byteSize);
    aclrtMalloc((void**)&devBuf, byteSize, ACL_MEM_MALLOC_HUGE_FIRST);

    size_t fs;
    ReadFile("../data/input_a.bin", fs, hostData, byteSize);
    aclrtMemcpy(devBuf, byteSize, hostData, byteSize, ACL_MEMCPY_HOST_TO_DEVICE);
    aclrtMemcpy(hostData, byteSize, devBuf, byteSize, ACL_MEMCPY_DEVICE_TO_HOST);

    // 保存副本用于验证
    std::copy(hostData, hostData + N, hostCopy);

    INFO_LOG("========== [CPU] Sort N=%u ==========", N);

    double t0 = GetTimeUs();
    std::sort(hostData, hostData + N);
    double t1 = GetTimeUs();
    INFO_LOG("CPU time: %.2f us", t1 - t0);

    // 验证: 检查是否有序
    bool ok = true;
    for (uint32_t i = 1; i < N; i++) {
        if (hostData[i] < hostData[i - 1]) { ok = false; break; }
    }
    INFO_LOG("Monotonic: %s", ok ? "PASS" : "FAIL");

    // 写回排序结果
    aclrtMemcpy(devBuf, byteSize, hostData, byteSize, ACL_MEMCPY_HOST_TO_DEVICE);

    aclrtFree(devBuf);
    aclrtFreeHost(hostData); aclrtFreeHost(hostCopy);
    aclrtDestroyStream(stream);
    aclrtResetDevice(0);
    aclFinalize();
    return ok ? 0 : 1;
}
