///------------------------------------------------------------------------------------------------
///  GoldenCardPlayedEffectGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 20/11/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/Animations.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <game/events/EventSystem.h>
#include <game/GameConstants.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/GoldenCardPlayedEffectGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>

///------------------------------------------------------------------------------------------------

static const float GOLDEN_CARD_LIGHT_EFFECT_ANIMATION_DURATION = 1.0f;
static const float GOLDEN_CARD_LIGHT_EFFECT_MIN_X = -0.3f;
static const float GOLDEN_CARD_LIGHT_EFFECT_MAX_X = 0.3f;

///------------------------------------------------------------------------------------------------

void GoldenCardPlayedEffectGameAction::VSetNewGameState()
{
}

///------------------------------------------------------------------------------------------------

void GoldenCardPlayedEffectGameAction::VInitAnimation()
{
    mFinished = false;
    
    mGoldenCardPlayedLightEffectX = GOLDEN_CARD_LIGHT_EFFECT_MIN_X;
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mGoldenCardPlayedLightEffectX, GOLDEN_CARD_LIGHT_EFFECT_MAX_X, GOLDEN_CARD_LIGHT_EFFECT_ANIMATION_DURATION),[=]()
    {
        mFinished = true;
    });
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult GoldenCardPlayedEffectGameAction::VUpdateAnimation(const float)
{
    for (const auto& cardSoWrappers: mBattleSceneLogicManager->GetBoardCardSoWrappers()[mBoardState->GetActivePlayerIndex()])
    {
        cardSoWrappers->mSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME] = mGoldenCardPlayedLightEffectX;
    }
    
    return mFinished ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool GoldenCardPlayedEffectGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& GoldenCardPlayedEffectGameAction::VGetRequiredExtraParamNames() const
{
    static std::vector<std::string> v;
    return v;
}

///------------------------------------------------------------------------------------------------
