/**
 * @file op_nms.cpp
 * @brief [CPU-Only] 算子13: Non-Maximum Suppression (NMS)
 *
 * 类别: 纯CPU — NMS是目标检测中去除冗余框的核心操作。
 * 涉及动态IOU计算和条件删除，非常不适合GPU/NPU向量化。
 *
 * 输入: N个框 [x1,y1,x2,y2,score], IoU阈值
 * 输出: 保留的框索引
 */

#include <algorithm>
#include <vector>
#include <numeric>
#include "acl/acl.h"
#include "../common/data_utils.h"

struct Box { float x1, y1, x2, y2, score; uint32_t idx; };

static float IoU(const Box &a, const Box &b)
{
    float ix1 = std::max(a.x1, b.x1), iy1 = std::max(a.y1, b.y1);
    float ix2 = std::min(a.x2, b.x2), iy2 = std::min(a.y2, b.y2);
    float iw = std::max(0.f, ix2 - ix1), ih = std::max(0.f, iy2 - iy1);
    float inter = iw * ih;
    float areaA = (a.x2 - a.x1) * (a.y2 - a.y1);
    float areaB = (b.x2 - b.x1) * (b.y2 - b.y1);
    return inter / (areaA + areaB - inter + 1e-8f);
}

int main()
{
    constexpr uint32_t N = 1024;
    constexpr float IOU_THRESH = 0.5f;
    size_t boxSize = N * 5 * sizeof(float);  // 5 values per box

    aclInit(nullptr);
    aclrtSetDevice(0);
    aclrtStream stream;
    aclrtCreateStream(&stream);

    float *hostBoxes;
    void *devBuf;
    aclrtMallocHost((void**)&hostBoxes, boxSize);
    aclrtMalloc(&devBuf, boxSize, ACL_MEM_MALLOC_HUGE_FIRST);

    size_t fs;
    ReadFile("../data/input_a.bin", fs, hostBoxes, boxSize);
    aclrtMemcpy(devBuf, boxSize, hostBoxes, boxSize, ACL_MEMCPY_HOST_TO_DEVICE);
    aclrtMemcpy(hostBoxes, boxSize, devBuf, boxSize, ACL_MEMCPY_DEVICE_TO_HOST);

    INFO_LOG("========== [CPU] NMS N=%u IoU=%.1f ==========", N, IOU_THRESH);

    // 构建框列表
    std::vector<Box> boxes(N);
    for (uint32_t i = 0; i < N; i++) {
        float *b = hostBoxes + i * 5;
        boxes[i] = {std::min(b[0],b[2]), std::min(b[1],b[3]),
                    std::max(b[0],b[2]), std::max(b[1],b[3]), b[4], i};
    }

    double t0 = GetTimeUs();

    // === CPU NMS ===
    // 按score降序排列
    std::sort(boxes.begin(), boxes.end(),
        [](const Box &a, const Box &b) { return a.score > b.score; });

    std::vector<uint32_t> keep;
    std::vector<bool> suppressed(N, false);
    for (uint32_t i = 0; i < N; i++) {
        if (suppressed[i]) continue;
        keep.push_back(boxes[i].idx);
        for (uint32_t j = i + 1; j < N; j++) {
            if (suppressed[j]) continue;
            if (IoU(boxes[i], boxes[j]) > IOU_THRESH)
                suppressed[j] = true;
        }
    }
    // === CPU NMS End ===

    double t1 = GetTimeUs();
    INFO_LOG("CPU time: %.2f us", t1 - t0);
    INFO_LOG("Kept: %zu / %u boxes", keep.size(), N);

    // 验证: 检查保留框之间IoU是否都<=阈值
    bool ok = true;
    std::vector<Box> keptBoxes;
    for (auto idx : keep)
        for (auto &b : boxes) if (b.idx == idx) { keptBoxes.push_back(b); break; }
    for (size_t i = 0; i < keptBoxes.size() && ok; i++)
        for (size_t j = i + 1; j < keptBoxes.size() && ok; j++)
            if (IoU(keptBoxes[i], keptBoxes[j]) > IOU_THRESH + 1e-6f)
                ok = false;
    INFO_LOG("Verification: %s", ok ? "PASS" : "FAIL");

    aclrtFree(devBuf);
    aclrtFreeHost(hostBoxes);
    aclrtDestroyStream(stream);
    aclrtResetDevice(0);
    aclFinalize();
    return ok ? 0 : 1;
}
