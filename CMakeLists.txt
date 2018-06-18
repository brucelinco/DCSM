cmake_minimum_required(VERSION 3.7)

project(PFAC-Duo)
find_package(CUDA REQUIRED)
find_package(OpenCL REQUIRED)
find_package(PythonInterp REQUIRED)
include(cmake/oclminify.cmake)

add_definitions("-std=gnu++11")

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; -arch=sm_52 -Xptxas -dlcm=ca --ptxas-options=--allow-expensive-optimizations=true)
cuda_add_executable(dpfac main.cpp dpfac.cpp dpfac.h dpfac-internal.cpp dpfac-internal.h
                          Dispatcher.cpp Dispatcher.h
                          dpfac_kernel.cl dpfac_kernel.cu dpfac_kernel.cuh
                          DualByteTree.cpp DualByteTree.h
                          HashTable.cpp HashTable.h
                          MetaTable.cpp MetaTable.h)
oclminify_minify_sources(TARGET dpfac
                         SOURCES "dpfac_kernel.cl"
                         PYTHON_COMMAND ${PYTHON_EXECUTABLE}
                         OPTIONS "--header" "--try-build" "--no-minify")
target_link_libraries (dpfac OpenCL::OpenCL)

#add_custom_target(run cd '${PROJECT_SOURCE_DIR}' && '${PROJECT_BINARY_DIR}/${PROJECT_NAME}'
#                  DEPENDS cnnConvLayer)
#add_custom_target(run ./${PROJECT_NAME}
#                  DEPENDS cnnConvLayer))
