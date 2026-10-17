set(CURRENT_FOLDER Test)

configure_file(EngineRoot.h.in ${CMAKE_BINARY_DIR}/Gen/test/EngineRoot.h)
include_directories(${CMAKE_BINARY_DIR}/Gen)

add_subdirectory(core)
add_subdirectory(framework)
add_subdirectory(vulkan)
# render 测试引用了已重构掉的 API（StaticMesh 等），暂时不编
# add_subdirectory(render)