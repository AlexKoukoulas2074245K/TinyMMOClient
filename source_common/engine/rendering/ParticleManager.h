///------------------------------------------------------------------------------------------------
///  ParticleManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 18/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef ParticleManager_h
#define ParticleManager_h

///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/StringUtils.h>
#include <memory>
#include <unordered_map>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene { class Scene; }
namespace scene { struct SceneObject; }
namespace scene { struct ParticleEmitterObjectData; }

///------------------------------------------------------------------------------------------------

inline const float DEFAULT_PARTICLE_ENLARGEMENT_SPEED = 0.00001f;

///------------------------------------------------------------------------------------------------

namespace particle_flags
{
    static constexpr uint8_t NONE                           = 0x0;
    static constexpr uint8_t PREFILLED                      = 0x1;
    static constexpr uint8_t CONTINUOUS_PARTICLE_GENERATION = 0x2;
    static constexpr uint8_t ENLARGE_OVER_TIME              = 0x4;
    static constexpr uint8_t ROTATE_OVER_TIME               = 0x8;
    static constexpr uint8_t INITIALLY_ROTATED              = 0x10;
    static constexpr uint8_t CUSTOM_UPDATE                  = 0x20;
}

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

class ParticleManager final
{
    friend struct CoreSystemsEngine::SystemsImpl;
    
public:
    void UpdateSceneParticles(const float dtMilis, scene::Scene& scene);

    std::shared_ptr<scene::SceneObject> CreateParticleEmitterAtPosition(const strutils::StringId particleEmitterDefinitionName, const glm::vec3& pos, scene::Scene& scene, const strutils::StringId particleEmitterSceneObjectName = strutils::StringId(), std::function<void(float, scene::ParticleEmitterObjectData&)> customUpdateFunction = nullptr);
    int SpawnParticleAtFirstAvailableSlot(scene::SceneObject& particleEmitterSceneObject);
    
    void RemoveParticleGraphicsData(scene::SceneObject& particleEmitterSceneObject);
    void RemoveParticleEmitterFlag(const uint8_t flag, const strutils::StringId particleEmitterSceneObjectName, scene::Scene& scene);
    void SortParticles(scene::ParticleEmitterObjectData& particleEmitterData) const;
    void ChangeParticleTexture(const strutils::StringId& particleEmitterDefinitionName, const resources::ResourceId textureResourceId);
    void LoadParticleData(const resources::ResourceReloadMode resourceReloadMode = resources::ResourceReloadMode::DONT_RELOAD);
    void ReloadParticlesFromDisk();
    
private:
    ParticleManager() = default;
    void SpawnParticleAtIndex(const size_t index, const glm::vec3& sceneObjectPosition, scene::ParticleEmitterObjectData& particleEmitterObjectData);
    void SpawnParticleAtIndex(const size_t index, scene::SceneObject& particleEmitterSceneObject);

private:
    std::vector<std::shared_ptr<scene::SceneObject>> mParticleEmittersToDelete;
    std::unordered_map<strutils::StringId, scene::ParticleEmitterObjectData, strutils::StringIdHasher> mParticleNamesToData;
    resources::ResourceReloadMode mResourceReloadMode;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* ParticleManager_h */
