// Copyright (c) 2008-2022 the Urho3D project
// License: MIT

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Zone.h"
#include "../GraphicsAPI/Texture2D.h"
#include "../Scene/Scene.h"
#include "../UI/UI.h"
#include "../UI/UIEvents.h"
#include "../UI/View3D.h"

namespace Urho3D
{

extern const char* UI_CATEGORY;

View3D::View3D(Context* context) :
    Window(context),
    ownScene_(true),
    rttFormat_(Graphics::GetRGBFormat()),
    autoUpdate_(true)
{
    renderTexture_ = new Texture2D(context_);
    depthTexture_ = new Texture2D(context_);
    viewport_ = new Viewport(context_);

    // Disable mipmaps since the texel ratio should be 1:1
    renderTexture_->SetNumLevels(1);
    depthTexture_->SetNumLevels(1);

    SubscribeToEvent(E_RENDERSURFACEUPDATE, URHO3D_HANDLER(View3D, HandleRenderSurfaceUpdate));
}

View3D::~View3D()
{
    ResetScene();
}

void View3D::RegisterObject(Context* context)
{
    context->RegisterFactory<View3D>(UI_CATEGORY);

    URHO3D_COPY_BASE_ATTRIBUTES(Window);
    // The texture format is API specific, so do not register it as a serializable attribute
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Update", GetAutoUpdate, SetAutoUpdate, true, AM_FILE);
    URHO3D_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Clip Children", true);
    URHO3D_UPDATE_ATTRIBUTE_DEFAULT_VALUE("Is Enabled", true);
}

void View3D::OnResize(const IntVector2& newSize, const IntVector2& delta)
{
    int width = newSize.x_;
    int height = newSize.y_;

    if (width > 0 && height > 0)
    {
        renderTexture_->SetSize(width, height, rttFormat_, TEXTURE_RENDERTARGET);
        depthTexture_->SetSize(width, height, Graphics::GetDepthStencilFormat(), TEXTURE_DEPTHSTENCIL);
        RenderSurface* surface = renderTexture_->GetRenderSurface();
        surface->SetViewport(0, viewport_);
        surface->SetUpdateMode(SURFACE_MANUALUPDATE);
        surface->SetLinkedDepthStencil(depthTexture_->GetRenderSurface());

        SetTexture(renderTexture_);
        SetImageRect(IntRect(0, 0, width, height));

        if (!autoUpdate_)
            surface->QueueUpdate();
    }
}

void View3D::SetView(Scene* scene, Camera* camera, bool ownScene)
{
    ResetScene();

    scene_ = scene;
    cameraNode_ = camera ? camera->GetNode() : nullptr;
    ownScene_ = ownScene;

    viewport_->SetScene(scene_);
    viewport_->SetCamera(camera);
    QueueUpdate();
}

void View3D::SetFormat(unsigned format)
{
    if (format != rttFormat_)
    {
        rttFormat_ = format;
        OnResize(GetSize(), IntVector2::ZERO);
    }
}

void View3D::SetAutoUpdate(bool enable)
{
    autoUpdate_ = enable;
}

void View3D::QueueUpdate()
{
    RenderSurface* surface = renderTexture_->GetRenderSurface();
    if (surface)
        surface->QueueUpdate();
}

Scene* View3D::GetScene() const
{
    return scene_;
}

Node* View3D::GetCameraNode() const
{
    return cameraNode_;
}

Texture2D* View3D::GetRenderTexture() const
{
    return renderTexture_;
}

Texture2D* View3D::GetDepthTexture() const
{
    return depthTexture_;
}

Viewport* View3D::GetViewport() const
{
    return viewport_;
}

void View3D::ResetScene()
{
    if (!scene_)
        return;

    if (!ownScene_)
    {
        RefCount* refCount = scene_->RefCountPtr();
        ++refCount->refs_;
        scene_ = nullptr;
        --refCount->refs_;
    }
    else
        scene_ = nullptr;
}

void View3D::HandleRenderSurfaceUpdate(StringHash eventType, VariantMap& eventData)
{
    if (autoUpdate_ && IsVisibleEffective())
        QueueUpdate();
}

}
