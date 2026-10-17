//
// Created by Zach Lee on 2021/11/9.
//

#include <framework/platform/PlatformBase.h>
#include <framework/application/GameApplication.h>

#include "SampleScene.h"

using namespace sky;

int main(int argc, char **argv)
{
    // platform
    sky::Platform* platform = sky::Platform::Get();
    if (!platform->Init({})) {
        return -1;
    }

    // --prefab <path> 指定要渲染的 prefab 资产路径，可重复多次加载多个 prefab
    // --cam <x> <y> <z> <yawDeg> [pitchDeg] 指定相机位置和朝向（yaw 0 朝 -Z，pitch 负值俯视）
    std::vector<std::string> prefabPaths = {
        "Prefabs/NewSponza_Curtains_USD_YUp.usda/NewSponza_Curtains_USD_YUp.prefab"
    };
    float camX = 0.f, camY = 3.f, camZ = 12.f, camYaw = 0.f, camPitch = 0.f;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--prefab") {
            if (prefabPaths.size() == 1 && prefabPaths[0].find("NewSponza_Curtains_USD") != std::string::npos) {
                prefabPaths.clear(); // 首次显式指定时去掉默认项
            }
            prefabPaths.emplace_back(argv[i + 1]);
        }
    }
    for (int i = 1; i + 4 < argc; ++i) {
        if (std::string(argv[i]) == "--cam") {
            camX   = std::stof(argv[i + 1]);
            camY   = std::stof(argv[i + 2]);
            camZ   = std::stof(argv[i + 3]);
            camYaw = std::stof(argv[i + 4]);
            if (i + 5 < argc && (isdigit(argv[i + 5][0]) != 0 ||
                (argv[i + 5][0] == '-' && isdigit(argv[i + 5][1]) != 0))) {
                camPitch = std::stof(argv[i + 5]);
            }
        }
    }

    sky::GameApplication app;
    if (app.Init(argc, argv)) {
        sky::Sample sample;
        sample.Init(app.GetWindow(), prefabPaths, camX, camY, camZ, camYaw, camPitch);
        app.BindTick([&sample](float dt) { sample.Tick(dt); });

        app.Mainloop();
        sample.Shutdown();
    }

    return 0;
}
