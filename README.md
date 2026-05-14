# Ascend Simple Operators (昇腾简单算子集)

基于华为 **Ascend C** (asc-devkit) 开发的 **13 个算子**，覆盖 **四类核心**：

| 类别 | 数量 | 核心 | 内核声明 |
|------|------|------|----------|
| **Vector-Only** | 4 | 纯向量单元 MTE2/V/MTE3 | `__global__ __vector__` |
| **Cube-Only** | 3 | 纯矩阵单元 Cube+Mmad | `__global__ __cube__` |
| **Fusion (Cube+Vector)** | 3 | Cube矩阵乘 + Vector后处理 | `__global__ __cube_vec__` |
| **CPU** | 3 | Host CPU (不规则计算) | C++ host |

## 13 个算子一览

| # | 算子 | 类别 | 运算 | 核心API |
|---|------|------|------|---------|
| 1 | VectorAdd | Vector | C=A+B | `AscendC::Add` |
| 2 | ReLU | Vector | B=max(0,A) | `AscendC::Relu` |
| 3 | Sigmoid | Vector | B=1/(1+e^-A) | `AscendC::Sigmoid` |
| 4 | Abs | Vector | B=\|A\| | `AscendC::Abs` |
| 5 | MatMul | Cube | C=A×B (64³) | `AscendC::Mmad` |
| 6 | MatMulTransB | Cube | C=A×B^T (64³) | `AscendC::Mmad` |
| 7 | MatMulTiled | Cube | C=A×B (128³, K循环) | `AscendC::Mmad` |
| 8 | MatMul+ReLU | Fusion | matmul→relu | Mmad+Relu |
| 9 | MatMul+Bias+ReLU | Fusion | matmul+bias→relu | Mmad+Adds+Relu |
| 10 | MatMul+Add | Fusion | matmul+residual | Mmad+Add |
| 11 | TopK | CPU | 选取最大K个值 | `std::partial_sort` |
| 12 | Sort | CPU | 浮点升序排序 | `std::sort` |
| 13 | NMS | CPU | IoU非极大抑制 | 自实现IoU |

## 核心流水架构

```
Vector-Only:   MTE2(GM→UB) → VECTOR(计算) → MTE3(UB→GM)
Cube-Only:     MTE2→MTE1→Mmad→MTE3 (Nd2Nz,LoadData,Fixpipe)
Fusion:        MTE2→MTE1→Mmad→VECTOR→MTE3
CPU:           ACL memcpy → Host std::sort/partial_sort/IoU
```

## 快速开始

```bash
source /usr/local/Ascend/latest/set_env.sh
cd simple_ops

# 1. 生成数据 (~1MB vector, ~64KB matmul)
python3 scripts/gen_data.py

# 2. 编译 (根据卡选架构)
bash build.sh                         # dav-2201 (910B)
# ASC_ARCH=dav-3510 bash build.sh      # dav-3510 (910C/950)

# 3. 运行 + 计时
bash run_all.sh

# 4. msprof trace (仅 AI Core 算子)
bash run_all.sh profile

# 5. 多卡集群: 指定卡运行 (ASCEND_RT_VISIBLE_DEVICES 隔离)
bash run_all.sh --device 0            # 只用卡0 (默认)
bash run_all.sh --device 1            # 只用卡1
bash run_all.sh profile --device 1    # 卡1 + profiling
```

## 多卡集群说明

所有算子的 `main()` 中硬编码 `aclrtSetDevice(0)` 固定使用设备0。
`run_all.sh` 在启动时设置 `ASCEND_RT_VISIBLE_DEVICES` 隔离其他卡。

```bash
# 查看可用卡
npu-smi info

# 指定卡运行 (内部设置 ASCEND_RT_VISIBLE_DEVICES=$DEVICE_ID)
bash run_all.sh --device 1

# 手动隔离 (等效)
ASCEND_RT_VISIBLE_DEVICES=1 bash run_all.sh
```

每个算子独立运行，不会占用多张卡。
