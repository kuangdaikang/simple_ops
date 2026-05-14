#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
gen_data.py - 为13个昇腾算子生成测试数据

生成文件:
  input_a.bin              - 通用浮点输入 (Vector/CPU算子用)
  input_b.bin              - 通用浮点输入 B
  input_a_positive.bin     - 正数输入 (Sqrt用, 已弃用保留兼容)
  input_matmul_a.bin       - 矩阵乘 A 输入 (M*K=128*128 floats)
  input_matmul_b.bin       - 矩阵乘 B 输入 (K*N=128*128 floats)
"""

import numpy as np
import os
import sys

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "data")

def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    np.random.seed(42)

    # === Vector/CPU 通用数据 ===
    VLEN = 4096 * 9 * 8  # 294912

    a = np.random.uniform(-5.0, 5.0, VLEN).astype(np.float32)
    a.tofile(os.path.join(OUT_DIR, "input_a.bin"))
    print(f"[OK] input_a.bin  ({VLEN} floats)")

    b = np.random.uniform(-3.0, 3.0, VLEN).astype(np.float32)
    b.tofile(os.path.join(OUT_DIR, "input_b.bin"))
    print(f"[OK] input_b.bin  ({VLEN} floats)")

    # === Matmul 数据 (M=128, K=128, N=128) ===
    M, K, N = 128, 128, 128

    matA = np.random.uniform(-2.0, 2.0, M * K).astype(np.float32)
    matA.tofile(os.path.join(OUT_DIR, "input_matmul_a.bin"))
    print(f"[OK] input_matmul_a.bin  ({M}x{K} floats)")

    matB = np.random.uniform(-2.0, 2.0, K * N).astype(np.float32)
    matB.tofile(os.path.join(OUT_DIR, "input_matmul_b.bin"))
    print(f"[OK] input_matmul_b.bin  ({K}x{N} floats)")

    print(f"\nAll data files generated in: {OUT_DIR}")

if __name__ == "__main__":
    main()
