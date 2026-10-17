//
// Sample scene loader for macOS launcher.
//

#include "SampleScene.h"

#include <framework/window/NativeWindow.h>
#include <framework/world/Actor.h>
#include <framework/world/TransformComponent.h>
#include <framework/asset/AssetDataBase.h>

#include <render/Renderer.h>
#include <render/RenderWindow.h>
#include <render/RenderPassPipeline.h>
#include <render/RHI.h>
#include <rhi/Queue.h>
#include <rhi/Device.h>
#include <rhi/SwapChain.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <render/adaptor/pipeline/DefaultForwardPipeline.h>
#include <render/adaptor/components/CameraComponent.h>
#include <render/adaptor/components/LightComponent.h>
#include <render/adaptor/components/PrefabComponent.h>

#include <core/logger/Logger.h>

static const char *TAG = "Sample";

namespace sky {

    void Sample::Init(NativeWindow *window, const std::vector<std::string> &prefabPaths,
                      float camX, float camY, float camZ, float camYawDeg, float camPitchDeg)
    {
        nativeWindow = window;

        InitWorld(prefabPaths, camX, camY, camZ, camYawDeg, camPitchDeg);
        InitPipeline();

        controller.BindWindow(window);
    }

    void Sample::InitWorld(const std::vector<std::string> &prefabPaths, float camX, float camY, float camZ, float camYawDeg, float camPitchDeg)
    {
        camTransform.translation = Vector3(camX, camY, camZ);
        camTransform.rotation.FromEulerYZX(Vector3(camPitchDeg, camYawDeg, 0.f));   // FromEulerYZX 输入为角度制

        sceneProxy = new RenderSceneProxy();

        world = World::CreateWorld();
        world->Init();
        world->AddSubSystem(Name(RenderSceneProxy::NAME.data()), sceneProxy);

        // 相机：Sponza 中庭大约 20m 宽，原点附近
        {
            auto actor = world->CreateActor("MainCamera");
            cameraTrans = actor->GetComponent<TransformComponent>();
            if (cameraTrans != nullptr) {
                cameraTrans->SetWorldTransform(camTransform);
            }
            auto *cam = actor->AddComponent<CameraComponent>();
            cam->Perspective(0.1f, 500.f, 60.f);   // CameraComponent::fov 为角度制，内部转弧度
            cam->SetAspect(nativeWindow->GetWidth(), nativeWindow->GetHeight());
        }

        // 主平行光
        {
            auto actor = world->CreateActor("Sun");
            auto *trans = actor->GetComponent<TransformComponent>();
            if (trans != nullptr) {
                Quaternion rot;
                rot.FromEulerYZX(Vector3(-40.f, 23.f, 0.f));   // 角度制：俯仰 -40°，偏航 23°
                trans->SetWorldRotation(rot);
            }
            auto *light = actor->AddComponent<MainDirectLightComponent>();
            light->SetIntensity(3.f);
        }

        // 导入的 prefab（支持多个）
        for (const auto &path : prefabPaths) {
            auto src = AssetDataBase::Get()->FindAsset(path);
            if (!src) {
                LOG_E(TAG, "prefab asset not found: %s", path.c_str());
                continue;
            }
            auto actor = world->CreateActor("Prefab");
            auto *prefab = actor->AddComponent<PrefabComponent>();
            prefab->SetPrefabId(src->uuid);
        }
    }

    void Sample::InitPipeline()
    {
        renderWindow = Renderer::Get()->CreateRenderWindow(nativeWindow->GetNativeHandle(),
            nativeWindow->GetWidth(), nativeWindow->GetHeight(), false);

        auto *ppl = new RenderPassPipeline();
        Renderer::Get()->SetPipeline(ppl);

        auto *scenePipeline = new DefaultForwardPipeline(sceneProxy->GetRenderScene());
        scenePipeline->SetOutput(renderWindow);
        ppl->AddScenePass(scenePipeline);
    }

    void Sample::Shutdown()
    {
        Renderer::Get()->SetPipeline(nullptr);
        sceneProxy = nullptr;
        Renderer::Get()->DestroyRenderWindow(renderWindow);
        world = nullptr;
    }

    void Sample::Tick(float time)
    {
        // 编辑器同款控制器：右键拖拽旋转 / WASD 移动 / 滚轮调速
        if (cameraTrans != nullptr) {
            auto [res, dirty] = controller.Resolve(time, camTransform);
            if (dirty) {
                camTransform = res;
                cameraTrans->SetWorldTransform(camTransform);
            }
        }

        if (world) {
            world->Tick(time);
        }

        // 调试：启动 60 秒（纹理流式加载完成）后抓一组交换链图像
        ++tickCount;
        if (tickCount == 1) {
            startTime = std::chrono::steady_clock::now();
        }
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (elapsed >= std::chrono::seconds(60) && !captured) {
            captured = true;
            CaptureScreenshot("/tmp/sky_frame.png");
        }
    }

    void Sample::CaptureScreenshot(const std::string &path)
    {
        auto &sc = renderWindow->GetSwapChain();
        if (!sc) {
            return;
        }
        const auto &ext = sc->GetExtent();
        const uint32_t w = ext.width;
        const uint32_t h = ext.height;
        auto fmt = sc->GetFormat();
        bool bgra = (fmt == rhi::PixelFormat::BGRA8_UNORM || fmt == rhi::PixelFormat::BGRA8_SRGB);

        auto *queue = RHI::Get()->GetDevice()->GetQueue(rhi::QueueType::GRAPHICS);
        const uint32_t imageCount = sc->GetImageCount();
        for (uint32_t img = 0; img < imageCount; ++img) {
            std::string imgPath = path;
            auto dot = imgPath.find_last_of('.');
            imgPath = imgPath.substr(0, dot) + "_" + std::to_string(img) + imgPath.substr(dot);

            auto image = sc->GetImage(img);
            auto handle = queue->ReadImage(image, [imgPath, w, h, bgra](const uint8_t *data, uint32_t layer, uint64_t size) {
                if (layer != 0 || data == nullptr) {
                    return;
                }
                std::vector<uint8_t> rgba(static_cast<size_t>(w) * h * 4);
                for (size_t i = 0; i < static_cast<size_t>(w) * h; ++i) {
                    rgba[i * 4 + 0] = bgra ? data[i * 4 + 2] : data[i * 4 + 0];
                    rgba[i * 4 + 1] = data[i * 4 + 1];
                    rgba[i * 4 + 2] = bgra ? data[i * 4 + 0] : data[i * 4 + 2];
                    rgba[i * 4 + 3] = 255;
                }
                if (stbi_write_png(imgPath.c_str(), static_cast<int>(w), static_cast<int>(h), 4, rgba.data(), static_cast<int>(w * 4))) {
                    LOG_I(TAG, "screenshot saved: %s (%ux%u)", imgPath.c_str(), w, h);
                } else {
                    LOG_E(TAG, "screenshot write failed: %s", imgPath.c_str());
                }
            });
            queue->Wait(handle);
        }
    }

} // namespace sky
