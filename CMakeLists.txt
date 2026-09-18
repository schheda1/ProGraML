# CMake build for the ProGraML llvm2graph component (LLVM-21 port).
#
# Replaces the Bazel build for OUR needs: the LLVM-IR -> program-graph path only
# (no clang2graph, no XLA, no python-bundled toolchain). Built up progressively,
# one testable layer at a time. See LLVM21_MIGRATION_PLAN.md.
#
# Layers:
#   [x] proto        - generate + build the ProgramGraph protobufs
#   [x] graph base   - programl/graph/*.cc  (labm8 compat shim + absl)
#   [x] llvm builder - programl/ir/llvm/**  ported to LLVM 21   (opt-in)
#   [ ] llvm2graph   - the binary: .ll/.bc -> ProgramGraph proto
#
# Build Layers 1-2 only (no LLVM):
#   cmake -S . -B build && cmake --build build
# Build the LLVM builder too (needs your LLVM 21):
#   cmake -S . -B build -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm
#   cmake --build build
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
set(LABM8_COMPAT_INCLUDE ${PROGRAML_ROOT}/third_party/labm8_compat)

# ---------------------------------------------------------------------------
# Optional LLVM (opt-in via -DLLVM_DIR). Detected FIRST so its RTTI setting can
# be applied globally — protobuf generates different code under RTTI vs no-RTTI,
# so every target must agree, or objects mismatch at link time.
# ---------------------------------------------------------------------------
set(PROGRAML_HAVE_LLVM OFF)
if(DEFINED LLVM_DIR OR PROGRAML_ENABLE_LLVM)
  find_package(LLVM REQUIRED CONFIG)
  message(STATUS "ProGraML: LLVM ${LLVM_PACKAGE_VERSION} "
                 "(major=${LLVM_VERSION_MAJOR}, RTTI=${LLVM_ENABLE_RTTI})")
  set(PROGRAML_HAVE_LLVM ON)
  # Match LLVM's RTTI setting across ALL targets (default LLVM builds are no-RTTI;
  # protobuf headers then need GOOGLE_PROTOBUF_NO_RTTI to avoid dynamic_cast).
  if(NOT LLVM_ENABLE_RTTI)
    add_compile_options(-fno-rtti)
    add_compile_definitions(GOOGLE_PROTOBUF_NO_RTTI)
  endif()
endif()

# ---------------------------------------------------------------------------
# Layer: protobuf
# ---------------------------------------------------------------------------
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

# import paths are repo-root-relative (e.g. "programl/proto/..."), so -I is the
# repo root and outputs mirror that layout under gen/.
set(PROTO_FILES
  programl/proto/program_graph.proto
  programl/proto/util.proto
  programl/third_party/tensorflow/features.proto)

# Generate ALL protos in ONE protoc call so every .pb.h exists before any .pb.cc
# is compiled (util.pb.cc #includes program_graph.pb.h — separate per-file
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

# ---------------------------------------------------------------------------
# Layer: graph base  (programl/graph/*.cc — no LLVM)
# ---------------------------------------------------------------------------
find_package(absl CONFIG REQUIRED)

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

# ---------------------------------------------------------------------------
# Layer: LLVM builder  (programl/ir/llvm/** — ported to LLVM 21; opt-in)
# ---------------------------------------------------------------------------
if(PROGRAML_HAVE_LLVM)
  add_library(programl_ir_llvm STATIC
    programl/ir/llvm/llvm.cc
    programl/ir/llvm/internal/program_graph_builder.cc
    programl/ir/llvm/internal/text_encoder.cc
    programl/ir/llvm/internal/program_graph_builder_pass.cc)
  # PROGRAML_LLVM_VERSION_MAJOR drives the #if version forks in the sources.
  target_compile_definitions(programl_ir_llvm PRIVATE
    PROGRAML_LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR})
  separate_arguments(LLVM_DEFS NATIVE_COMMAND "${LLVM_DEFINITIONS}")
  target_compile_options(programl_ir_llvm PRIVATE ${LLVM_DEFS})
  target_include_directories(programl_ir_llvm PUBLIC
    ${PROGRAML_ROOT}
    ${LABM8_COMPAT_INCLUDE}
    ${PROTO_GEN_DIR}
    ${LLVM_INCLUDE_DIRS})
  llvm_map_components_to_libnames(LLVM_LIBS core irreader support)
  target_link_libraries(programl_ir_llvm PUBLIC
    programl_graph
    programl_proto
    ${LLVM_LIBS})
endif()
