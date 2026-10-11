cmake_minimum_required(VERSION 3.20)
project(RenombrarArchivosNative LANGUAGES CXX)

add_library(RenombrarArchivos.Core SHARED native/RenombrarArchivosCore.cpp)
target_compile_features(RenombrarArchivos.Core PRIVATE cxx_std_17)
target_compile_definitions(RenombrarArchivos.Core PRIVATE RENOMBRAR_CORE_EXPORTS)
set_target_properties(RenombrarArchivos.Core PROPERTIES
    OUTPUT_NAME "RenombrarArchivos.Core"
    CXX_VISIBILITY_PRESET hidden
)
