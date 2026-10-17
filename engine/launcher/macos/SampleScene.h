//
// Sample scene loader for macOS launcher.
// Mirrors engine/launcher/android/app/src/main/cpp/SampleScene.cpp,
// but builds the world programmatically instead of loading a .world json.
//

#pragma once

#include <framework/world/World.h>
#include <framework/controller/SimpleController.h>
#include <chrono>
#include <string>
#include <vector>
#include <render/adaptor/RenderSceneProxy.h>

namespace sky {

    class NativeWindow;
    class RenderWindow;
    class RenderPassPipeline;
    class TransformComponent;

    class Sample {
    public:
        Sample() = default;
        ~Sample() = default;

        void Init(NativeWindow *window, const std::vector<std::string> &prefabPaths,
                  float camX = 0.f, float camY = 3.f, float camZ = 12.f, float camYawDeg = 0.f, float camPitchDeg = 0.f);
        void Shutdown();
        void Tick(float time);

    private:
        void InitWorld(const std::vector<std::string> &prefabPaths, float camX, float camY, float camZ, float camYawDeg, float camPitchDeg);
        void InitPipeline();
        void CaptureScreenshot(const std::string &path);

        NativeWindow      *nativeWindow = nullptr;
        WorldPtr           world;
        RenderSceneProxy  *sceneProxy   = nullptr;
        RenderWindow      *renderWindow = nullptr;
        uint32_t           tickCount    = 0;
        bool               captured     = false;
        std::chrono::steady_clock::time_point startTime;

        // 编辑器同款第一人称控制器（右键旋转 / WASD / 滚轮调速）
        FirstPersonController controller;
        TransformComponent   *cameraTrans = nullptr;
        Transform             camTransform;
    };

} // namespace sky
