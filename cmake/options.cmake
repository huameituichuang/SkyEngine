SET(3RD_PATH "" CACHE STRING "SkyEngine 3rd path")

# Apple 平台下模块系统依赖跨进程边界的单例共享（ModuleManager dlopen 模块后，模块与宿主必须看到同一份
# Core/Framework 单例），静态库会让每个 dylib/exe 各持一份副本导致状态分裂。Apple 上使用 SHARED。
if (APPLE)
    set(SKY_BASE_LIB_TYPE SHARED)
else ()
    set(SKY_BASE_LIB_TYPE STATIC)
endif ()

option(SKY_BUILD_EDITOR "build editor" OFF)
option(SKY_EDITOR "editor mode" OFF)

if (SKY_BUILD_EDITOR)
    set(SKY_EDITOR ON)
endif ()

option(SKY_BUILD_GLES "build gles" OFF)
option(SKY_BUILD_TEST "build test" OFF)
option(SKY_USE_TRACY "use tracy profiler" OFF)

# todo: controlled by project config json.
option(SKY_BUILD_XR "xr plugin" OFF)
option(SKY_BUILD_PYTHON "python plugin" OFF)

option(SKY_MATH_SIMD "enable simd math" OFF)