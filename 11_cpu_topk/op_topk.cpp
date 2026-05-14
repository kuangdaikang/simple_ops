/**
 * @file op_topk.cpp
 * @brief [CPU-Only] 算子11: TopK — 从向量中选取最大的K个值
 *
 * 类别: 纯CPU — 运行在Host CPU上 (昇腾卡上的CPU核心)
 * TopK涉及排序和不规则内存访问，不适合Vector/Cube核心。
 * 本算子在Host端完成计算，仅使用ACL管理设备内存。
 */

#include <algorithm>
#include <vector>
#include "acl/acl.h"
#include "acl/acl_op.h"
#include "../common/data_utils.h"

int main()
{
    constexpr uint32_t N = 10240;
    constexpr uint32_t K = 10;
    size_t byteSize = N * sizeof(float);

    aclInit(nullptr);
    aclrtSetDevice(0);
    aclrtStream stream;
    aclrtCreateStream(&stream);

    float *hostIn, *hostVal, *hostIdx;
    void *devBuf;
    aclrtMallocHost((void**)&hostIn, byteSize);
    aclrtMallocHost((void**)&hostVal, K * sizeof(float));
    aclrtMallocHost((void**)&hostIdx, K * sizeof(float));
    aclrtMalloc(&devBuf, byteSize, ACL_MEM_MALLOC_HUGE_FIRST);

    size_t fs;
    ReadFile("../data/input_a.bin", fs, hostIn, byteSize);
    aclrtMemcpy(devBuf, byteSize, hostIn, byteSize, ACL_MEMCPY_HOST_TO_DEVICE);

    INFO_LOG("========== [CPU] TopK N=%u K=%u ==========", N, K);

    // Warmup: 从设备拷回后CPU计算
    aclrtMemcpy(hostIn, byteSize, devBuf, byteSize, ACL_MEMCPY_DEVICE_TO_HOST);

    double t0 = GetTimeUs();

    // === CPU 核心计算: TopK ===
    std::vector<std::pair<float, uint32_t>> indexed;
    indexed.reserve(N);
    for (uint32_t i = 0; i < N; i++)
        indexed.emplace_back(hostIn[i], i);
    std::partial_sort(indexed.begin(), indexed.begin() + K, indexed.end(),
        [](auto &a, auto &b) { return a.first > b.first; });
    for (uint32_t i = 0; i < K; i++) {
        hostVal[i] = indexed[i].first;
        hostIdx[i] = (float)indexed[i].second;
    }
    // === CPU 计算结束 ===

    double t1 = GetTimeUs();
    INFO_LOG("CPU time: %.2f us", t1 - t0);

    for (uint32_t i = 0; i < K; i++)
        INFO_LOG("  Top[%u]: val=%.4f  idx=%.0f", i, hostVal[i], hostIdx[i]);

    // 验证: 用 std::nth_element 得到第K大
    std::nth_element(hostIn, hostIn + K, hostIn + N, std::greater<float>());
    float kth = hostIn[K - 1];
    bool ok = (hostVal[K - 1] >= kth - 1e-4f);
    INFO_LOG("Verification: %s (kth=%.4f, topK_min=%.4f)", ok ? "PASS" : "FAIL", kth, hostVal[K - 1]);

    aclrtFree(devBuf);
    aclrtFreeHost(hostIn); aclrtFreeHost(hostVal); aclrtFreeHost(hostIdx);
    aclrtDestroyStream(stream);
    aclrtResetDevice(0);
    aclFinalize();
    return ok ? 0 : 1;
}
