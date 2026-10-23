//
// Created by Zach Lee on 2021/12/1.
//

#include <render/adaptor/components/CameraComponent.h>
#include <framework/serialization/SerializationContext.h>
#include <framework/serialization/JsonArchive.h>
#include <framework/world/TransformComponent.h>
#include <core/math/MathUtil.h>

#include <render/adaptor/Util.h>
#include <render/Renderer.h>

namespace sky {
    CameraComponent::~CameraComponent()
    {
        ShutDown();
    }

    void CameraComponent::Reflect(SerializationContext *context)
    {
        context->Register<ProjectType>("ProjectType")
            .Enum(ProjectType::PROJECTIVE, "Projective")
            .Enum(ProjectType::ORTHOGONAL, "Orthogonal");

        context->Register<CameraComponent>("CameraComponent")
            .Member<&CameraComponent::near>("near")
            .Member<&CameraComponent::far>("far")
            .Member<&CameraComponent::fov>("fov");
//            .Member<&CameraComponent::type>("type");
//            .Property(UI_PROP_VISIBLE, false);
    }

    void CameraComponent::Perspective(float near_, float far_, float fov_)
    {
        near   = near_;
        far    = far_;
        fov    = fov_;
        type   = ProjectType::PROJECTIVE;
        UpdateProjection();
    }

    void CameraComponent::Otho(float h)
    {
        othoH = h;
        type  = ProjectType::ORTHOGONAL;
        UpdateProjection();
    }

    void CameraComponent::SetAspect(uint32_t width_, uint32_t height_)
    {
        width = width_;
        height = height_;
        aspect = static_cast<float>(width) / static_cast<float>(height);
        UpdateProjection();
    }

    const Matrix4 &CameraComponent::GetProject() const
    {
        return sceneView != nullptr ? sceneView->GetProject() : Matrix4::Identity();
    }

    const Matrix4 &CameraComponent::GetView() const
    {
        return sceneView != nullptr ? sceneView->GetView() : Matrix4::Identity();
    }

    void CameraComponent::OnAttachToWorld()
    {
        transformEvent.Bind(this, actor);

        auto *scene = GetRenderSceneFromActor(actor);
        sceneView = scene->CreateSceneView(1);

        static const Name MainCameraName("MainCamera");

        scene->AttachSceneView(sceneView, MainCameraName);
        scene->SetMainView(Name(MainCameraName));

        if (auto *trans = actor->GetComponent<TransformComponent>(); trans != nullptr) {
            OnTransformChanged(trans->GetLocalTransform(), trans->GetWorldTransform());
        }
    }

    void CameraComponent::OnDetachFromWorld()
    {
        ShutDown();
    }

    void CameraComponent::ShutDown()
    {
        if (sceneView != nullptr) {
            auto *scene = GetRenderSceneFromActor(actor);
            scene->DetachSceneView(sceneView, Name("MainCamera"));
            scene->RemoveSceneView(sceneView);
            sceneView = nullptr;
        }
    }

    void CameraComponent::SaveJson(JsonOutputArchive &ar) const
    {
        ar.StartObject();
        ar.SaveValueObject(std::string("near"), near);
        ar.SaveValueObject(std::string("far"), far);
        ar.SaveValueObject(std::string("fov"), fov);
        ar.SaveValueObject(std::string("aspect"), aspect);
        ar.SaveValueObject(std::string("othoH"), othoH);
        ar.SaveValueObject(std::string("type"), type);
        ar.EndObject();
    }

    void CameraComponent::LoadJson(JsonInputArchive &ar)
    {
        ar.LoadKeyValue("near", near);
        ar.LoadKeyValue("far", far);
        ar.LoadKeyValue("fov", fov);
        ar.LoadKeyValue("aspect", aspect);
        ar.LoadKeyValue("othoH", othoH);
        ar.LoadKeyValue("type", type);
    }

    void CameraComponent::OnTransformChanged(const Transform& global, const Transform& local)
    {
        auto world = global.ToMatrix();
        sceneView->SetMatrix(world);
        UpdateProjection();
    }

    void CameraComponent::UpdateProjection()
    {
        if (sceneView == nullptr) {
            return;
        }
        if (type == ProjectType::PROJECTIVE) {
            // fov 存储为角度制，MakePerspective 需要弧度
            sceneView->SetPerspective(near, far, fov / 180.f * 3.1415926f, aspect);
        } else {
            const float x = othoH;
            const float y = othoH;
            sceneView->SetOrthogonal(-x, x, y, -y, near, far, 0);
        }
    }
} // namespace sky
