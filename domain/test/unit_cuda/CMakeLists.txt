
if(CMAKE_CUDA_COMPILER)

    add_executable(component_units_cuda btree.cu octree.cu octree_internal.cu $<TARGET_OBJECTS:gather_obj> gather.cpp test_main.cpp)
    target_include_directories(component_units_cuda PRIVATE ../../include)
    target_include_directories(component_units_cuda PRIVATE ../)
    target_link_libraries(component_units_cuda PUBLIC OpenMP::OpenMP_CXX gtest_main)
endif()
