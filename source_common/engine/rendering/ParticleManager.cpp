///------------------------------------------------------------------------------------------------
///  ParticleManager.cpp
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 18/10/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/rendering/OpenGL.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <engine/utils/OSMessageBox.h>
#include <nlohmann/json.hpp>
#include <numeric>

///------------------------------------------------------------------------------------------------

#define IS_FLAG_SET(flag) ((particleEmitterData.mParticleFlags & flag) != 0)

///------------------------------------------------------------------------------------------------

static const std::vector<std::vector<float>> PARTICLE_VERTEX_POSITIONS =
{
    {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f
    }
};

static const std::vector<float> PARTICLE_UVS =
{
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f
};

static int sParticleEmitterCount = 0;
static std::string PARTICLE_EMITTER_NAME_PREFIX = "particle_emitter_";
static std::string GENERIC_PARTICLE_SHADER_FILE_NAME = "generic_particle.vs";

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

void ParticleManager::UpdateSceneParticles(const float dtMillis, scene::Scene& scene)
{
    mParticleEmittersToDelete.clear();
    for (auto& sceneObject: scene.GetSceneObjects())
    {
        if (std::holds_alternative<scene::ParticleEmitterObjectData>(sceneObject->mSceneObjectTypeData))
        {
            auto& particleEmitterData = std::get<scene::ParticleEmitterObjectData>(sceneObject->mSceneObjectTypeData);
            
            if (IS_FLAG_SET(particle_flags::CUSTOM_UPDATE))
            {
                particleEmitterData.mCustomUpdateFunction(dtMillis, particleEmitterData);
                continue;
            }
            
            particleEmitterData.mParticleGenerationCurrentDelaySecs -= dtMillis/1000.0f;
            if (particleEmitterData.mParticleGenerationCurrentDelaySecs <= 0.0f)
            {
                particleEmitterData.mParticleGenerationCurrentDelaySecs = 0.0f;
            }
            
            size_t deadParticles = 0;
            for (size_t i = 0; i < particleEmitterData.mParticleCount; ++i)
            {
                // subtract from the particles lifetime
                particleEmitterData.mParticleLifetimeSecs[i] -= dtMillis/1000.0f;
                
                // if the lifetime is below add to the count of finished particles
                if (particleEmitterData.mParticleLifetimeSecs[i] <= 0.0f )
                {
                    if (IS_FLAG_SET(particle_flags::CONTINUOUS_PARTICLE_GENERATION) && particleEmitterData.mParticleGenerationCurrentDelaySecs <= 0.0f)
                    {
                        SpawnParticleAtIndex(i, sceneObject->mPosition, particleEmitterData);
                        particleEmitterData.mParticleGenerationCurrentDelaySecs = particleEmitterData.mParticleGenerationMaxDelaySecs;
                    }
                    else
                    {
                        particleEmitterData.mParticleLifetimeSecs[i] = 0.0f;
                        deadParticles++;
                    }
                }
                
                // move the particle up depending on the delta time
                if (IS_FLAG_SET(particle_flags::ENLARGE_OVER_TIME))
                {
                    particleEmitterData.mParticleSizes[i] += particleEmitterData.mParticleEnlargementSpeed * dtMillis;
                }
                
                // rotate the particle depending on the delta time
                if (IS_FLAG_SET(particle_flags::ROTATE_OVER_TIME))
                {
                    particleEmitterData.mParticleAngles[i] += particleEmitterData.mParticleRotationSpeed * dtMillis;
                }
                
                particleEmitterData.mParticleVelocities[i] += particleEmitterData.mParticleGravityVelocity * dtMillis;
                particleEmitterData.mParticlePositions[i] += particleEmitterData.mParticleVelocities[i] * dtMillis;
            }
            
            if (deadParticles == particleEmitterData.mParticleCount && !IS_FLAG_SET(particle_flags::CONTINUOUS_PARTICLE_GENERATION))
            {
                mParticleEmittersToDelete.push_back(sceneObject);
            }
            else
            {
                SortParticles(particleEmitterData);
            }
        }
    }
    
    for (const auto& particleEmitter: mParticleEmittersToDelete)
    {
        scene.RemoveSceneObject(particleEmitter->mName);
    }
}

///------------------------------------------------------------------------------------------------

const std::unordered_map<strutils::StringId, scene::ParticleEmitterObjectData, strutils::StringIdHasher> ParticleManager::GetLoadedParticleNamesToData() const
{
    return mParticleNamesToData;
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<scene::SceneObject> ParticleManager::CreateParticleEmitterAtPosition
(
    const strutils::StringId particleEmitterDefinitionName,
    const glm::vec3& pos,
    scene::Scene& scene,
    const strutils::StringId particleEmitterSceneObjectName /* = strutils::StringId() */,
    std::function<void(float, scene::ParticleEmitterObjectData&)> customUpdateFunction /* = nullptr */
)
{
    if (!mParticleNamesToData.count(particleEmitterDefinitionName))
    {
        ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "Unable to find particle definition", "Particle emitter definition: " + particleEmitterDefinitionName.GetString() + " could not be found.");
        return nullptr;
    }
    
    auto particleEmitterData = mParticleNamesToData.at(particleEmitterDefinitionName);
    
    auto particleSystemSo = scene.CreateSceneObject(particleEmitterSceneObjectName.isEmpty() ? strutils::StringId(PARTICLE_EMITTER_NAME_PREFIX + std::to_string(sParticleEmitterCount)) : particleEmitterSceneObjectName);
    particleSystemSo->mPosition = pos;
    particleSystemSo->mTextureResourceId = particleEmitterData.mTextureResourceId;
    particleSystemSo->mShaderResourceId = particleEmitterData.mShaderResourceId;
    
    assert(IS_FLAG_SET(particle_flags::PREFILLED) || IS_FLAG_SET(particle_flags::CONTINUOUS_PARTICLE_GENERATION) || IS_FLAG_SET(particle_flags::CUSTOM_UPDATE));
    
    particleEmitterData.mTotalParticlesSpawned = 0;
    
    particleEmitterData.mParticleLifetimeSecs.clear();
    particleEmitterData.mParticleVelocities.clear();
    particleEmitterData.mParticleSizes.clear();
    particleEmitterData.mParticleAngles.clear();
    particleEmitterData.mParticlePositions.clear();
    
    particleEmitterData.mParticleLifetimeSecs.resize(particleEmitterData.mParticleCount);
    particleEmitterData.mParticleVelocities.resize(particleEmitterData.mParticleCount);
    particleEmitterData.mParticleSizes.resize(particleEmitterData.mParticleCount);
    particleEmitterData.mParticleAngles.resize(particleEmitterData.mParticleCount);
    particleEmitterData.mParticlePositions.resize(particleEmitterData.mParticleCount);
    
    if (IS_FLAG_SET(particle_flags::CUSTOM_UPDATE))
    {
        assert(customUpdateFunction);
        particleEmitterData.mCustomUpdateFunction = customUpdateFunction;
    }
    
    particleSystemSo->mShaderVec3UniformValues[strutils::StringId("rotation_axis")] = glm::vec3(0.0f);
    if (IS_FLAG_SET(particle_flags::ROTATE_OVER_TIME) || IS_FLAG_SET(particle_flags::INITIALLY_ROTATED))
    {
        particleSystemSo->mShaderVec3UniformValues[strutils::StringId("rotation_axis")] = particleEmitterData.mRotationAxis;
    }
    
    for (size_t i = 0U; i < particleEmitterData.mParticleCount; ++i)
    {
        particleEmitterData.mParticleLifetimeSecs[i] = 0.0f;
        
        if (IS_FLAG_SET(particle_flags::PREFILLED))
        {
            SpawnParticleAtIndex(i, pos, particleEmitterData);
        }
    }
    
    GL_CALL(glGenVertexArrays(1, &particleEmitterData.mParticleVertexArrayObject));
    GL_CALL(glGenBuffers(1, &particleEmitterData.mParticleVertexBuffer));
    GL_CALL(glGenBuffers(1, &particleEmitterData.mParticleUVBuffer));
    GL_CALL(glGenBuffers(1, &particleEmitterData.mParticlePositionsBuffer));
    GL_CALL(glGenBuffers(1, &particleEmitterData.mParticleLifetimeSecsBuffer));
    GL_CALL(glGenBuffers(1, &particleEmitterData.mParticleSizesBuffer));
    GL_CALL(glGenBuffers(1, &particleEmitterData.mParticleAnglesBuffer));
    
    GL_CALL(glBindVertexArray(particleEmitterData.mParticleVertexArrayObject));
    
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, particleEmitterData.mParticleVertexBuffer));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, PARTICLE_VERTEX_POSITIONS[0].size() * sizeof(float) , PARTICLE_VERTEX_POSITIONS[0].data(), GL_STATIC_DRAW));
    
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, particleEmitterData.mParticleUVBuffer));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, PARTICLE_UVS.size() * sizeof(float) , PARTICLE_UVS.data(), GL_STATIC_DRAW));
    
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, particleEmitterData.mParticlePositionsBuffer));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, particleEmitterData.mParticleCount * sizeof(glm::vec3), particleEmitterData.mParticlePositions.data(), GL_DYNAMIC_DRAW));
    
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, particleEmitterData.mParticleLifetimeSecsBuffer));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, particleEmitterData.mParticleCount * sizeof(float), particleEmitterData.mParticleLifetimeSecs.data(), GL_DYNAMIC_DRAW));
    
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, particleEmitterData.mParticleSizesBuffer));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, particleEmitterData.mParticleCount * sizeof(float), particleEmitterData.mParticleSizes.data(), GL_DYNAMIC_DRAW));
    
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, particleEmitterData.mParticleAnglesBuffer));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, particleEmitterData.mParticleCount * sizeof(float), particleEmitterData.mParticleAngles.data(), GL_DYNAMIC_DRAW));
    
    particleSystemSo->mSceneObjectTypeData = std::move(particleEmitterData);
    
    sParticleEmitterCount++;
    
    return particleSystemSo;
}

///------------------------------------------------------------------------------------------------

int ParticleManager::SpawnParticleAtFirstAvailableSlot(scene::SceneObject& particleEmitterSceneObject)
{
    if (std::holds_alternative<scene::ParticleEmitterObjectData>(particleEmitterSceneObject.mSceneObjectTypeData))
    {
        const auto& particleEmitterData = std::get<scene::ParticleEmitterObjectData>(particleEmitterSceneObject.mSceneObjectTypeData);
        
        auto particleCount = particleEmitterData.mParticlePositions.size();
        
        for (size_t i = 0; i < particleCount; ++i)
        {
            if (particleEmitterData.mParticleLifetimeSecs[i] <= 0.0f)
            {
                SpawnParticleAtIndex(i, particleEmitterSceneObject);
                return static_cast<int>(i);
            }
        }
    }
    return 0;
}

///------------------------------------------------------------------------------------------------

void ParticleManager::LoadParticleData(const resources::ResourceReloadMode resourceReloadMode /* = resources::ResourceReloadMode::DONT_RELOAD */)
{
    mResourceReloadMode = resourceReloadMode;
  
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    
    auto particlesDefinitionJsonResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_DATA_ROOT + "particle_data.json", resourceReloadMode);
    const auto particlesJson =  nlohmann::json::parse(systemsEngine.GetResourceLoadingService().GetResource<resources::DataFileResource>(particlesDefinitionJsonResourceId).GetContents());
    
    for (const auto& particleObject: particlesJson["particle_data"])
    {
        scene::ParticleEmitterObjectData particleEmitterData = {};
        strutils::StringId particleName = strutils::StringId(particleObject["name"].get<std::string>());
        
        particleEmitterData.mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + particleObject["texture"].get<std::string>());
        particleEmitterData.mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + (particleObject.count("shader") ? particleObject["shader"].get<std::string>() : GENERIC_PARTICLE_SHADER_FILE_NAME));
        particleEmitterData.mParticleCount = particleObject["particle_count"].get<int>();
        
        particleEmitterData.mParticleFlags |= particleObject["prefilled"].get<bool>() ? particle_flags::PREFILLED : particle_flags::NONE;
        particleEmitterData.mParticleFlags |= particleObject["continuous_generation"].get<bool>() ? particle_flags::CONTINUOUS_PARTICLE_GENERATION : particle_flags::NONE;
        particleEmitterData.mParticleFlags |= particleObject["enlarge_over_time"].get<bool>() ? particle_flags::ENLARGE_OVER_TIME : particle_flags::NONE;
        particleEmitterData.mParticleFlags |= particleObject["rotate_over_time"].get<bool>() ? particle_flags::ROTATE_OVER_TIME : particle_flags::NONE;
        particleEmitterData.mParticleFlags |= particleObject["initially_rotated"].get<bool>() ? particle_flags::INITIALLY_ROTATED : particle_flags::NONE;
        particleEmitterData.mParticleFlags |= particleObject["custom_update"].get<bool>() ? particle_flags::CUSTOM_UPDATE : particle_flags::NONE;
        
        particleEmitterData.mParticleLifetimeRangeSecs.x = particleObject["lifetime_range"]["min"].get<float>();
        particleEmitterData.mParticleLifetimeRangeSecs.y = particleObject["lifetime_range"]["max"].get<float>();
        
        particleEmitterData.mParticlePositionXOffsetRange.x = particleObject["position_x_range"]["min"].get<float>();
        particleEmitterData.mParticlePositionXOffsetRange.y = particleObject["position_x_range"]["max"].get<float>();
        
        particleEmitterData.mParticlePositionYOffsetRange.x = particleObject["position_y_range"]["min"].get<float>();
        particleEmitterData.mParticlePositionYOffsetRange.y = particleObject["position_y_range"]["max"].get<float>();
        
        particleEmitterData.mParticleSizeRange.x = particleObject["particle_size_range"]["min"].get<float>();
        particleEmitterData.mParticleSizeRange.y = particleObject["particle_size_range"]["max"].get<float>();
        
        particleEmitterData.mParticleGravityVelocity = glm::vec3(0.0f);
        if (particleObject.count("gravity_velocity"))
        {
            particleEmitterData.mParticleGravityVelocity.x = particleObject["gravity_velocity"]["x"].get<float>();
            particleEmitterData.mParticleGravityVelocity.y = particleObject["gravity_velocity"]["y"].get<float>();
        }
        
        particleEmitterData.mParticleVelocityXOffsetRange = glm::vec2(0.0f);
        if (particleObject.count("velocity_x_range"))
        {
            particleEmitterData.mParticleVelocityXOffsetRange.x = particleObject["velocity_x_range"]["min"].get<float>();
            particleEmitterData.mParticleVelocityXOffsetRange.y = particleObject["velocity_x_range"]["max"].get<float>();
        }
        
        particleEmitterData.mParticleVelocityYOffsetRange = glm::vec2(0.0f);
        if (particleObject.count("velocity_y_range"))
        {
            particleEmitterData.mParticleVelocityYOffsetRange.x = particleObject["velocity_y_range"]["min"].get<float>();
            particleEmitterData.mParticleVelocityYOffsetRange.y = particleObject["velocity_y_range"]["max"].get<float>();
        }
        
        if (IS_FLAG_SET(particle_flags::ENLARGE_OVER_TIME))
        {
            particleEmitterData.mParticleEnlargementSpeed = particleObject["particle_enlargement_speed"].get<float>();
        }
        
        if (IS_FLAG_SET(particle_flags::CONTINUOUS_PARTICLE_GENERATION))
        {
            particleEmitterData.mParticleGenerationMaxDelaySecs = particleObject["particle_generation_delay_secs"].get<float>();
        }
        
        if (IS_FLAG_SET(particle_flags::ROTATE_OVER_TIME))
        {
            particleEmitterData.mParticleRotationSpeed = particleObject["particle_rotation_speed"].get<float>();
        }
        
        if (IS_FLAG_SET(particle_flags::INITIALLY_ROTATED))
        {
            particleEmitterData.mParticleInitialAngleRange.x = particleObject["particle_initial_angle_range"]["min"].get<float>();
            particleEmitterData.mParticleInitialAngleRange.y = particleObject["particle_initial_angle_range"]["max"].get<float>();
        }
        
        if (IS_FLAG_SET(particle_flags::ROTATE_OVER_TIME) || IS_FLAG_SET(particle_flags::INITIALLY_ROTATED))
        {
            auto rotationAxisString = particleObject["rotation_axis"].get<std::string>();
            if (rotationAxisString == "x") particleEmitterData.mRotationAxis.x = 1.0f;
            else if (rotationAxisString == "y") particleEmitterData.mRotationAxis.y = 1.0f;
            else if (rotationAxisString == "z") particleEmitterData.mRotationAxis.z = 1.0f;
        }
        
        mParticleNamesToData[particleName] = std::move(particleEmitterData);
    }
}

///------------------------------------------------------------------------------------------------

void ParticleManager::ReloadParticlesFromDisk()
{
    if (mResourceReloadMode == resources::ResourceReloadMode::RELOAD_EVERY_SECOND)
    {
        LoadParticleData(mResourceReloadMode);
    }
}

///------------------------------------------------------------------------------------------------

void ParticleManager::SortParticles(scene::ParticleEmitterObjectData& particleEmitterData) const
{
    // Create permutation index vector for final positions
    const auto particleCount = particleEmitterData.mParticleCount;
    
    std::vector<std::size_t> indexVec(particleCount);
    std::iota(indexVec.begin(), indexVec.end(), 0);
    std::sort(indexVec.begin(), indexVec.end(), [&](const size_t i, const size_t j)
    {
        return particleEmitterData.mParticlePositions[i].z < particleEmitterData.mParticlePositions[j].z;
    });
    
    // Create corrected vectors
    std::vector<glm::vec3> correctedPositions(particleCount);
    std::vector<glm::vec3> correctedDirections(particleCount);
    std::vector<float> correctedLifetimes(particleCount);
    std::vector<float> correctedSizes(particleCount);
    std::vector<float> correctedAngles(particleCount);
    
    for (size_t i = 0U; i < particleCount; ++i)
    {
        correctedPositions[i]  = particleEmitterData.mParticlePositions[indexVec[i]];
        correctedDirections[i] = particleEmitterData.mParticleVelocities[indexVec[i]];
        correctedLifetimes[i]  = particleEmitterData.mParticleLifetimeSecs[indexVec[i]];
        correctedSizes[i]      = particleEmitterData.mParticleSizes[indexVec[i]];
        correctedAngles[i]     = particleEmitterData.mParticleAngles[indexVec[i]];
    }
    
    particleEmitterData.mParticlePositions    = std::move(correctedPositions);
    particleEmitterData.mParticleVelocities   = std::move(correctedDirections);
    particleEmitterData.mParticleLifetimeSecs = std::move(correctedLifetimes);
    particleEmitterData.mParticleSizes        = std::move(correctedSizes);
    particleEmitterData.mParticleAngles       = std::move(correctedAngles);
}

///------------------------------------------------------------------------------------------------

void ParticleManager::ChangeParticleTexture(const strutils::StringId& particleEmitterDefinitionName, const resources::ResourceId textureResourceId)
{
    auto findIter = mParticleNamesToData.find(particleEmitterDefinitionName);
    if (findIter != mParticleNamesToData.end())
    {
        findIter->second.mTextureResourceId = textureResourceId;
    }
}

///------------------------------------------------------------------------------------------------

void ParticleManager::SpawnParticleAtIndex(const size_t index, const glm::vec3& sceneObjectPosition, scene::ParticleEmitterObjectData& particleEmitterData)
{
    const auto lifeTime = math::RandomFloat(particleEmitterData.mParticleLifetimeRangeSecs.x, particleEmitterData.mParticleLifetimeRangeSecs.y);
    const auto xOffset = math::RandomFloat(particleEmitterData.mParticlePositionXOffsetRange.x, particleEmitterData.mParticlePositionXOffsetRange.y);
    const auto yOffset = math::RandomFloat(particleEmitterData.mParticlePositionYOffsetRange.x, particleEmitterData.mParticlePositionYOffsetRange.y);
    const auto velXOffset = math::RandomFloat(particleEmitterData.mParticleVelocityXOffsetRange.x, particleEmitterData.mParticleVelocityXOffsetRange.y);
    const auto velYOffset = math::RandomFloat(particleEmitterData.mParticleVelocityYOffsetRange.x, particleEmitterData.mParticleVelocityYOffsetRange.y);
    const auto zOffset = math::RandomFloat(sceneObjectPosition.z - sceneObjectPosition.z * 0.0001f, sceneObjectPosition.z + sceneObjectPosition.z * 0.0001f);
    const auto size = math::RandomFloat(particleEmitterData.mParticleSizeRange.x, particleEmitterData.mParticleSizeRange.y);
    auto angle = 0.0f;
    
    if (IS_FLAG_SET(particle_flags::INITIALLY_ROTATED))
    {
        angle = math::RandomFloat(particleEmitterData.mParticleInitialAngleRange.x, particleEmitterData.mParticleInitialAngleRange.y);
    }
    
    particleEmitterData.mParticleLifetimeSecs[index] = lifeTime;
    particleEmitterData.mParticlePositions[index] = sceneObjectPosition;
    particleEmitterData.mParticlePositions[index].x += xOffset;
    particleEmitterData.mParticlePositions[index].y += yOffset;
    particleEmitterData.mParticlePositions[index].z = zOffset;
    particleEmitterData.mParticleVelocities[index].x += velXOffset;
    particleEmitterData.mParticleVelocities[index].y += velYOffset;
    particleEmitterData.mParticleSizes[index] = size;
    particleEmitterData.mParticleAngles[index] = angle;
    
    particleEmitterData.mTotalParticlesSpawned++;
}

///------------------------------------------------------------------------------------------------

void ParticleManager::SpawnParticleAtIndex(const size_t index, scene::SceneObject& particleEmitterSceneObject)
{
    if (std::holds_alternative<scene::ParticleEmitterObjectData>(particleEmitterSceneObject.mSceneObjectTypeData))
    {
        SpawnParticleAtIndex(index, particleEmitterSceneObject.mPosition, std::get<scene::ParticleEmitterObjectData>(particleEmitterSceneObject.mSceneObjectTypeData));
    }
}

///------------------------------------------------------------------------------------------------

void ParticleManager::RemoveParticleGraphicsData(scene::SceneObject& particleEmitterSceneObject)
{
    assert(std::holds_alternative<scene::ParticleEmitterObjectData>(particleEmitterSceneObject.mSceneObjectTypeData));
    auto& particleEmitterData = std::get<scene::ParticleEmitterObjectData>(particleEmitterSceneObject.mSceneObjectTypeData);
    GL_CALL(glDeleteBuffers(1, &particleEmitterData.mParticleUVBuffer));
    GL_CALL(glDeleteBuffers(1, &particleEmitterData.mParticleSizesBuffer));
    GL_CALL(glDeleteBuffers(1, &particleEmitterData.mParticleAnglesBuffer));
    GL_CALL(glDeleteBuffers(1, &particleEmitterData.mParticleVertexBuffer));
    GL_CALL(glDeleteBuffers(1, &particleEmitterData.mParticlePositionsBuffer));
    GL_CALL(glDeleteBuffers(1, &particleEmitterData.mParticleLifetimeSecsBuffer));
    GL_CALL(glDeleteVertexArrays(1, &particleEmitterData.mParticleVertexArrayObject));
}

///------------------------------------------------------------------------------------------------

void ParticleManager::RemoveParticleEmitterFlag(const uint8_t flag, const strutils::StringId particleEmitterSceneObjectName, scene::Scene& scene)
{
    auto particleSystemSo = scene.FindSceneObject(particleEmitterSceneObjectName);
    if (particleSystemSo && std::holds_alternative<scene::ParticleEmitterObjectData>(particleSystemSo->mSceneObjectTypeData))
    {
        std::get<scene::ParticleEmitterObjectData>(particleSystemSo->mSceneObjectTypeData).mParticleFlags &= (~flag);
    }
}

///------------------------------------------------------------------------------------------------

}
