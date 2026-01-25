///------------------------------------------------------------------------------------------------
///  RendererPlatformImpl.h
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/10/2023
///------------------------------------------------------------------------------------------------

#ifndef RendererPlatformImpl_h
#define RendererPlatformImpl_h

///------------------------------------------------------------------------------------------------

#include <engine/rendering/IRenderer.h>
#include <engine/CoreSystemsEngine.h>
#include <functional>
#include <memory>
#include <set>
#include <unordered_map>

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

class RendererPlatformImpl final: public IRenderer
{
    friend struct CoreSystemsEngine::SystemsImpl;
public:
    struct FontRenderingData
    {
        std::vector<glm::vec3> mGlyphPositions;
        std::vector<glm::vec3> mGlyphScales;
        std::vector<glm::vec2> mGlyphMinUVs;
        std::vector<glm::vec2> mGlyphMaxUVs;
        std::vector<float> mGlyphAlphas;
    };

    // FontName -> ShaderResourceId -> FontData map
    using FontRenderingDataMap = std::unordered_map<strutils::StringId, std::unordered_map<resources::ResourceId, FontRenderingData>, strutils::StringIdHasher>;
    
public:
    void VInitialize() override;
    void VBeginRenderPass() override;
    void VRenderScene(scene::Scene& scene) override;
    void VRenderSceneObjectsToTexture(const std::vector<std::shared_ptr<scene::SceneObject>>& sceneObjects, const rendering::Camera& camera) override;
    void VEndRenderPass() override;
    
private:
    RendererPlatformImpl() = default;
    
    void CreateIMGuiWidgets();
    void RenderSceneText(scene::Scene& scene);

private:
    std::vector<std::pair<rendering::Camera*, std::shared_ptr<scene::SceneObject>>> mSceneObjectsWithDeferredRendering;
    FontRenderingDataMap mFontRenderingPassData;
    std::vector<std::reference_wrapper<scene::Scene>> mCachedScenes;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* RendererPlatformImpl_h */
