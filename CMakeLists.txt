# CMake build for the ProGraML llvm2graph component (LLVM-21 port).
#
# Replaces the Bazel build for OUR needs: the LLVM-IR -> program-graph path only
# (no clang2graph, no XLA, no python-bundled toolchain). Built up progressively,
# one testable layer at a time. See LLVM21_MIGRATION_PLAN.md.
#
# Layers (add as each is verified):
#   [x] proto        - generate + build the ProgramGraph protobufs   (this file)
#   [x] graph base   - programl/graph/*.cc  (labm8 compat shim + absl)
#   [ ] llvm builder - programl/ir/llvm/**  ported to LLVM 21
#   [ ] llvm2graph   - the binary: .ll/.bc -> ProgramGraph proto
cmake_minimum_required(VERSION 3.20)
project(programl_llvm21 LANGUAGES CXX)

# LLVM 21 requires C++17 (the old Bazel build used C++14 — intentional bump).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(PROGRAML_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# ---------------------------------------------------------------------------
# Layer: protobuf  (proven to work under modern protobuf 5.x)
# ---------------------------------------------------------------------------
# Prefer the CONFIG package modern protobuf ships (pulls its abseil deps);
# fall back to the legacy FindProtobuf module.
find_package(protobuf CONFIG QUIET)
if(protobuf_FOUND)
  set(PROTOBUF_LIB protobuf::libprotobuf)
  set(PROTOC protobuf::protoc)
else()
  find_package(Protobuf REQUIRED)
  set(PROTOBUF_LIB protobuf::libprotobuf)
  set(PROTOC ${Protobuf_PROTOC_EXECUTABLE})
endif()

set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

# proto import paths are repo-root-relative (e.g. "programl/proto/..."),
# so protoc's -I is the repo root and outputs mirror that layout under gen/.
set(PROTO_FILES
  programl/proto/program_graph.proto
  programl/proto/util.proto
  programl/third_party/tensorflow/features.proto)

# Generate ALL protos in ONE protoc call so every .pb.h exists before any .pb.cc
# is compiled (util.pb.cc #includes program_graph.pb.h — separate per-file custom
# commands would race under a parallel build).
set(PROTO_SRCS "")
set(PROTO_OUTS "")
set(PROTO_ABS "")
foreach(pf ${PROTO_FILES})
  get_filename_component(pf_dir ${pf} DIRECTORY)
  get_filename_component(pf_name ${pf} NAME_WE)
  list(APPEND PROTO_SRCS ${PROTO_GEN_DIR}/${pf_dir}/${pf_name}.pb.cc)
  list(APPEND PROTO_OUTS ${PROTO_GEN_DIR}/${pf_dir}/${pf_name}.pb.cc
                         ${PROTO_GEN_DIR}/${pf_dir}/${pf_name}.pb.h)
  list(APPEND PROTO_ABS ${PROGRAML_ROOT}/${pf})
endforeach()
add_custom_command(
  OUTPUT ${PROTO_OUTS}
  COMMAND ${PROTOC} --proto_path=${PROGRAML_ROOT} --cpp_out=${PROTO_GEN_DIR} ${PROTO_ABS}
  DEPENDS ${PROTO_ABS}
  COMMENT "protoc (all ProgramGraph protos)"
  VERBATIM)

add_library(programl_proto STATIC ${PROTO_SRCS})
target_include_directories(programl_proto PUBLIC ${PROTO_GEN_DIR})
target_link_libraries(programl_proto PUBLIC ${PROTOBUF_LIB})

# Convenience: build just the proto layer with `cmake --build . --target programl_proto`.

# ---------------------------------------------------------------------------
# Layer: graph base  (programl/graph/*.cc — no LLVM, so locally buildable)
# ---------------------------------------------------------------------------
# abseil (flat_hash_map/set used directly by the builder). Modern protobuf pulls
# abseil transitively, but we request it explicitly for the container targets.
find_package(absl CONFIG REQUIRED)

# Our self-contained labm8 stand-in: adding this dir makes "labm8/cpp/*.h" resolve.
set(LABM8_COMPAT_INCLUDE ${PROGRAML_ROOT}/third_party/labm8_compat)

add_library(programl_graph STATIC
  programl/graph/program_graph_builder.cc
  programl/graph/features.cc)
target_include_directories(programl_graph PUBLIC
  ${PROGRAML_ROOT}          # for "programl/graph/*.h" source headers
  ${LABM8_COMPAT_INCLUDE}   # for "labm8/cpp/*.h"
  ${PROTO_GEN_DIR})         # for generated "programl/proto/*.pb.h"
target_link_libraries(programl_graph PUBLIC
  programl_proto
  absl::flat_hash_map
  absl::flat_hash_set)
