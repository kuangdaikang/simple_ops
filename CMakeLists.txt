cmake_minimum_required(VERSION 3.16)

set(CMAKE_ASC_ARCHITECTURES "dav-2201" CACHE STRING "NPU architecture: dav-2201, dav-3510")

find_package(ASC REQUIRED)

project(simple_ascend_ops LANGUAGES ASC CXX)

# ==================================================================
# 三类 AI Core 算子 (共10个)
# ==================================================================

# Vector-only (4个):   __global__ __vector__
set(VECTOR_OPS
    01_vector_add/op_add
    02_vector_relu/op_relu
    03_vector_sigmoid/op_sigmoid
    04_vector_abs/op_abs
)

# Cube-only (3个):     __global__ __cube__
set(CUBE_OPS
    05_cube_matmul/op_matmul
    06_cube_matmul_bias/op_matmul_bias
    07_cube_matmul_large/op_matmul_large
)

# Fusion Vector+Cube (3个): __global__ __cube_vec__
set(FUSION_OPS
    08_fusion_matmul_relu/op_matmul_relu
    09_fusion_matmul_bias_relu/op_matmul_bias_relu
    10_fusion_matmul_add/op_matmul_add
)

set(AICORE_OPS ${VECTOR_OPS} ${CUBE_OPS} ${FUSION_OPS})

foreach(op_path ${AICORE_OPS})
    get_filename_component(op_name ${op_path} NAME)
    add_executable(${op_name} ${op_path}.asc)
    target_compile_options(${op_name} PRIVATE
        $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=${CMAKE_ASC_ARCHITECTURES}>
    )
    target_link_options(${op_name} PRIVATE -lm)
endforeach()

# ==================================================================
# CPU 算子 (3个):  C++ only, 在Host CPU上执行
# ==================================================================
set(CPU_OPS
    11_cpu_topk/op_topk
    12_cpu_sort/op_sort
    13_cpu_nms/op_nms
)

foreach(op_path ${CPU_OPS})
    get_filename_component(op_name ${op_path} NAME)
    add_executable(${op_name} ${op_path}.cpp)
    target_include_directories(${op_name} PRIVATE
        ${ASCEND_CANN_PACKAGE_PATH}/include
        ${ASCEND_CANN_PACKAGE_PATH}/include/acl
    )
    target_link_libraries(${op_name} PRIVATE ascendcl acl_op_compiler)
endforeach()

message(STATUS "CMAKE_ASC_ARCHITECTURES = ${CMAKE_ASC_ARCHITECTURES}")
message(STATUS "AI Core operators: ${AICORE_OPS}")
message(STATUS "CPU operators:      ${CPU_OPS}")
