
if(CMAKE_CUDA_COMPILER)
    add_library(cuda_sph OBJECT cudaDensity.cu cudaIAD.cu cudaMomentumAndEnergyIAD.cu)
    target_include_directories(cuda_sph PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_include_directories(cuda_sph PRIVATE ${PROJECT_SOURCE_DIR}/domain/include)

    target_compile_definitions(cuda_sph PRIVATE USE_CUDA)
    target_compile_options(cuda_sph PRIVATE --expt-relaxed-constexpr)
    set_target_properties(cuda_sph PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
endif()
