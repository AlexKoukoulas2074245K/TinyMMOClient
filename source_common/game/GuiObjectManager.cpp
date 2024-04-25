///------------------------------------------------------------------------------------------------
///  GuiObjectManager.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 28/12/2023                                                       
///------------------------------------------------------------------------------------------------

#include <game/AchievementManager.h>
#include <game/AnimatedButton.h>
#include <game/AnimatedStatContainer.h>
#include <game/ArtifactProductIds.h>
#include <game/GuiObjectManager.h>
#include <game/GameConstants.h>
#include <game/events/EventSystem.h>
#include <game/DataRepository.h>
#include <game/ProductRepository.h>
#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/sound/SoundManager.h>
#include <engine/utils/Logging.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME = strutils::StringId("generic_stat_particle_emitter");
static const strutils::StringId HEALTH_REWARD_PARTICLE_EMITTER_SCENE_OBJECT_NAME = strutils::StringId("health_reward_stat_particle_emitter");
static const strutils::StringId COINS_REWARD_PARTICLE_EMITTER_SCENE_OBJECT_NAME = strutils::StringId("coins_reward_stat_particle_emitter");
static const strutils::StringId PARTICLE_EMITTER_DEFINITION_COIN_SMALL = strutils::StringId("coin_gain_small");
static const strutils::StringId PARTICLE_EMITTER_DEFINITION_COIN_LARGE = strutils::StringId("coin_gain_large");
static const strutils::StringId PARTICLE_EMITTER_DEFINITION_HEALTH_SMALL = strutils::StringId("health_refill_small");
static const strutils::StringId PARTICLE_EMITTER_DEFINITION_HEALTH_LARGE = strutils::StringId("health_refill_large");
static const strutils::StringId PARTICLE_EMITTER_DEFINITION_HEALTH_GAIN_SMALL = strutils::StringId("health_gain_small");
static const strutils::StringId PARTICLE_EMITTER_DEFINITION_HEALTH_GAIN_LARGE = strutils::StringId("health_gain_large");
static const strutils::StringId PARTICLE_EMITTER_DEFINITION_DAMAGE_GAIN_SMALL = strutils::StringId("damage_gain_small");
static const strutils::StringId PARTICLE_EMITTER_DEFINITION_DAMAGE_GAIN_LARGE = strutils::StringId("damage_gain_large");
static const strutils::StringId PARTICLE_EMITTER_DEFINITION_WEIGHT_GAIN_SMALL = strutils::StringId("weight_gain_small");
static const strutils::StringId PARTICLE_EMITTER_DEFINITION_WEIGHT_GAIN_LARGE = strutils::StringId("weight_gain_large");
static const strutils::StringId PARTICLE_EMITTER_DEFINITION_GENERIC_RARE_ITEM_LARGE = strutils::StringId("generic_rare_item_large");

static const std::string OVERLAY_TEXTURE_FILE_NAME = "overlay.png";
static const std::string COIN_VALUE_TEXT_SHADER_FILE_NAME = "animated_stat_container_value_object.vs";
static const std::string SETTINGS_ICON_TEXTURE_FILE_NAME = "settings_button_icon.png";
static const std::string STORY_CARDS_ICON_TEXTURE_FILE_NAME = "story_cards_button_icon.png";
static const std::string INVENTORY_ICON_TEXTURE_FILE_NAME = "inventory_button_icon.png";
static const std::string COIN_STACK_TEXTURE_FILE_NAME = "coin_stack.png";
static const std::string HEALTH_CRYSTAL_TEXTURE_FILE_NAME = "health_icon.png";
static const std::string HEALTH_CRYSTAL_SCENE_OBJECT_NAME_PREFIX = "health_crystal_";
static const std::string COINS_SFX = "sfx_coins";
static const std::string HEALTH_GAIN_SFX = "sfx_bump";
static const std::string MAX_HEALTH_GAIN_SFX = "sfx_max_health_gain";
static const std::string RARE_ITEM_COLLECTED_SFX = "sfx_collected";

static const glm::vec3 BATTLE_SCENE_SETTINGS_BUTTON_POSITION = {0.145f, 0.09f, 24.0f};
static const glm::vec3 SETTINGS_BUTTON_POSITION = {0.145f, 0.161f, 24.0f};
static const glm::vec3 BATTLE_SCENE_STORY_CARDS_BUTTON_POSITION = {0.145f, 0.09f, 24.0f};
static const glm::vec3 STORY_CARDS_BUTTON_POSITION = {0.145f, 0.161f, 24.0f};
static const glm::vec3 BATTLE_SCENE_INVENTORY_BUTTON_POSITION = {0.145f, 0.09f, 24.0f};
static const glm::vec3 INVENTORY_BUTTON_POSITION = {0.145f, 0.161f, 24.0f};
static const glm::vec3 SETTINGS_BUTTON_SCALE = {0.05f, 0.05f, 0.05f};
static const glm::vec3 STORY_CARDS_BUTTON_SCALE = {0.05f, 0.05f, 0.05f};
static const glm::vec3 INVENTORY_BUTTON_SCALE = {0.05f, 0.05f, 0.05f};
static const glm::vec3 COIN_STACK_POSITION = {0.145f, 0.101f, 24.0f};
static const glm::vec3 BATTLE_SCENE_COIN_STACK_POSITION = {0.145f, 0.06f, 24.0f};
static const glm::vec3 COIN_STACK_SCALE = {0.08f, 0.08f, 0.08f};
static const glm::vec3 COIN_VALUE_TEXT_POSITION = {0.155f, 0.105f, 24.0f};
static const glm::vec3 BATTLE_SCENE_COIN_VALUE_TEXT_POSITION = {0.155f, 0.06f, 24.0f};
static const glm::vec3 COIN_VALUE_TEXT_SCALE = {0.0004f, 0.0004f, 0.0004f};
static const glm::vec3 COIN_VALUE_TEXT_COLOR = {0.80f, 0.71f, 0.11f};
static const glm::vec3 BATTLE_SCENE_HEALTH_CRYSTAL_POSITION = {0.145f, 0.02f, 24.0f};
static const glm::vec3 HEALTH_CRYSTAL_POSITION = {0.145f, 0.04f, 24.0f};
static const glm::vec3 STAT_PARTICLE_INIT_POSITION_OFFSET = { 0.0f, 0.0f, 0.7f };
static const glm::vec3 STAT_PARTICLE_TARGET_POSITION_OFFSET = { -0.02f, -0.01f, -0.001f };
static const glm::vec3 STAT_GAIN_BATTLE_PARTICLE_OFFSET_POSITION = {0.0f, -0.04f, -0.01f};
static const glm::vec3 STAT_GAIN_PARTICLE_OFFSET_POSITION = {0.0f, -0.08f, -0.01f};
static const glm::vec3 EXTRA_DAMAGE_WEIGHT_PARTICLE_ORIGIN_POSITION = {-0.025f, -0.12f, 23.5f};
static const glm::vec3 GENERIC_RARE_ITEM_PARTICLE_ORIGIN_POSITION = {0.0f, 0.0, 23.5f};

static const glm::vec2 RARE_ITEM_COLLECTED_ANIMATION_MIN_MAX_OFFSETS = {-0.3f, 0.3f};
static const glm::vec2 STAT_FLYING_PARTICLE_MIN_MAX_Y_OFFSET = {-0.1f, 0.1f};
static const glm::vec2 STAT_FLYING_PARTYCLE_MIN_MAX_Z_OFFSET = {0.01f, 0.02f};

static const float COIN_PARTICLE_RESPAWN_TICK_SECS = 0.025f;
static const float HEALTH_PARTICLE_RESPAWN_TICK_SECS = 0.125f;
static const float SETTINGS_BUTTON_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR = 8.25;
static const float INVENTORY_BUTTON_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR = 50.5f;
static const float STORY_CARDS_BUTTON_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR = 29.25f;
static const float COIN_STACK_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR = 1.4f;
static const float COIN_VALUE_TEXT_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR = 280.0f;
static const float HEALTH_CRYSTAL_BASE_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR = 1.0f;
static const float HEALTH_CRYSTAL_VALUE_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR = 260.0f;
static const float HEALTH_CRYSTAL_CONTAINER_CUSTOM_SCALE_FACTOR = 2.0f;
static const float BATTLE_SCENE_SCALE_FACTOR = 0.5f;
static const float STAT_PARTICLE_ANIMATION_DURATION_MAX_SECS = 0.65f;
static const float STAT_PARTICLE_ANIMATION_DURATION_MIN_SECS = 0.85f;
static const float STAT_GAIN_PARTICLE_RESPAWN_SECS = 0.2f;
static const float STAT_GAIN_ANIMATION_DURATION_SECS = 2.0f;
static const float STAT_GAIN_PARTICLE_LIFETIME_SPEED = 0.002f;
static const float MAX_HEALTH_STAT_GAIN_PARTICLE_LIFETIME_SPEED = 0.004f;
static const float RARE_ITEM_COLLECTED_ANIMATION_MIN_ALPHA = 0.3f;
static const float RARE_ITEM_COLLECTED_ANIMATION_LIBRARY_ICON_PULSE_FACTOR = 1.25f;
static const float RARE_ITEM_COLLECTED_ANIMATION_LIBRARY_ICON_PULSE_DURATION_SECS = 0.1f;
static const float RARE_ITEM_COLLECTED_ANIMATION_DURATION_SECS = 3.0f;
static const float STAT_PARTICLE_EMITTER_MIN_Z = 19.0f;

///------------------------------------------------------------------------------------------------

GuiObjectManager::GuiObjectManager(std::shared_ptr<scene::Scene> scene)
    : mScene(scene)
{
    // Sync any desynced values with delayed displays.
    // Might not be the best place to do this.
    DataRepository::GetInstance().CurrencyCoins().SetDisplayedValue(DataRepository::GetInstance().CurrencyCoins().GetValue());
    DataRepository::GetInstance().StoryCurrentHealth().SetDisplayedValue(DataRepository::GetInstance().StoryCurrentHealth().GetValue());
    
    auto forBattleScene = scene->GetName() == game_constants::BATTLE_SCENE;
    auto extraScaleFactor = forBattleScene ? BATTLE_SCENE_SCALE_FACTOR : 1.0f;
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(COINS_SFX);
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(HEALTH_GAIN_SFX);
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(MAX_HEALTH_GAIN_SFX);
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(RARE_ITEM_COLLECTED_SFX);
    
    mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        forBattleScene ? BATTLE_SCENE_SETTINGS_BUTTON_POSITION : SETTINGS_BUTTON_POSITION,
        extraScaleFactor * SETTINGS_BUTTON_SCALE,
        SETTINGS_ICON_TEXTURE_FILE_NAME,
        game_constants::GUI_SETTINGS_BUTTON_SCENE_OBJECT_NAME,
        [=](){ OnSettingsButtonPressed(); },
        *scene,
        scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE,
        SETTINGS_BUTTON_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR / extraScaleFactor
    ));
    
    mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        forBattleScene ? BATTLE_SCENE_STORY_CARDS_BUTTON_POSITION : STORY_CARDS_BUTTON_POSITION,
        extraScaleFactor * STORY_CARDS_BUTTON_SCALE,
        STORY_CARDS_ICON_TEXTURE_FILE_NAME,
        game_constants::GUI_STORY_CARDS_BUTTON_SCENE_OBJECT_NAME,
        [=](){ OnStoryCardsButtonPressed(); },
        *scene,
        scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE,
        STORY_CARDS_BUTTON_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR / extraScaleFactor
    ));
    
    mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        forBattleScene ? BATTLE_SCENE_INVENTORY_BUTTON_POSITION : INVENTORY_BUTTON_POSITION,
        extraScaleFactor * INVENTORY_BUTTON_SCALE,
        INVENTORY_ICON_TEXTURE_FILE_NAME,
        game_constants::GUI_INVENTORY_BUTTON_SCENE_OBJECT_NAME,
        [=](){ OnInventoryButtonPressed(); },
        *scene,
        scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE,
        INVENTORY_BUTTON_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR / extraScaleFactor
    ));
    
    auto coinStackSceneObject = scene->CreateSceneObject(game_constants::GUI_COIN_STACK_SCENE_OBJECT_NAME);
    coinStackSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    coinStackSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + COIN_STACK_TEXTURE_FILE_NAME);
    coinStackSceneObject->mPosition = forBattleScene ? BATTLE_SCENE_COIN_STACK_POSITION : COIN_STACK_POSITION;
    coinStackSceneObject->mScale = extraScaleFactor * COIN_STACK_SCALE;
    coinStackSceneObject->mSnapToEdgeBehavior = scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE;
    coinStackSceneObject->mSnapToEdgeScaleOffsetFactor = COIN_STACK_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR;
    
    scene::TextSceneObjectData coinValueText;
    coinValueText.mFontName = game_constants::DEFAULT_FONT_NAME;
    coinValueText.mText = std::to_string(DataRepository::GetInstance().CurrencyCoins().GetValue());
    auto coinValueTextSceneObject = scene->CreateSceneObject(game_constants::GUI_COIN_VALUE_TEXT_SCENE_OBJECT_NAME);
    coinValueTextSceneObject->mSceneObjectTypeData = std::move(coinValueText);
    coinValueTextSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + COIN_VALUE_TEXT_SHADER_FILE_NAME);
    coinValueTextSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = COIN_VALUE_TEXT_COLOR;
    coinValueTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    coinValueTextSceneObject->mPosition = forBattleScene ? BATTLE_SCENE_COIN_VALUE_TEXT_POSITION : COIN_VALUE_TEXT_POSITION;
    coinValueTextSceneObject->mScale = extraScaleFactor * COIN_VALUE_TEXT_SCALE;
    coinValueTextSceneObject->mSnapToEdgeBehavior = scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE;
    coinValueTextSceneObject->mSnapToEdgeScaleOffsetFactor = COIN_VALUE_TEXT_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR;
    
    mHealthStatContainer = std::make_unique<AnimatedStatContainer>(forBattleScene ? BATTLE_SCENE_HEALTH_CRYSTAL_POSITION : HEALTH_CRYSTAL_POSITION, HEALTH_CRYSTAL_TEXTURE_FILE_NAME, HEALTH_CRYSTAL_SCENE_OBJECT_NAME_PREFIX, &DataRepository::GetInstance().StoryCurrentHealth().GetDisplayedValue(), forBattleScene, *scene, scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE, extraScaleFactor * HEALTH_CRYSTAL_CONTAINER_CUSTOM_SCALE_FACTOR);
    mHealthStatContainer->ForceSetDisplayedValue(DataRepository::GetInstance().StoryCurrentHealth().GetValue());
    
    mHealthStatContainer->GetSceneObjects()[0]->mSnapToEdgeScaleOffsetFactor = HEALTH_CRYSTAL_BASE_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR;
    mHealthStatContainer->GetSceneObjects()[1]->mSnapToEdgeScaleOffsetFactor = HEALTH_CRYSTAL_VALUE_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR;
    
    Update(0.0f);
    
    events::EventSystem::GetInstance().RegisterForEvent<events::CoinRewardEvent>(this, &GuiObjectManager::OnCoinReward);
    events::EventSystem::GetInstance().RegisterForEvent<events::HealthRefillRewardEvent>(this, &GuiObjectManager::OnHealthRefillReward);
    events::EventSystem::GetInstance().RegisterForEvent<events::MaxHealthGainRewardEvent>(this, &GuiObjectManager::OnMaxHealthGainReward);
    events::EventSystem::GetInstance().RegisterForEvent<events::RareItemCollectedEvent>(this, &GuiObjectManager::OnRareItemCollected);
}

///------------------------------------------------------------------------------------------------

GuiObjectManager::~GuiObjectManager()
{
}

///------------------------------------------------------------------------------------------------

GuiUpdateInteractionResult GuiObjectManager::Update(const float dtMillis, const bool allowButtonInput /* = true */)
{
    GuiUpdateInteractionResult interactionResult = GuiUpdateInteractionResult::DID_NOT_CLICK_GUI_BUTTONS;
    
    if (allowButtonInput)
    {
        for (auto& animatedButton: mAnimatedButtons)
        {
            if (animatedButton->Update(dtMillis) == ButtonUpdateInteractionResult::CLICKED)
            {
                interactionResult = GuiUpdateInteractionResult::CLICKED_GUI_BUTTONS;
            }
        }
    }
    
    auto& currentHealth = DataRepository::GetInstance().StoryCurrentHealth();
    currentHealth.SetValue(math::Max(0, currentHealth.GetValue()));
    currentHealth.SetDisplayedValue(math::Max(0, currentHealth.GetDisplayedValue()));
    
    mHealthStatContainer->Update(dtMillis);
    SetCoinValueText();
    
    return interactionResult;
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::SetCoinValueText()
{
    auto coinValue = DataRepository::GetInstance().CurrencyCoins().GetDisplayedValue();

    if (coinValue < 1000)
    {
        std::get<scene::TextSceneObjectData>(mScene->FindSceneObject(game_constants::GUI_COIN_VALUE_TEXT_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = std::to_string(coinValue);
    }
    else if (coinValue < 1000000)
    {
        std::get<scene::TextSceneObjectData>(mScene->FindSceneObject(game_constants::GUI_COIN_VALUE_TEXT_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = std::to_string(coinValue/1000) + "." + std::to_string((coinValue % 1000)/100) + "k";
    }
    else
    {
        std::get<scene::TextSceneObjectData>(mScene->FindSceneObject(game_constants::GUI_COIN_VALUE_TEXT_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = std::to_string(coinValue/1000000) + "." + std::to_string((coinValue % 1000000)/100000) + "m";
    }
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::OnWindowResize()
{
    mHealthStatContainer->RealignBaseAndValueSceneObjects();
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::ResetDisplayedCurrencyCoins()
{
    DataRepository::GetInstance().CurrencyCoins().SetDisplayedValue(DataRepository::GetInstance().CurrencyCoins().GetValue());
    SetCoinValueText();
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::ForceSetStoryHealthValue(const int storyHealthValue)
{
    mHealthStatContainer->ForceSetDisplayedValue(storyHealthValue);
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::StopRewardAnimation()
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    animationManager.StopAllAnimations();
    
    auto sceneToStopParticleEmitters = mScene->GetName() == game_constants::BATTLE_SCENE && CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WHEEL_OF_FORTUNE_SCENE) ? CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WHEEL_OF_FORTUNE_SCENE) : mScene;
    sceneToStopParticleEmitters->RemoveSceneObject(GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME);
    sceneToStopParticleEmitters->RemoveSceneObject(COINS_REWARD_PARTICLE_EMITTER_SCENE_OBJECT_NAME);
    sceneToStopParticleEmitters->RemoveSceneObject(HEALTH_REWARD_PARTICLE_EMITTER_SCENE_OBJECT_NAME);
    
    mScene->RemoveSceneObject(GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME);
    mScene->RemoveSceneObject(COINS_REWARD_PARTICLE_EMITTER_SCENE_OBJECT_NAME);
    mScene->RemoveSceneObject(HEALTH_REWARD_PARTICLE_EMITTER_SCENE_OBJECT_NAME);
}

///------------------------------------------------------------------------------------------------

int GuiObjectManager::GetStoryHealthContainerCurrentValue() const
{
    return mHealthStatContainer->GetDisplayedValue();
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::AnimateStatParticlesFlyingToGui(const glm::vec3& originPosition, const StatParticleType statParticleType, const long long statAmount)
{
    auto forBattleScene = mScene->GetName() == game_constants::BATTLE_SCENE;
    auto& particleManager = CoreSystemsEngine::GetInstance().GetParticleManager();
    
    auto particleDefinition = strutils::StringId();
    auto particleEmitterName = strutils::StringId();
    
    auto baseEmitterPosition = originPosition;
    baseEmitterPosition.z = math::Max(STAT_PARTICLE_EMITTER_MIN_Z, baseEmitterPosition.z);
    
    switch (statParticleType)
    {
        case StatParticleType::COINS:
        {
            particleEmitterName = COINS_REWARD_PARTICLE_EMITTER_SCENE_OBJECT_NAME;
            particleDefinition = forBattleScene ? PARTICLE_EMITTER_DEFINITION_COIN_SMALL : PARTICLE_EMITTER_DEFINITION_COIN_LARGE;
        } break;
        case StatParticleType::HEALTH:
        {
            particleEmitterName = HEALTH_REWARD_PARTICLE_EMITTER_SCENE_OBJECT_NAME;
            particleDefinition = forBattleScene ? PARTICLE_EMITTER_DEFINITION_HEALTH_SMALL : PARTICLE_EMITTER_DEFINITION_HEALTH_LARGE;
        } break;
    }
    
    mParticleEmitterTimeAccums[particleEmitterName] = 0.0f;
    mScene->RemoveSceneObject(particleEmitterName);

    auto particleEmitterSceneObject = particleManager.CreateParticleEmitterAtPosition(particleDefinition, baseEmitterPosition, *mScene, particleEmitterName, [=](float dtMillis, scene::ParticleEmitterObjectData& particleEmitterData)
    {
        auto& particleManager = CoreSystemsEngine::GetInstance().GetParticleManager();
        auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
        
        auto targetRespawnSecs = 0.0f;
        auto targetPosition = STAT_PARTICLE_TARGET_POSITION_OFFSET;
        switch (statParticleType)
        {
            case StatParticleType::COINS:
            {
                targetPosition += mScene->FindSceneObject(game_constants::GUI_COIN_STACK_SCENE_OBJECT_NAME)->mPosition;
                targetRespawnSecs = COIN_PARTICLE_RESPAWN_TICK_SECS;
            } break;
                
            case StatParticleType::HEALTH:
            {
                if (!mBattleLootHealthRefillCase)
                {
                    targetPosition += mHealthStatContainer->GetSceneObjects().front()->mPosition;
                }
                else
                {
                    targetPosition += game_constants::HEALTH_CRYSTAL_BOT_POSITION;
                }
                
                targetRespawnSecs = HEALTH_PARTICLE_RESPAWN_TICK_SECS;
            } break;
        }
        
        auto& particleEmitterSceneObject = *mScene->FindSceneObject(particleEmitterName);
        
        mParticleEmitterTimeAccums[particleEmitterName] += dtMillis/1000.0f;

        if (mParticleEmitterTimeAccums[particleEmitterName] > targetRespawnSecs)
        {
            mParticleEmitterTimeAccums[particleEmitterName] = 0.0f;
            
            auto particlesToSpawn = 1;
            if (statAmount > 100)
            {
                particlesToSpawn = 5;
            }
            if (statAmount > 1000)
            {
                particlesToSpawn = 10;
            }
            
            for (auto i = 0; i < particlesToSpawn; ++i)
            {
                if (static_cast<int>(particleEmitterData.mTotalParticlesSpawned) < statAmount)
                {
                    int particleIndex = particleManager.SpawnParticleAtFirstAvailableSlot(particleEmitterSceneObject);
                    
                    particleEmitterData.mParticlePositions[particleIndex] = baseEmitterPosition + STAT_PARTICLE_INIT_POSITION_OFFSET;
                    
                    glm::vec3 midPosition = (particleEmitterData.mParticlePositions[particleIndex] + targetPosition)/2.0f;
                    midPosition.y += forBattleScene ?
                    math::RandomFloat(STAT_FLYING_PARTICLE_MIN_MAX_Y_OFFSET.s, STAT_FLYING_PARTICLE_MIN_MAX_Y_OFFSET.t) :
                    math::RandomFloat(2.0f * STAT_FLYING_PARTICLE_MIN_MAX_Y_OFFSET.s, 2.0f * STAT_FLYING_PARTICLE_MIN_MAX_Y_OFFSET.t);
                    midPosition.z = (particleEmitterData.mParticlePositions[particleIndex].z + targetPosition.z)/2.0f + math::RandomFloat(STAT_FLYING_PARTYCLE_MIN_MAX_Z_OFFSET.s, STAT_FLYING_PARTYCLE_MIN_MAX_Z_OFFSET.t);
                    
                    math::BezierCurve curve({particleEmitterData.mParticlePositions[particleIndex], midPosition, targetPosition});
                    
                    animationManager.StartAnimation(std::make_unique<rendering::BezierCurveAnimation>(particleEmitterData.mParticlePositions[particleIndex], curve, math::RandomFloat(STAT_PARTICLE_ANIMATION_DURATION_MIN_SECS, STAT_PARTICLE_ANIMATION_DURATION_MAX_SECS)), [=]()
                    {
                        std::get<scene::ParticleEmitterObjectData>(mScene->FindSceneObject(particleEmitterName)->mSceneObjectTypeData).mParticleLifetimeSecs[particleIndex] = 0.0f;
                        
                        switch (statParticleType)
                        {
                            case StatParticleType::COINS:
                            {
                                // Animation only coin change
                                auto& coins = DataRepository::GetInstance().CurrencyCoins();
                                coins.SetDisplayedValue(coins.GetDisplayedValue() + 1);
                                CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(COINS_SFX);
                            } break;
                                
                            case StatParticleType::HEALTH:
                            {
                                // Animation only health change
                                auto& health = DataRepository::GetInstance().StoryCurrentHealth();
                                health.SetDisplayedValue(health.GetDisplayedValue() + 1);
                                CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(HEALTH_GAIN_SFX);
                                events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(false);
                            } break;
                        }
                        
                        if (CoreSystemsEngine::GetInstance().GetAnimationManager().GetAnimationCountPlayingWithName(game_constants::STAT_PARTICLE_FLYING_ANIMATION_NAME) == 1)
                        {
                            events::EventSystem::GetInstance().DispatchEvent<events::GuiRewardAnimationFinishedEvent>();
                        }
                        
                    }, game_constants::STAT_PARTICLE_FLYING_ANIMATION_NAME);
                }
            }
        }
    });
    particleEmitterSceneObject->mDeferredRendering = true;
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::AnimateStatGainParticles(const glm::vec3& originPosition, const StatGainParticleType statGainParticleType)
{
    auto forBattleScene = mScene->GetName() == game_constants::BATTLE_SCENE;
    auto& particleManager = CoreSystemsEngine::GetInstance().GetParticleManager();
    
    mScene->RemoveSceneObject(GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME);
    mParticleEmitterTimeAccums[GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME] = 0.0f;
    
    mRewardAnimationSecsLeft = STAT_GAIN_ANIMATION_DURATION_SECS;
    auto particleDefinition = strutils::StringId();
    switch (statGainParticleType)
    {
        case StatGainParticleType::MAX_HEALTH: particleDefinition = forBattleScene ? PARTICLE_EMITTER_DEFINITION_HEALTH_GAIN_SMALL : PARTICLE_EMITTER_DEFINITION_HEALTH_GAIN_LARGE; break;
        case StatGainParticleType::DAMAGE: particleDefinition = forBattleScene ? PARTICLE_EMITTER_DEFINITION_DAMAGE_GAIN_SMALL : PARTICLE_EMITTER_DEFINITION_DAMAGE_GAIN_LARGE; break;
        case StatGainParticleType::WEIGHT: particleDefinition = forBattleScene ? PARTICLE_EMITTER_DEFINITION_WEIGHT_GAIN_SMALL : PARTICLE_EMITTER_DEFINITION_WEIGHT_GAIN_LARGE; break;
        default: break;
    }
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(STAT_GAIN_ANIMATION_DURATION_SECS * 2), [](){ events::EventSystem::GetInstance().DispatchEvent<events::GuiRewardAnimationFinishedEvent>(); } );
        
    auto particleEmitterSceneObject = particleManager.CreateParticleEmitterAtPosition(particleDefinition, originPosition + (forBattleScene ? STAT_GAIN_BATTLE_PARTICLE_OFFSET_POSITION: STAT_GAIN_PARTICLE_OFFSET_POSITION), *mScene, GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME, [=](float dtMillis, scene::ParticleEmitterObjectData& particleEmitterData)
    {
        auto& particleManager = CoreSystemsEngine::GetInstance().GetParticleManager();
        
        auto targetRespawnSecs = statGainParticleType == StatGainParticleType::MAX_HEALTH ? STAT_GAIN_PARTICLE_RESPAWN_SECS : STAT_GAIN_PARTICLE_RESPAWN_SECS/2;
        auto particleEmitterSceneObject = mScene->FindSceneObject(GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME);

        mParticleEmitterTimeAccums[GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME] += dtMillis/1000.0f;

        mRewardAnimationSecsLeft -= dtMillis/1000.0f;
        
        if (mParticleEmitterTimeAccums[GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME] > targetRespawnSecs && mRewardAnimationSecsLeft > 0.0f)
        {
            mParticleEmitterTimeAccums[GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME] = 0.0f;
            auto newParticleIndex = particleManager.SpawnParticleAtFirstAvailableSlot(*particleEmitterSceneObject);

            particleEmitterData.mParticleLifetimeSecs[newParticleIndex] = math::RandomFloat(0.01f, 0.1f);
            particleEmitterData.mParticleAngles[newParticleIndex] = 1.0f;
        }
        for (auto i = 0U; i < particleEmitterData.mParticleCount; ++i)
        {
            if (particleEmitterData.mParticleLifetimeSecs[i] <= 0.0f)
            {
                continue;
            }
            
            auto particleLifetimeSpeed = statGainParticleType != StatGainParticleType::MAX_HEALTH ? STAT_GAIN_PARTICLE_LIFETIME_SPEED : MAX_HEALTH_STAT_GAIN_PARTICLE_LIFETIME_SPEED;
            particleLifetimeSpeed *= forBattleScene ? 0.5f : 1.0f;
            if (particleEmitterData.mParticleAngles[i] > 0.0f)
            {
                particleEmitterData.mParticleLifetimeSecs[i] += dtMillis * particleLifetimeSpeed;
                if (particleEmitterData.mParticleLifetimeSecs[i] > (statGainParticleType == StatGainParticleType::MAX_HEALTH ? 1.0f : 2.0f))
                {
                    particleEmitterData.mParticleAngles[i] = -1.0f;
                }
            }
            else
            {
                particleEmitterData.mParticleLifetimeSecs[i] = math::Max(0.01f, particleEmitterData.mParticleLifetimeSecs[i] - dtMillis * particleLifetimeSpeed);
            }

            particleEmitterData.mParticlePositions[i] += particleEmitterData.mParticleVelocities[i] * dtMillis;
            if (statGainParticleType != StatGainParticleType::MAX_HEALTH)
            {
                particleEmitterData.mParticlePositions[i].x = math::Sinf(particleEmitterData.mParticleLifetimeSecs[i] * (forBattleScene ? 8.0f : 4.0f))/(forBattleScene ? 8.0f : 4.0f) - 0.05f;
            }
        }
        
        particleManager.SortParticles(particleEmitterData);
    });
    particleEmitterSceneObject->mDeferredRendering = true;
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::OnSettingsButtonPressed()
{
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mScene->GetUpdateTimeSpeedFactor(), 0.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::SETTINGS_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::OnStoryCardsButtonPressed()
{
    DataRepository::GetInstance().SetCurrentCardLibraryBehaviorType(CardLibraryBehaviorType::STORY_CARDS);
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mScene->GetUpdateTimeSpeedFactor(), 0.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::CARD_LIBRARY_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::OnInventoryButtonPressed()
{
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mScene->GetUpdateTimeSpeedFactor(), 0.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::INVENTORY_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::OnCoinReward(const events::CoinRewardEvent& event)
{
    mBattleLootHealthRefillCase = false;
    DataRepository::GetInstance().CurrencyCoins().SetValue(DataRepository::GetInstance().CurrencyCoins().GetValue() + event.mCoinAmount);
    AnimateStatParticlesFlyingToGui(event.mAnimationOriginPosition, StatParticleType::COINS, event.mCoinAmount);
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::OnHealthRefillReward(const events::HealthRefillRewardEvent& event)
{
    mBattleLootHealthRefillCase = event.mBattleLootHealthRefillCase;
    
    if (!mBattleLootHealthRefillCase)
    {
        for (auto sceneObject: mHealthStatContainer->GetSceneObjects())
        {
            sceneObject->mInvisible = false;
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, 0.5f), [=](){});
        }
        
        DataRepository::GetInstance().StoryCurrentHealth().SetValue(DataRepository::GetInstance().StoryCurrentHealth().GetValue() + event.mHealthAmount);
    }

    AnimateStatParticlesFlyingToGui(event.mAnimationOriginPosition, StatParticleType::HEALTH, event.mHealthAmount);
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::OnMaxHealthGainReward(const events::MaxHealthGainRewardEvent& event)
{
    for (auto sceneObject: mHealthStatContainer->GetSceneObjects())
    {
        sceneObject->mInvisible = false;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, 0.5f), [=]()
        {
            DataRepository::GetInstance().StoryCurrentHealth().SetDisplayedValue(DataRepository::GetInstance().StoryCurrentHealth().GetValue());
        });
    }
    DataRepository::GetInstance().SetStoryMaxHealth(DataRepository::GetInstance().GetStoryMaxHealth() + event.mMaxHealthGainAmount);
    DataRepository::GetInstance().StoryCurrentHealth().SetValue(DataRepository::GetInstance().StoryCurrentHealth().GetValue() + event.mMaxHealthGainAmount);
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(MAX_HEALTH_GAIN_SFX);
    
    AnimateStatGainParticles(mHealthStatContainer->GetSceneObjects().front()->mPosition, StatGainParticleType::MAX_HEALTH);
}

///------------------------------------------------------------------------------------------------

void GuiObjectManager::OnRareItemCollected(const events::RareItemCollectedEvent& event)
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    if (event.mRareItemSceneObject)
    {
        // Calculate bezier points for item animation
        auto inventoryIconSceneObject = mScene->FindSceneObject(game_constants::GUI_INVENTORY_BUTTON_SCENE_OBJECT_NAME);
        auto inventoryIconPosition = inventoryIconSceneObject->mPosition;
        glm::vec3 midPosition = glm::vec3(event.mRareItemSceneObject->mPosition + inventoryIconPosition)/2.0f;
        
        if (mScene->GetName() != game_constants::BATTLE_SCENE)
        {
            midPosition.y += math::RandomSign() == 1 ? RARE_ITEM_COLLECTED_ANIMATION_MIN_MAX_OFFSETS.t : RARE_ITEM_COLLECTED_ANIMATION_MIN_MAX_OFFSETS.s ;
        }
        else
        {
            if (CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WHEEL_OF_FORTUNE_SCENE))
            {
                midPosition.y += math::RandomSign() == 1 ? RARE_ITEM_COLLECTED_ANIMATION_MIN_MAX_OFFSETS.t : RARE_ITEM_COLLECTED_ANIMATION_MIN_MAX_OFFSETS.s ;
                inventoryIconPosition.x *= game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR;
            }
            else
            {
                midPosition.y += math::RandomSign() == 1 ? RARE_ITEM_COLLECTED_ANIMATION_MIN_MAX_OFFSETS.t/2 : RARE_ITEM_COLLECTED_ANIMATION_MIN_MAX_OFFSETS.s/2 ;
            }
        }
        
        math::BezierCurve curve({event.mRareItemSceneObject->mPosition, midPosition, inventoryIconPosition});
        
        // Animate collected rare item to inventory icon
        animationManager.StartAnimation(std::make_unique<rendering::BezierCurveAnimation>(event.mRareItemSceneObject->mPosition, curve, game_constants::RARE_ITEM_COLLECTION_ANIMATION_DURATION_SECS), [=]()
        {
            CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(RARE_ITEM_COLLECTED_SFX);
            
            // And pulse card library icon
            auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
            auto originalScale = inventoryIconSceneObject->mScale;
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(inventoryIconSceneObject, inventoryIconSceneObject->mPosition, originalScale * RARE_ITEM_COLLECTED_ANIMATION_LIBRARY_ICON_PULSE_FACTOR, RARE_ITEM_COLLECTED_ANIMATION_LIBRARY_ICON_PULSE_DURATION_SECS, animation_flags::IGNORE_X_COMPONENT | animation_flags::IGNORE_Y_COMPONENT | animation_flags::IGNORE_Z_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
            {
                auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
                animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(inventoryIconSceneObject, inventoryIconSceneObject->mPosition, originalScale, RARE_ITEM_COLLECTED_ANIMATION_LIBRARY_ICON_PULSE_DURATION_SECS, animation_flags::IGNORE_X_COMPONENT | animation_flags::IGNORE_Y_COMPONENT | animation_flags::IGNORE_Z_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
                {
                    inventoryIconSceneObject->mScale = originalScale;
                });
            });
            
            if (mScene->GetName() == game_constants::BATTLE_SCENE && !CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WHEEL_OF_FORTUNE_SCENE))
            {
                return;
            }
            
            // Handle animation/music for rare item
            if (event.mRareItemProductId == artifacts::BLOOD_DIAMOND)
            {
                CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(MAX_HEALTH_GAIN_SFX);
                AnimateStatGainParticles(EXTRA_DAMAGE_WEIGHT_PARTICLE_ORIGIN_POSITION, StatGainParticleType::DAMAGE);
            }
            else if (event.mRareItemProductId == artifacts::BLUE_SAPPHIRE)
            {
                CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(MAX_HEALTH_GAIN_SFX);
                AnimateStatGainParticles(EXTRA_DAMAGE_WEIGHT_PARTICLE_ORIGIN_POSITION, StatGainParticleType::WEIGHT);
            }
            else
            {
                CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(MAX_HEALTH_GAIN_SFX);
                
                auto rareItemTexturePath = std::get<std::string>(ProductRepository::GetInstance().GetProductDefinition(event.mRareItemProductId).mProductTexturePathOrCardId);
                auto rareItemTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + rareItemTexturePath);
                auto particleDefinition = PARTICLE_EMITTER_DEFINITION_GENERIC_RARE_ITEM_LARGE;
                
                CoreSystemsEngine::GetInstance().GetParticleManager().ChangeParticleTexture(particleDefinition, rareItemTextureResourceId);
                
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(RARE_ITEM_COLLECTED_ANIMATION_DURATION_SECS), [](){ events::EventSystem::GetInstance().DispatchEvent<events::GuiRewardAnimationFinishedEvent>(); } );
                
                // Rare items can only be collected in wheel, event, or shop scenes. If the base scene here is battle
                // then we need to be looking at wheel at the top level and we spawn the particles there directly.
                // This is necessary due to how the shader works for generic rare item particles.
                auto sceneToSpawnParticlesIn = mScene->GetName() == game_constants::BATTLE_SCENE ? CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WHEEL_OF_FORTUNE_SCENE) : mScene;
                
                if (sceneToSpawnParticlesIn)
                {
                    mScene->RemoveSceneObject(GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME);
                    auto particleEmitterSceneObject = CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition(particleDefinition, GENERIC_RARE_ITEM_PARTICLE_ORIGIN_POSITION, *sceneToSpawnParticlesIn, GENERIC_PARTICLE_EMITTER_SCENE_OBJECT_NAME, [=](float dtMillis, scene::ParticleEmitterObjectData& particleEmitterData){});
                }
            }
            
            animationManager.StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(STAT_GAIN_ANIMATION_DURATION_SECS), [=]()
            {
                if (DataRepository::GetInstance().GetStoryArtifactCount(event.mRareItemProductId) == 3)
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::STACK_ARTIFACT_THRICE);
                }
            });
        });
        
        // And its alpha
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(event.mRareItemSceneObject, RARE_ITEM_COLLECTED_ANIMATION_MIN_ALPHA, game_constants::RARE_ITEM_COLLECTION_ANIMATION_DURATION_SECS), [=](){ event.mRareItemSceneObject->mInvisible = true; });
        
        if (mScene->GetName() != game_constants::BATTLE_SCENE)
        {
            // And its scale for non battle scenes
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(event.mRareItemSceneObject, glm::vec3(), inventoryIconSceneObject->mScale * 2.0f, game_constants::RARE_ITEM_COLLECTION_ANIMATION_DURATION_SECS, animation_flags::IGNORE_X_COMPONENT | animation_flags::IGNORE_Y_COMPONENT | animation_flags::IGNORE_Z_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
        }
    }
    
    // Handle data updates for rare item
    DataRepository::GetInstance().AddStoryArtifact(event.mRareItemProductId);
    
    if (event.mRareItemProductId == artifacts::BLOOD_DIAMOND)
    {
        auto existingDamageModifierIter = DataRepository::GetInstance().GetStoryPlayerCardStatModifiers().find(CardStatType::DAMAGE);
        auto modifierValue = existingDamageModifierIter == DataRepository::GetInstance().GetStoryPlayerCardStatModifiers().cend() ? 2 : existingDamageModifierIter->second + 2;
        
        DataRepository::GetInstance().SetStoryPlayerCardStatModifier(CardStatType::DAMAGE, modifierValue);
    }
    else if (event.mRareItemProductId == artifacts::MASSIVE_MEAL)
    {
        auto existingDamageModifierIter = DataRepository::GetInstance().GetStoryPlayerCardStatModifiers().find(CardStatType::DAMAGE);
        auto modifierValue = existingDamageModifierIter == DataRepository::GetInstance().GetStoryPlayerCardStatModifiers().cend() ? 1 : existingDamageModifierIter->second + 1;
        
        DataRepository::GetInstance().SetStoryPlayerCardStatModifier(CardStatType::DAMAGE, modifierValue);
        
        auto existingWeightModifierIter = DataRepository::GetInstance().GetStoryPlayerCardStatModifiers().find(CardStatType::WEIGHT);
        modifierValue = existingWeightModifierIter == DataRepository::GetInstance().GetStoryPlayerCardStatModifiers().cend() ? 1 : existingWeightModifierIter->second + 1;
        
        DataRepository::GetInstance().SetStoryPlayerCardStatModifier(CardStatType::WEIGHT, modifierValue);
    }
    else if (event.mRareItemProductId == artifacts::BLUE_SAPPHIRE)
    {
        DataRepository::GetInstance().SetNextBattleBotPlayerInitWeight(DataRepository::GetInstance().GetNextBattleBotPlayerInitWeight() + 2);
    }
}

///------------------------------------------------------------------------------------------------
