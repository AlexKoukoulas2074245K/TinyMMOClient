///------------------------------------------------------------------------------------------------
///  CardAttackGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 27/10/2023
///------------------------------------------------------------------------------------------------

#include <game/AchievementManager.h>
#include <game/ArtifactProductIds.h>
#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/CardAttackGameAction.h>
#include <game/gameactions/CardDestructionGameAction.h>
#include <game/gameactions/CardHistoryEntryAdditionGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/GameOverResurrectionCheckGameAction.h>
#include <game/gameactions/RodentsDigAnimationGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>

///------------------------------------------------------------------------------------------------

const std::string CardAttackGameAction::CARD_INDEX_PARAM = "cardIndex";
const std::string CardAttackGameAction::PLAYER_INDEX_PARAM = "playerIndex";
const std::string CARD_LIGHT_ATTACK_SFX = "sfx_light_attack";
const std::string CARD_MEDIUM_ATTACK_SFX = "sfx_medium_attack";
const std::string CARD_HEAVY_ATTACK_SFX = "sfx_heavy_attack";
const std::string CARD_SHIELD_ATTACK_SFX = "sfx_shield";

static const strutils::StringId GAME_OVER_CHECK_GAME_ACTION_NAME = strutils::StringId("GameOverResurrectionCheckGameAction");
static const strutils::StringId CARD_DESTRUCTION_GAME_ACTION_NAME = strutils::StringId("CardDestructionGameAction");
static const strutils::StringId CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME = strutils::StringId("CardHistoryEntryAdditionGameAction");
static const strutils::StringId RODENTS_DIG_ANIMATION_GAME_ACTION_NAME = strutils::StringId("RodentsDigAnimationGameAction");
static const strutils::StringId ATTACKING_CARD_PARTICLE_NAME = strutils::StringId("card_attack");

static const float ATTACKING_CARD_ANIMATION_Y_OFFSET = 0.16f;
static const float ATTACKING_CARD_CAMERA_SHAKE_DURATION = 0.25f;
static const float ATTACKING_CARD_CAMERA_SHAKE_STRENGTH = 0.005f;
static const float ATTACKING_CARD_PARTICLE_EMITTER_Z = 0.01f;
static const float ATTACKING_CARD_SHORT_ANIMATION_DURATION = 0.25f;
static const float ATTACKING_CARD_LONG_ANIMATION_DURATION = 0.4f;
static const float ATTACKING_CARD_ANIMATION_ELEVATED_Z = 20.0f;
static const float ATTACKING_CARD_CAMERA_SHAKE_MAX_DURATION = 1.5f;
static const float ATTACKING_CARD_CAMERA_SHAKE_MAX_STRENGTH = 0.037f;

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
    CardAttackGameAction::CARD_INDEX_PARAM,
    CardAttackGameAction::PLAYER_INDEX_PARAM
};

///------------------------------------------------------------------------------------------------

void CardAttackGameAction::VSetNewGameState()
{
    assert(mExtraActionParams.count(CARD_INDEX_PARAM) != 0);
    assert(mExtraActionParams.count(PLAYER_INDEX_PARAM) != 0);
    
    auto cardIndex = std::stoi(mExtraActionParams.at(CARD_INDEX_PARAM));
    auto attackingPlayerIndex = std::stoi(mExtraActionParams.at(PLAYER_INDEX_PARAM));
    auto& attackingPlayerBoardCards = mBoardState->GetPlayerStates()[attackingPlayerIndex].mPlayerBoardCards;
    const auto& attackingCardData = CardDataRepository::GetInstance().GetCardData(attackingPlayerBoardCards[cardIndex], attackingPlayerIndex);
    
    // Card has been destroyed in between this action's creation and it's invocation of setting state here
    if (mBoardState->GetPlayerStates()[attackingPlayerIndex].mBoardCardIndicesToDestroy.count(cardIndex))
    {
        return;
    }
    
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    auto& attackingPlayerOverrides = mBoardState->GetPlayerStates()[attackingPlayerIndex].mPlayerBoardCardStatOverrides;
    
    auto damage = attackingCardData.mCardDamage;
    
    if (!attackingPlayerOverrides.empty() && static_cast<int>(attackingPlayerOverrides.size()) > cardIndex && attackingPlayerOverrides[cardIndex].count(CardStatType::DAMAGE))
    {
        damage = math::Max(0, attackingPlayerOverrides[cardIndex].at(CardStatType::DAMAGE));
    }
    
    if (mBoardState->GetPlayerStates()[attackingPlayerIndex].mBoardModifiers.mGlobalCardStatModifiers.count(CardStatType::DAMAGE))
    {
        damage = math::Max(0, damage + mBoardState->GetPlayerStates()[attackingPlayerIndex].mBoardModifiers.mGlobalCardStatModifiers.at(CardStatType::DAMAGE));
    }
    
    if (attackingCardData.mCardFamily == game_constants::INSECTS_FAMILY_NAME)
    {
        activePlayerState.mPlayerPoisonStack++;
        
        if ((mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::DOUBLE_POISON_ATTACKS) != 0)
        {
            activePlayerState.mPlayerPoisonStack++;
        }
    }

    mPendingDamage = damage;
    mAmountOfArmorDamaged = 0;
    mAmountOfHealthDamaged = 0;
    mLifestealHealedAtLeast1Hp = false;
    
    if (damage > 0)
    {
        if (activePlayerState.mPlayerCurrentArmor > 0)
        {
            int startingArmorValue = activePlayerState.mPlayerCurrentArmor;
            activePlayerState.mPlayerCurrentArmor = math::Max(0, activePlayerState.mPlayerCurrentArmor - damage);
            damage = math::Max(0, damage - startingArmorValue);
            mAmountOfArmorDamaged = math::Min(startingArmorValue, mPendingDamage);
        }
        
        if (damage > 0 && activePlayerState.mPlayerCurrentArmor <= 0)
        {
            activePlayerState.mPlayerHealth -= damage;
            mAmountOfHealthDamaged = damage;
        }
        
        auto demonFangsLifesteal = DataRepository::GetInstance().GetStoryArtifactCount(artifacts::DEMON_FANGS) * 2;
        if (demonFangsLifesteal > 0 && mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX)
        {
            int oldHealth = mBoardState->GetInactivePlayerState().mPlayerHealth;
            
            mBoardState->GetInactivePlayerState().mPlayerHealth = math::Min(mBoardState->GetInactivePlayerState().mPlayerHealth + demonFangsLifesteal, DataRepository::GetInstance().GetStoryMaxHealth());
        
            mLifestealHealedAtLeast1Hp = oldHealth != mBoardState->GetInactivePlayerState().mPlayerHealth;
        }
        
        if ((mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::RODENT_LIFESTEAL) != 0)
        {
            int oldHealth = mBoardState->GetInactivePlayerState().mPlayerHealth;
            if (mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX)
            {
                mBoardState->GetInactivePlayerState().mPlayerHealth = math::Min(mBoardState->GetInactivePlayerState().mPlayerHealth + mPendingDamage, DataRepository::GetInstance().GetStoryMaxHealth());
            }
            else
            {
                mBoardState->GetInactivePlayerState().mPlayerHealth += mPendingDamage;
            }
            
            mLifestealHealedAtLeast1Hp = oldHealth != mBoardState->GetInactivePlayerState().mPlayerHealth;
        }
    }
    
    mGameActionEngine->AddGameAction(CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME,
    {
        { CardHistoryEntryAdditionGameAction::PLAYER_INDEX_PARAM, std::to_string(attackingPlayerIndex) },
        { CardHistoryEntryAdditionGameAction::CARD_INDEX_PARAM, std::to_string(cardIndex) },
        { CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_PARAM, CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_BATTLE },
        { CardHistoryEntryAdditionGameAction::IS_TURN_COUNTER_PARAM, "false"}
    });
    
    if (activePlayerState.mPlayerHealth <= 0)
    {
        activePlayerState.mPlayerHealth = 0;
        mGameActionEngine->AddGameAction(GAME_OVER_CHECK_GAME_ACTION_NAME,
        {
            { GameOverResurrectionCheckGameAction::VICTORIOUS_PLAYER_INDEX_PARAM, std::to_string(attackingPlayerIndex)}
        });
    }
   
    // Check for rodents respawn flow
    if (attackingCardData.mCardFamily == game_constants::RODENTS_FAMILY_NAME)
    {
        if (math::ControlledRandomFloat() <= game_constants::RODENTS_RESPAWN_CHANCE || (mBoardState->GetPlayerStates()[attackingPlayerIndex].mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::DIG_NO_FAIL) != 0)
        {
            mGameActionEngine->AddGameAction(RODENTS_DIG_ANIMATION_GAME_ACTION_NAME,
            {
                { RodentsDigAnimationGameAction::CARD_INDEX_PARAM, std::to_string(cardIndex) },
                { RodentsDigAnimationGameAction::PLAYER_INDEX_PARAM, std::to_string(attackingPlayerIndex) }
            });
            return;
        }
    }
    
    // Hero cards do not get destroyed at the end of turn
    if (mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX && cardIndex == 0 && mBoardState->GetInactivePlayerState().mHasHeroCard)
    {
        return;
    }
    
    mGameActionEngine->AddGameAction(CARD_DESTRUCTION_GAME_ACTION_NAME,
    {
        { CardDestructionGameAction::CARD_INDICES_PARAM, {"[" + std::to_string(cardIndex) + "]"}},
        { CardDestructionGameAction::PLAYER_INDEX_PARAM, std::to_string(attackingPlayerIndex)},
        { CardDestructionGameAction::IS_BOARD_CARD_PARAM, "true"},
        { CardDestructionGameAction::IS_TRAP_TRIGGER_PARAM, "false"},
    });
}

///------------------------------------------------------------------------------------------------

void CardAttackGameAction::VInitAnimation()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& animationManager = systemsEngine.GetAnimationManager();
    
    auto cardIndex = std::stoi(mExtraActionParams.at(CARD_INDEX_PARAM));
    auto attackingPayerIndex = std::stoi(mExtraActionParams.at(PLAYER_INDEX_PARAM));
    
    systemsEngine.GetSoundManager().PreloadSfx(CARD_LIGHT_ATTACK_SFX);
    systemsEngine.GetSoundManager().PreloadSfx(CARD_MEDIUM_ATTACK_SFX);
    systemsEngine.GetSoundManager().PreloadSfx(CARD_HEAVY_ATTACK_SFX);
    systemsEngine.GetSoundManager().PreloadSfx(CARD_SHIELD_ATTACK_SFX);
    
    mPendingAnimations = 0;
    
    // Card has been destroyed in between this action's creation and it's invocation of setting state here
    if (mBoardState->GetPlayerStates()[attackingPayerIndex].mBoardCardIndicesToDestroy.count(cardIndex))
    {
        return;
    }
    
    auto cardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers().at(attackingPayerIndex).at(cardIndex);
    
    mOriginalCardPosition = cardSoWrapper->mSceneObject->mPosition;
    mOriginalCardScale = cardSoWrapper->mSceneObject->mScale;
    
    // Enlargement animation
    {
        auto targetScale = mOriginalCardScale * 1.2f;
        auto targetPos = cardSoWrapper->mSceneObject->mPosition;
        targetPos.z += ATTACKING_CARD_ANIMATION_ELEVATED_Z;
        
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardSoWrapper->mSceneObject, targetPos, targetScale, ATTACKING_CARD_SHORT_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [&]()
        {
            mPendingAnimations--;
            
            // Move to target position animation
            {
                auto cardIndex = std::stoi(mExtraActionParams.at(CARD_INDEX_PARAM));
                auto attackingPayerIndex = std::stoi(mExtraActionParams.at(PLAYER_INDEX_PARAM));
                auto cardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers().at(attackingPayerIndex).at(cardIndex);
                
                auto targetPos = cardSoWrapper->mSceneObject->mPosition;
                targetPos.y += attackingPayerIndex == game_constants::LOCAL_PLAYER_INDEX ? ATTACKING_CARD_ANIMATION_Y_OFFSET : - ATTACKING_CARD_ANIMATION_Y_OFFSET;
                
                animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardSoWrapper->mSceneObject, targetPos, cardSoWrapper->mSceneObject->mScale, ATTACKING_CARD_SHORT_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [&]()
                {
                    auto cardIndex = std::stoi(mExtraActionParams.at(CARD_INDEX_PARAM));
                    auto attackingPayerIndex = std::stoi(mExtraActionParams.at(PLAYER_INDEX_PARAM));
                    
                    auto cardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers().at(attackingPayerIndex).at(cardIndex);
                    
                    systemsEngine.GetParticleManager().CreateParticleEmitterAtPosition
                    (
                        ATTACKING_CARD_PARTICLE_NAME,
                        glm::vec3(cardSoWrapper->mSceneObject->mPosition.x, cardSoWrapper->mSceneObject->mPosition.y, ATTACKING_CARD_PARTICLE_EMITTER_Z),
                        *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)
                     );
                    
                    card_utils::PlayCardAttackSfx(mPendingDamage, mAmountOfArmorDamaged);
                    
                    auto cameraShakeDuration = math::Min(ATTACKING_CARD_CAMERA_SHAKE_MAX_DURATION, ATTACKING_CARD_CAMERA_SHAKE_DURATION * (1.0f + 0.05f * std::powf(static_cast<float>(mPendingDamage), 2.0f)));
                    auto cameraShakeStrength = math::Min(ATTACKING_CARD_CAMERA_SHAKE_MAX_STRENGTH, ATTACKING_CARD_CAMERA_SHAKE_STRENGTH * (1.0f + 0.05f * std::powf(static_cast<float>(mPendingDamage), 2.0f)));
                    CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->GetCamera().Shake(cameraShakeDuration, cameraShakeStrength);
                    
                    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardSoWrapper->mSceneObject, mOriginalCardPosition, mOriginalCardScale, ATTACKING_CARD_LONG_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [&]()
                    {
                        mPendingAnimations--;
                    });
                    
                    animationManager.StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(cameraShakeDuration), [=]()
                    {
                        mPendingAnimations--;
                        if (mPendingDamage != 0)
                        {
                            if (mAmountOfArmorDamaged > 0)
                            {
                                events::EventSystem::GetInstance().DispatchEvent<events::ArmorChangeChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, mBoardState->GetActivePlayerState().mPlayerCurrentArmor);
                                
                                if (mAmountOfHealthDamaged > 0)
                                {
                                    mPendingAnimations++;
                                    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(game_constants::PER_ARMOR_DROPPED_DELAY_ANIMATION_DURATION_SECS * mAmountOfArmorDamaged), [&]()
                                    {
                                        events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
                                        mPendingAnimations--;
                                    });
                                }
                            }
                            else
                            {
                                events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
                            }
                            
                            if (DataRepository::GetInstance().GetStoryArtifactCount(artifacts::DEMON_FANGS) > 0 && mLifestealHealedAtLeast1Hp)
                            {
                                events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX);
                            }
                            
                            if ((mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::RODENT_LIFESTEAL) != 0 && mLifestealHealedAtLeast1Hp)
                            {
                                events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX);
                            }
                        }
                        
                        if (!cardSoWrapper->mCardData.IsSpell() && cardSoWrapper->mCardData.mCardFamily == game_constants::INSECTS_FAMILY_NAME)
                        {
                            events::EventSystem::GetInstance().DispatchEvent<events::PoisonStackChangeChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, mBoardState->GetActivePlayerState().mPlayerPoisonStack);
                        }
                    });
                });
            }
        });
    }
    
    mPendingAnimations = 3;
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult CardAttackGameAction::VUpdateAnimation(const float)
{
    if (mPendingAnimations == 0)
    {
        if (std::stoi(mExtraActionParams.at(PLAYER_INDEX_PARAM)) == game_constants::LOCAL_PLAYER_INDEX)
        {
            if (mPendingDamage >= 10)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::DEAL_10_DAMAGE);
            }
            
            if (mPendingDamage >= 20)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::DEAL_20_DAMAGE);
            }   
        }
        
        return ActionAnimationUpdateResult::FINISHED;
    }
    
    return ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool CardAttackGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& CardAttackGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
