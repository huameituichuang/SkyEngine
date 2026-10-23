//
// Created by blues on 2024/1/1.
//

#include <framework/application/ToolApplicationBase.h>

#include <cxxopts.hpp>

#include <core/logger/Logger.h>
#include <core/file/FileIO.h>

#include <framework/asset/AssetManager.h>
#include <framework/asset/AssetDataBase.h>
#include <framework/asset/AssetBuilderManager.h>
static const char *TAG = "Application";
static const char *CONFIG_PATH = "/config/modules_tool.json";

namespace sky {
#ifdef SKY_EDITOR
    void ToolApplicationBase::ParseStartArgs()
    {
        cxxopts::Options options("Application Launcher", "SkyEngine Launcher");
        options.allow_unrecognised_options();

        options.add_options()
            ("p,project", "Project Directory", cxxopts::value<std::string>())
            ("e,engine", "Engine Directory", cxxopts::value<std::string>())
            ("i,intermediate", "Project Intermediate Directory", cxxopts::value<std::string>())
            ("m,module", "Addition Module", cxxopts::value<std::vector<std::string>>())
            ("h,help", "Print usage");

        auto result = options.parse(static_cast<int32_t>(arguments.args.size()), arguments.args.data());

        if (result.count("help") != 0u) {
            printf("%s", options.help().c_str());
            return;
        }
        if (result.count("project") != 0u) {
            projectPath = result["project"].as<std::string>();
        }
        if (result.count("module") != 0u) {
            auto modules = result["module"].as<std::vector<std::string>>();
            for (auto &module : modules) {
                moduleManager->RegisterModule({module, {}});
            }
        }

        // 对齐 EditorApplication 的 FS 初始化：builder 模块依赖 AssetBuilderManager/AssetDataBase 的完整工作区
        std::string enginePath = projectPath;
        if (result.count("engine") != 0u) {
            enginePath = result["engine"].as<std::string>();
        }
        FilePath intermediatePath = FilePath(projectPath) / FilePath("Intermediate");
        if (result.count("intermediate") != 0u) {
            intermediatePath = FilePath(result["intermediate"].as<std::string>());
        }

        FileSystemPtr       workFs   = new NativeFileSystem(projectPath);
        NativeFileSystemPtr engineFs = new NativeFileSystem(enginePath);
        NativeFileSystemPtr interFs  = new NativeFileSystem(intermediatePath);

        AssetManager::Get()->SetWorkFileSystem(workFs);
        AssetDataBase::Get()->SetEngineFs(engineFs);
        AssetDataBase::Get()->SetWorkSpaceFs(workFs);

        AssetBuilderManager::Get()->SetEngineFs(engineFs);
        AssetBuilderManager::Get()->SetWorkSpaceFs(workFs);
        AssetBuilderManager::Get()->SetInterMediateFs(interFs);
    }

    void ToolApplicationBase::LoadConfigs()
    {
        std::string json;
        if (!ReadString(projectPath + CONFIG_PATH, json)) {
            LOG_W(TAG, "Load Config Failed");
            return;
        }

        rapidjson::Document document;
        document.Parse(json.c_str());

        if (document.HasMember("modules")) {
            auto array = document["modules"].GetArray();
            for (auto &module : array) {
                if (!module.HasMember("name")) {
                    continue;
                }

                ModuleInfo info;
                info.name = module["name"].GetString();

                if (module.HasMember("dependencies")) {
                    auto depArray = module["dependencies"].GetArray();
                    for (auto &dep : depArray) {
                        info.dependencies.emplace_back(dep.GetString());
                    }
                }
                moduleManager->RegisterModule(info);
            }
        }
    }

    void ToolApplicationBase::PostInit()
    {
    }
#endif
} // namespace sky