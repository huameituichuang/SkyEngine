set(CURRENT_FOLDER Engine/Render/RHI)

add_subdirectory(rhi)
add_subdirectory(vulkan)

if (WIN32)
    add_subdirectory(dx12)
endif()

option(SKY_BUILD_METAL "build metal rhi backend" ON)

if (APPLE AND SKY_BUILD_METAL)
    add_subdirectory(metal)
endif()

if (SKY_BUILD_GLES OR ANDROID)
    #add_subdirectory(gles)
endif()
