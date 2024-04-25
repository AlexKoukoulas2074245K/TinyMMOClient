///------------------------------------------------------------------------------------------------
///  CardPlayedParticleEffectGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 23/11/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/Animations.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/sound/SoundManager.h>
#include <game/events/EventSystem.h>
#include <game/GameConstants.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/CardPlayedParticleEffectGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>

///------------------------------------------------------------------------------------------------

static const std::string EXPLOSION_SFX = "sfx_explosion";
static const strutils::StringId PARTICLE_SCENE_OBJECT_NAME = strutils::StringId("card_played_particle_effect");
static const glm::vec3 PARTICLE_EMITTER_OFFSET = {0.0f, 0.0f, 0.01f};

///------------------------------------------------------------------------------------------------

void CardPlayedParticleEffectGameAction::VSetNewGameState()
{
}

///------------------------------------------------------------------------------------------------

void CardPlayedParticleEffectGameAction::VInitAnimation()
{
    const auto& lastPlayedCardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers()[mBoardState->GetActivePlayerIndex()].back();
    auto scene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    
    assert(!lastPlayedCardSoWrapper->mCardData.mParticleEffect.isEmpty());
    
    CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition(lastPlayedCardSoWrapper->mCardData.mParticleEffect, lastPlayedCardSoWrapper->mSceneObject->mPosition + PARTICLE_EMITTER_OFFSET, *scene, PARTICLE_SCENE_OBJECT_NAME);
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(EXPLOSION_SFX);
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult CardPlayedParticleEffectGameAction::VUpdateAnimation(const float)
{
    auto scene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    if (scene->FindSceneObject(PARTICLE_SCENE_OBJECT_NAME))
    {
        return ActionAnimationUpdateResult::ONGOING;
    }
    else
    {
        const auto& lastPlayedCardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers()[mBoardState->GetActivePlayerIndex()].back();
        if (lastPlayedCardSoWrapper->mCardData.mParticleShakeDurationSecs > 0.0f && lastPlayedCardSoWrapper->mCardData.mParticleShakeStrength > 0.0f)
        {
            CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(EXPLOSION_SFX);
            CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->GetCamera().Shake(lastPlayedCardSoWrapper->mCardData.mParticleShakeDurationSecs, lastPlayedCardSoWrapper->mCardData.mParticleShakeStrength);
        }
        
        return ActionAnimationUpdateResult::FINISHED;
    }
}

///------------------------------------------------------------------------------------------------

bool CardPlayedParticleEffectGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& CardPlayedParticleEffectGameAction::VGetRequiredExtraParamNames() const
{
    static std::vector<std::string> v;
    return v;
}

///------------------------------------------------------------------------------------------------
