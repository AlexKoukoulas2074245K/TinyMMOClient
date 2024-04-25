///------------------------------------------------------------------------------------------------
///  SceneObject.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 25/09/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef SceneObject_h
#define SceneObject_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/ResourceLoadingService.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <functional>
#include <game/GameConstants.h>
#include <unordered_map>
#include <variant>

///------------------------------------------------------------------------------------------------

namespace resources { using ResourceId = size_t; }

///------------------------------------------------------------------------------------------------

namespace scene
{

///------------------------------------------------------------------------------------------------

struct DefaultSceneObjectData
{
};

///------------------------------------------------------------------------------------------------

struct TextSceneObjectData
{
    std::string mText;
    strutils::StringId mFontName;
};

///------------------------------------------------------------------------------------------------

struct ParticleEmitterObjectData
{
    size_t mParticleCount;
    uint8_t mParticleFlags;
    
    resources::ResourceId mTextureResourceId;
    resources::ResourceId mShaderResourceId;
    
    std::vector<glm::vec3> mParticlePositions;
    std::vector<glm::vec3> mParticleVelocities;
    std::vector<float> mParticleLifetimeSecs;
    std::vector<float> mParticleSizes;
    std::vector<float> mParticleAngles;
    
    glm::vec3 mRotationAxis;
    glm::vec3 mParticleGravityVelocity;
    glm::vec2 mParticleLifetimeRangeSecs;
    glm::vec2 mParticlePositionXOffsetRange;
    glm::vec2 mParticlePositionYOffsetRange;
    glm::vec2 mParticleVelocityXOffsetRange;
    glm::vec2 mParticleVelocityYOffsetRange;
    glm::vec2 mParticleSizeRange;
    glm::vec2 mParticleInitialAngleRange;
    
    unsigned int mParticleVertexArrayObject;
    unsigned int mParticleVertexBuffer;
    unsigned int mParticleUVBuffer;
    unsigned int mParticlePositionsBuffer;
    unsigned int mParticleLifetimeSecsBuffer;
    unsigned int mParticleSizesBuffer;
    unsigned int mParticleAnglesBuffer;
    unsigned int mTotalParticlesSpawned;
    
    float mParticleGenerationMaxDelaySecs;
    float mParticleGenerationCurrentDelaySecs;
    float mParticleEnlargementSpeed;
    float mParticleRotationSpeed;
    
    std::function<void(float, ParticleEmitterObjectData&)> mCustomUpdateFunction;
};

///------------------------------------------------------------------------------------------------

enum class SnapToEdgeBehavior
{
    NONE,
    SNAP_TO_LEFT_EDGE,
    SNAP_TO_RIGHT_EDGE,
    SNAP_TO_TOP_EDGE,
    SNAP_TO_BOT_EDGE,
};

///------------------------------------------------------------------------------------------------

inline constexpr int EFFECT_TEXTURES_COUNT = 3;

///------------------------------------------------------------------------------------------------

class Scene;
struct SceneObject
{
    ~SceneObject()
    {
        if (std::holds_alternative<scene::ParticleEmitterObjectData>(mSceneObjectTypeData) && !CoreSystemsEngine::GetInstance().IsShuttingDown())
        {
            CoreSystemsEngine::GetInstance().GetParticleManager().RemoveParticleGraphicsData(*this);
        }
    }
    
    const Scene* mScene = nullptr;
    strutils::StringId mName = strutils::StringId();
    std::variant<DefaultSceneObjectData, TextSceneObjectData, ParticleEmitterObjectData> mSceneObjectTypeData;
    std::unordered_map<strutils::StringId, glm::vec3, strutils::StringIdHasher> mShaderVec3UniformValues;
    std::unordered_map<strutils::StringId, float, strutils::StringIdHasher> mShaderFloatUniformValues;
    std::unordered_map<strutils::StringId, int, strutils::StringIdHasher> mShaderIntUniformValues;
    std::unordered_map<strutils::StringId, bool, strutils::StringIdHasher> mShaderBoolUniformValues;
    glm::vec3 mPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 mRotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 mScale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 mBoundingRectMultiplier = glm::vec3(1.0f, 1.0f, 1.0f);
    resources::ResourceId mMeshResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_MESHES_ROOT + game_constants::DEFAULT_MESH_NAME);
    resources::ResourceId mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + game_constants::DEFAULT_TEXTURE_NAME);
    resources::ResourceId mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::DEFAULT_SHADER_NAME);
    resources::ResourceId mEffectTextureResourceIds[EFFECT_TEXTURES_COUNT] = {};
    SnapToEdgeBehavior mSnapToEdgeBehavior = SnapToEdgeBehavior::NONE;
    float mSnapToEdgeScaleOffsetFactor = 0.0f;
    bool mInvisible = false;
    bool mDeferredRendering = false;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* SceneObject_h */
