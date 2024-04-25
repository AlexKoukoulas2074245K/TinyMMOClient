///------------------------------------------------------------------------------------------------
///  HeroCardEntryGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 02/01/2024
///------------------------------------------------------------------------------------------------

#include <game/ArtifactProductIds.h>
#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/CardHistoryEntryAdditionGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/HeroCardEntryGameAction.h>
#include <game/GameRuleEngine.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <game/TutorialManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>
#include <engine/utils/FileUtils.h>

///------------------------------------------------------------------------------------------------

static const std::string CARD_PLAY_SFX = "sfx_card_play";

static const strutils::StringId CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME = strutils::StringId("CardHistoryEntryAdditionGameAction");
static const strutils::StringId CARD_PLAY_PARTICLE_NAME = strutils::StringId("card_play");
static const strutils::StringId TOP_PLAYER_HEALTH_CONTAINER_BASE = strutils::StringId("health_crystal_top_base");
static const strutils::StringId TOP_PLAYER_HEALTH_CONTAINER_VALUE = strutils::StringId("health_crystal_top_value");
static const strutils::StringId CARD_PLAY_PARTICLE_EMITTER_NAME = strutils::StringId("card_play_emitter");

static const glm::vec3 HEALTH_VALUE_TEXT_OFFSET = {0.001, 0.001, 0.02f};
static const glm::vec3 HEALTH_BASE_OFFSET = {-0.0005f, 0.03f, 0.12f};
static const glm::vec3 TUTORIAL_BATTLE_1_ARROW_ORIGIN_POSITION = {-0.104f, -0.025f, 0.0f};
static const glm::vec3 TUTORIAL_BATTLE_1_ARROW_TARGET_POSITION = {-0.104f, 0.05f, 0.0f};
static const glm::vec3 TUTORIAL_BATTLE_2_ARROW_ORIGIN_POSITION = {-0.1075f, 0.03f, 0.0f};
static const glm::vec3 TUTORIAL_BATTLE_2_ARROW_TARGET_POSITION = {-0.1075f, -0.045f, 0.0f};
static const glm::vec3 TUTORIAL_BATTLE_3_ARROW_ORIGIN_POSITION = {0.0985f, 0.035f, 0.0f};
static const glm::vec3 TUTORIAL_BATTLE_3_ARROW_TARGET_POSITION = {0.0985f, -0.04f, 0.0f};
static const glm::vec3 TUTORIAL_BATTLE_4_ARROW_ORIGIN_POSITION = {-0.104f, -0.025f, 0.0f};
static const glm::vec3 TUTORIAL_BATTLE_4_ARROW_TARGET_POSITION = {-0.104f, 0.10f, 0.0f};

static const float CARD_CAMERA_SHAKE_DURATION = 0.25f;
static const float CARD_CAMERA_SHAKE_STRENGTH = 0.005f;
static const float CARD_PLAY_PARTICLE_EMITTER_Z = 0.01f;
static const float IN_GAME_PLAYED_CARD_ANIMATION_DURATION = 0.5f;
static const float HEALTH_CONTAINER_INIT_SCALE_FACTOR = 0.5f;
static const float HEALTH_CRYSTAL_ANIMATION_DELAY_SECS = 0.5f;
static const float HEALTH_CRYSTAL_ANIMATION_CURVE_MIDPOINT_Y_OFFSET = 0.05f;
static const float HEALTH_CRYSTAL_ANIMATION_DURATION_SECS = 1.0f;

static constexpr int MINI_BOSS_ARMOR = 2;
static constexpr int FINAL_BOSS_ARMOR = 4;

///------------------------------------------------------------------------------------------------

void HeroCardEntryGameAction::VSetNewGameState()
{
    auto& activePlayerState = mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX];
    
    assert(!DataRepository::GetInstance().GetNextStoryOpponentTexturePath().empty());
    
    CardData heroCardData = {};
    heroCardData.mCardFamily = DataRepository::GetInstance().GetNextStoryOpponentName() == game_constants::EMERALD_DRAGON_NAME.GetString() ? game_constants::DRAGON_FAMILY_NAME : game_constants::DEMONS_GENERIC_FAMILY_NAME;
    heroCardData.mCardId = 0; // to be filled by CardDataRepository
    heroCardData.mCardName = strutils::StringId(DataRepository::GetInstance().GetNextStoryOpponentName());
    heroCardData.mCardDamage = DataRepository::GetInstance().GetNextStoryOpponentDamage();
    heroCardData.mCardWeight = DataRepository::GetInstance().GetNextBattleTopPlayerWeightLimit();
    heroCardData.mCardShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::DEFAULT_SHADER_NAME);
    
    // "Localize" dynamically created hero card texture. This path could have come from an iPhone app.
    auto heroCardTextureFileName = fileutils::GetFileName(DataRepository::GetInstance().GetNextStoryOpponentTexturePath());
    heroCardData.mCardTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "story_cards/" + heroCardTextureFileName);
    
    mHeroCardId = CardDataRepository::GetInstance().InsertDynamicCardData(heroCardData);
    
    activePlayerState.mGoldenCardIds.push_back(mHeroCardId);
    activePlayerState.mPlayerBoardCards.push_back(mHeroCardId);
    
    mGameActionEngine->AddGameAction(CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME,
    {
        { CardHistoryEntryAdditionGameAction::PLAYER_INDEX_PARAM, std::to_string(game_constants::REMOTE_PLAYER_INDEX) },
        { CardHistoryEntryAdditionGameAction::CARD_INDEX_PARAM, std::to_string(activePlayerState.mPlayerBoardCards.size() - 1) },
        { CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_PARAM, CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_EFFECT },
        { CardHistoryEntryAdditionGameAction::IS_TURN_COUNTER_PARAM, "false"}
    });
    
    // Add mini boss & boss armor
    if (DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::TUTORIAL_MAP_BOSS_COORD && DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::TUTORIAL_MAP)
    {
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerCurrentArmor = MINI_BOSS_ARMOR;
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerArmorRecharge = MINI_BOSS_ARMOR;
    }
    else if (DataRepository::GetInstance().GetCurrentStoryMapNodeType() == StoryMap::NodeType::BOSS_ENCOUNTER)
    {
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerCurrentArmor = FINAL_BOSS_ARMOR;
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerArmorRecharge = FINAL_BOSS_ARMOR;
    }
    
    // Add player armor
    auto heavyArmorCount = DataRepository::GetInstance().GetStoryArtifactCount(artifacts::HEAVY_ARMOR) * 2;
    mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerArmorRecharge = heavyArmorCount;
    
    // Add resurrection
    mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mHasResurrectionActive = DataRepository::GetInstance().GetStoryArtifactCount(artifacts::GUARDIAN_ANGEL);
    mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mHasResurrectionActive = DataRepository::GetInstance().DoesCurrentStoryHaveMutation(game_constants::MUTATION_FINAL_BOSS_REVIVES) && DataRepository::GetInstance().GetCurrentStoryMapNodeType() == StoryMap::NodeType::BOSS_ENCOUNTER;
}

///------------------------------------------------------------------------------------------------

void HeroCardEntryGameAction::VInitAnimation()
{
    mAnimationState = AnimationState::ANIMATING_HERO_CARD;
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(CARD_PLAY_SFX);
    
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    auto cardData = CardDataRepository::GetInstance().GetCardData(mHeroCardId, game_constants::REMOTE_PLAYER_INDEX);
    
    auto heroCardSoWrapper = card_utils::CreateCardSoWrapper
    (
        &cardData,
        glm::vec3(0.0f, 1.0f, 0.0f),
        game_constants::TOP_PLAYER_BOARD_CARD_SO_NAME_PREFIX + std::to_string(mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerBoardCards.size() - 1),
        CardOrientation::FRONT_FACE,
        card_utils::GetCardRarity(mHeroCardId, game_constants::REMOTE_PLAYER_INDEX, *mBoardState),
        false,
        true,
        true,
        {},
        {},
        *sceneManager.FindScene(game_constants::BATTLE_SCENE)
    );
    
    // Animate played card to board
    events::EventSystem::GetInstance().DispatchEvent<events::HeroCardCreatedEvent>(heroCardSoWrapper);
    heroCardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers()[game_constants::REMOTE_PLAYER_INDEX].at(0);
    
    const auto& boardCards = mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerBoardCards;
    const auto& deadBoardCardIndices = mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mBoardCardIndicesToDestroy;
    const auto nonDeadBoardCardCount = card_utils::CalculateNonDeadCardsCount(boardCards, deadBoardCardIndices);
    
    auto targetPosition = card_utils::CalculateBoardCardPosition(nonDeadBoardCardCount - 1, nonDeadBoardCardCount, true);
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(heroCardSoWrapper->mSceneObject, targetPosition, heroCardSoWrapper->mSceneObject->mScale * game_constants::IN_GAME_PLAYED_CARD_SCALE_FACTOR, IN_GAME_PLAYED_CARD_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
    {
        card_utils::PlayCardPlaySfx(&heroCardSoWrapper->mCardData);
        
        CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->GetCamera().Shake(CARD_CAMERA_SHAKE_DURATION, CARD_CAMERA_SHAKE_STRENGTH);
        CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition
        (
            CARD_PLAY_PARTICLE_NAME,
            glm::vec3(targetPosition.x, targetPosition.y, CARD_PLAY_PARTICLE_EMITTER_Z),
            *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE),
            CARD_PLAY_PARTICLE_EMITTER_NAME
        );
        
        auto topHealthContainerBase = scene->FindSceneObject(TOP_PLAYER_HEALTH_CONTAINER_BASE);
        auto topHealthContainerValue = scene->FindSceneObject(TOP_PLAYER_HEALTH_CONTAINER_VALUE);
        
        auto heroCardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers()[game_constants::REMOTE_PLAYER_INDEX].at(0);
        topHealthContainerBase->mPosition = heroCardSoWrapper->mSceneObject->mPosition + HEALTH_BASE_OFFSET;
        topHealthContainerValue->mPosition = topHealthContainerBase->mPosition + HEALTH_VALUE_TEXT_OFFSET;
        
        auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*topHealthContainerValue);
        topHealthContainerValue->mPosition.x -= (boundingRect.topRight.x - boundingRect.bottomLeft.x)/2.0f;
        
        heroCardSoWrapper->mSceneObject->mShaderBoolUniformValues[game_constants::IS_HELD_CARD_UNIFORM_NAME] = false;
        mAnimationState = AnimationState::INITIALIZE_HEALTH_CRYSTAL_ANIMATION;
    });
    
    auto topHealthContainerBase = scene->FindSceneObject(TOP_PLAYER_HEALTH_CONTAINER_BASE);
    auto topHealthContainerValue = scene->FindSceneObject(TOP_PLAYER_HEALTH_CONTAINER_VALUE);
    
    mTargetHealthCrystalBasePosition = topHealthContainerBase->mPosition;
    mTargetHealthCrystalBaseScale = topHealthContainerBase->mScale;
    
    topHealthContainerBase->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    topHealthContainerBase->mScale *= HEALTH_CONTAINER_INIT_SCALE_FACTOR;
    
    mTargetHealthCrystalValuePosition = topHealthContainerValue->mPosition;
    mTargetHealthCrystalValueScale = topHealthContainerValue->mScale;
    
    topHealthContainerValue->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    topHealthContainerValue->mScale *= HEALTH_CONTAINER_INIT_SCALE_FACTOR;
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult HeroCardEntryGameAction::VUpdateAnimation(const float)
{
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    auto topHealthContainerBase = scene->FindSceneObject(TOP_PLAYER_HEALTH_CONTAINER_BASE);
    auto topHealthContainerValue = scene->FindSceneObject(TOP_PLAYER_HEALTH_CONTAINER_VALUE);
    
    switch (mAnimationState)
    {
        case AnimationState::ANIMATING_HERO_CARD:
        {
            auto heroCardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers()[game_constants::REMOTE_PLAYER_INDEX].at(0);
            topHealthContainerBase->mPosition = heroCardSoWrapper->mSceneObject->mPosition + HEALTH_BASE_OFFSET;
            topHealthContainerValue->mPosition = topHealthContainerBase->mPosition + HEALTH_VALUE_TEXT_OFFSET;
            
            auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*topHealthContainerValue);
            topHealthContainerValue->mPosition.x -= (boundingRect.topRight.x - boundingRect.bottomLeft.x)/2.0f;
        } break;
            
        case AnimationState::INITIALIZE_HEALTH_CRYSTAL_ANIMATION:
        {
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(topHealthContainerBase, mTargetHealthCrystalBasePosition, mTargetHealthCrystalBaseScale, HEALTH_CRYSTAL_ANIMATION_DURATION_SECS, animation_flags::IGNORE_X_COMPONENT | animation_flags::IGNORE_Y_COMPONENT | animation_flags::IGNORE_Z_COMPONENT, HEALTH_CRYSTAL_ANIMATION_DELAY_SECS, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
            {
                events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::BATTLE_1_TUTORIAL, TUTORIAL_BATTLE_1_ARROW_ORIGIN_POSITION * game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR, TUTORIAL_BATTLE_1_ARROW_TARGET_POSITION *  game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR);
                events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::BATTLE_2_TUTORIAL, TUTORIAL_BATTLE_2_ARROW_ORIGIN_POSITION * game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR, TUTORIAL_BATTLE_2_ARROW_TARGET_POSITION *  game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR);
                events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::BATTLE_3_TUTORIAL, TUTORIAL_BATTLE_3_ARROW_ORIGIN_POSITION * game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR, TUTORIAL_BATTLE_3_ARROW_TARGET_POSITION * game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR);
                
                if (mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerCurrentArmor > 0)
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::BATTLE_ARMOR_TUTORIAL, TUTORIAL_BATTLE_4_ARROW_ORIGIN_POSITION * game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR, TUTORIAL_BATTLE_4_ARROW_TARGET_POSITION * game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR);
                    
                    events::EventSystem::GetInstance().DispatchEvent<events::ArmorChangeChangeAnimationTriggerEvent>(true, mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerCurrentArmor);
                }
                
                mAnimationState = AnimationState::COMPLETE;
            });
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(topHealthContainerValue, mTargetHealthCrystalValuePosition, mTargetHealthCrystalValueScale, HEALTH_CRYSTAL_ANIMATION_DURATION_SECS, animation_flags::IGNORE_X_COMPONENT | animation_flags::IGNORE_Y_COMPONENT | animation_flags::IGNORE_Z_COMPONENT, HEALTH_CRYSTAL_ANIMATION_DELAY_SECS, math::LinearFunction, math::TweeningMode::EASE_OUT), [](){});
            
            auto crystalBaseMidwayPosition = (topHealthContainerBase->mPosition + mTargetHealthCrystalBasePosition)/2.0f;
            crystalBaseMidwayPosition.y += HEALTH_CRYSTAL_ANIMATION_CURVE_MIDPOINT_Y_OFFSET;
            
            auto crystalValueMidwayPosition = (topHealthContainerValue->mPosition + mTargetHealthCrystalValuePosition)/2.0f;
            crystalValueMidwayPosition.y += HEALTH_CRYSTAL_ANIMATION_CURVE_MIDPOINT_Y_OFFSET;
            
            math::BezierCurve crystalBaseCurve({ topHealthContainerBase->mPosition, crystalBaseMidwayPosition, mTargetHealthCrystalBasePosition });
            math::BezierCurve crystalValueCurve({ topHealthContainerValue->mPosition, crystalValueMidwayPosition, mTargetHealthCrystalValuePosition });
            
            animationManager.StartAnimation(std::make_unique<rendering::BezierCurveAnimation>(topHealthContainerBase, crystalBaseCurve, HEALTH_CRYSTAL_ANIMATION_DURATION_SECS, animation_flags::NONE, HEALTH_CRYSTAL_ANIMATION_DELAY_SECS), [](){});
            animationManager.StartAnimation(std::make_unique<rendering::BezierCurveAnimation>(topHealthContainerValue, crystalValueCurve, HEALTH_CRYSTAL_ANIMATION_DURATION_SECS, animation_flags::NONE, HEALTH_CRYSTAL_ANIMATION_DELAY_SECS), [](){});
            
            mAnimationState = AnimationState::ANIMATING_HEALTH_CRYSTAL;
        } break;

        default: break;
    }
    
    return mAnimationState == AnimationState::COMPLETE ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool HeroCardEntryGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& HeroCardEntryGameAction::VGetRequiredExtraParamNames() const
{
    static std::vector<std::string> extraParams;
    return extraParams;
}

///------------------------------------------------------------------------------------------------
