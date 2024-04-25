///------------------------------------------------------------------------------------------------
///  SettingsSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 14/12/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/SceneObjectUtils.h>
#include <game/AnimatedButton.h>
#include <game/ArtifactProductIds.h>
#include <game/DataRepository.h>
#include <game/GuiObjectManager.h>
#include <game/events/EventSystem.h>
#include <game/GameSceneTransitionManager.h>
#include <game/ProductRepository.h>
#include <game/scenelogicmanagers/WheelOfFortuneSceneLogicManager.h>
#include <game/TutorialManager.h>
#include <game/WheelOfFortuneController.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId WHEEL_OF_FORTUNE_SCENE_NAME = strutils::StringId("wheel_of_fortune_scene");
static const strutils::StringId SPIN_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("spin_button");
static const strutils::StringId CONTINUE_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("continue_button");
static const strutils::StringId WHEEL_OF_FORTUNE_TITLE_SCENE_OBJECT_NAME = strutils::StringId("wheel_of_fortune_title");

static const std::string COIN_VALUE_TEXT_SHADER_FILE_NAME = "animated_stat_container_value_object.vs";
static const std::string REWARD_TEXT_SCENE_OBJECT_NAME_PREFIX = "reward_text_";

static const glm::vec3 BUTTON_POSITION = {0.103f, -0.178f, 23.1f};
static const glm::vec3 BUTTON_SCALE = {0.0005f, 0.0005f, 0.0005f};
static const glm::vec3 REWARD_ORIGIN_POSITION = {-0.032f, -0.034f, 23.1f};
static const glm::vec3 MINI_BOSS_TITLE_COLOR = {0.9f, 0.27f, 0.125f};
static const glm::vec3 FINAL_BOSS_TITLE_COLOR = {0.86f, 0.1f, 0.1f};
static const glm::vec3 COIN_VALUE_TEXT_COLOR = {0.80f, 0.71f, 0.11f};
static const glm::vec3 REWARD_TEXT_SCALE = {0.00032f, 0.00032f, 0.00032f};

static constexpr int EXTRA_HP_REWARD_VALUE = 10;
static constexpr int REWARD_COUNT = 12;

static const float FADE_IN_OUT_DURATION_SECS = 1.0f;
static const float REWARD_TEXT_STAGGERED_FADE_IN_SECS = 0.1f;

static const strutils::StringId REWARD_EXTRA_15_COINS_PRODUCT_NAME = strutils::StringId("extra_15_coins");
static const strutils::StringId REWARD_EXTRA_50_COINS_PRODUCT_NAME = strutils::StringId("extra_50_coins");
static const strutils::StringId REWARD_EXTRA_100_COINS_PRODUCT_NAME = strutils::StringId("extra_100_coins");
static const strutils::StringId REWARD_EXTRA_HP_PRODUCT_NAME = strutils::StringId("extra_hp");
static const strutils::StringId REWARD_REFILL_HP_PRODUCT_NAME = strutils::StringId("story_health_refill");
static const strutils::StringId REWARD_NORMAL_PACK_NAME = strutils::StringId("normal_card_pack");
static const strutils::StringId REWARD_GOLDEN_PACK_NAME = strutils::StringId("golden_card_pack");

static const glm::vec3 REWARD_TEXT_OFFSETS[4] =
{
    {0.138f, 0.00f, 23.2f},
    {0.15f, -0.044f, 23.2f},
    {0.15f, -0.088f, 23.2f},
    {0.138f, -0.132f, 23.2f}
};

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    WHEEL_OF_FORTUNE_SCENE_NAME
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    WHEEL_OF_FORTUNE_TITLE_SCENE_OBJECT_NAME,
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& WheelOfFortuneSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

WheelOfFortuneSceneLogicManager::WheelOfFortuneSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

WheelOfFortuneSceneLogicManager::~WheelOfFortuneSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void WheelOfFortuneSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void WheelOfFortuneSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::BATTLE_WHEEL_REWARD_TUTORIAL);
    mScene = scene;
    mWheelRewards.clear();
    mWheelRewards.resize(REWARD_COUNT);
    
    if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty())
    {
        DataRepository::GetInstance().SetCurrentBattleSubSceneType(BattleSubSceneType::WHEEL);
        DataRepository::GetInstance().SetCurrentStoryMapNodeSeed(math::GetControlSeed());
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    const auto wheelType = DataRepository::GetInstance().GetCurrentWheelOfFortuneType();
    auto rareItemProductNames = ProductRepository::GetInstance().GetRareItemProductNames();
    for (auto iter = rareItemProductNames.begin(); iter != rareItemProductNames.end();)
    {
        if (ProductRepository::GetInstance().GetProductDefinition(*iter).mUnique &&
            DataRepository::GetInstance().GetStoryArtifactCount(*iter) > 0)
        {
            iter = rareItemProductNames.erase(iter);
        }
        else
        {
            iter++;
        }
    }
    
    auto titleSceneObject = scene->FindSceneObject(WHEEL_OF_FORTUNE_TITLE_SCENE_OBJECT_NAME);
    
    switch (wheelType)
    {
        case WheelOfFortuneType::ELITE:
        {
            auto rareItemsCount = DataRepository::GetInstance().GetStoryArtifactCount(artifacts::GREEDY_GOBLIN);
            rareItemsCount = rareItemsCount == 0 ? 2 : rareItemsCount * 2 * 2;
            rareItemsCount = math::Min(REWARD_COUNT, rareItemsCount);
            
            std::unordered_set<strutils::StringId, strutils::StringIdHasher> rareItemSelection;
            while (rareItemSelection.size() < rareItemsCount && rareItemSelection.size() < rareItemProductNames.size())
            {
                rareItemSelection.insert(rareItemProductNames[math::ControlledRandomInt() % rareItemProductNames.size()]);
            }
            
            mWheelRewards =
            {
                REWARD_REFILL_HP_PRODUCT_NAME,
                REWARD_EXTRA_HP_PRODUCT_NAME,
                REWARD_EXTRA_50_COINS_PRODUCT_NAME,
                REWARD_EXTRA_HP_PRODUCT_NAME,
                REWARD_REFILL_HP_PRODUCT_NAME,
                REWARD_EXTRA_HP_PRODUCT_NAME,
                REWARD_EXTRA_100_COINS_PRODUCT_NAME,
                REWARD_EXTRA_HP_PRODUCT_NAME,
                REWARD_EXTRA_15_COINS_PRODUCT_NAME,
                REWARD_REFILL_HP_PRODUCT_NAME,
                REWARD_EXTRA_HP_PRODUCT_NAME,
                REWARD_EXTRA_50_COINS_PRODUCT_NAME
            };
            
            const auto step = REWARD_COUNT/rareItemSelection.size();
            auto index = 0;
            auto initIndex = 4;
            for (auto& rareItemName: rareItemSelection)
            {
                mWheelRewards[(initIndex + (index * step)) % REWARD_COUNT] = rareItemName;
                index++;
            }
        } break;
            
        case WheelOfFortuneType::TUTORIAL_BOSS:
        {
            std::get<scene::TextSceneObjectData>(titleSceneObject->mSceneObjectTypeData).mText = "Mini Boss Wheel";
            titleSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = MINI_BOSS_TITLE_COLOR;
            
            auto rareItemCount = rareItemProductNames.size();
            if (rareItemCount > REWARD_COUNT)
            {
                for (int i = 0; i < REWARD_COUNT; ++i)
                {
                    auto nextItem = rareItemProductNames[math::ControlledRandomInt() % rareItemCount];
                    while (std::find(mWheelRewards.begin(), mWheelRewards.end(), nextItem) != mWheelRewards.end())
                    {
                        nextItem = rareItemProductNames[math::ControlledRandomInt() % rareItemCount];
                    }
                    mWheelRewards.push_back(nextItem);
                }
            }
            else
            {
                for (int i = 0; i < REWARD_COUNT; ++i)
                {
                    mWheelRewards[i] = rareItemProductNames[i % rareItemCount];
                }
            }
        } break;
            
        case WheelOfFortuneType::FINAL_BOSS:
        {
            std::get<scene::TextSceneObjectData>(titleSceneObject->mSceneObjectTypeData).mText = "Boss Wheel";
            titleSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = FINAL_BOSS_TITLE_COLOR;
            
            if (DataRepository::GetInstance().DoesCurrentStoryHaveMutation(game_constants::MUTATION_FINAL_BOSS_REVIVES))
            {
                mWheelRewards =
                {
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME
                };
            }
            else
            {
                mWheelRewards =
                {
                    REWARD_NORMAL_PACK_NAME,
                    REWARD_NORMAL_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_NORMAL_PACK_NAME,
                    REWARD_NORMAL_PACK_NAME,
                    REWARD_NORMAL_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_NORMAL_PACK_NAME,
                    REWARD_NORMAL_PACK_NAME,
                    REWARD_NORMAL_PACK_NAME,
                    REWARD_GOLDEN_PACK_NAME,
                    REWARD_NORMAL_PACK_NAME
                };
            }
        } break;
    }
    
    auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*titleSceneObject);
    auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
    titleSceneObject->mPosition.x -= textLength/2.0f;
    
    mFinalBossFlow = DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::NORMAL_MAP && DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::STORY_MAP_BOSS_COORD;
    
    mWheelController = std::make_unique<WheelOfFortuneController>(*scene, mWheelRewards, [=](const int itemIndex, const std::shared_ptr<scene::SceneObject> itemSceneObject){ OnWheelItemSelected(itemIndex, itemSceneObject); });
        
    mContinueButton = nullptr;
    mSpinButton = std::make_unique<AnimatedButton>
    (
        BUTTON_POSITION,
        BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        "Spin!",
        SPIN_BUTTON_SCENE_OBJECT_NAME,
        [=]()
        {
            if (!mHasSpinnedWheel)
            {
                mWheelController->Spin();
                mHasSpinnedWheel = true;
                
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mSpinButton->GetSceneObject(), 0.0f, FADE_IN_OUT_DURATION_SECS, animation_flags::NONE), [=]()
                {
                    mSpinButton->GetSceneObject()->mInvisible = true;
                });
            }
        },
        *scene
    );
    
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        sceneObject->mInvisible = false;
        
        if (!STATIC_SCENE_ELEMENTS.count(sceneObject->mName))
        {
            sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        }
        
        sceneObject->mInvisible = false;
        sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, FADE_IN_OUT_DURATION_SECS, animation_flags::NONE), [=](){});
    }
    
    mHasSpinnedWheel = false;
}

///------------------------------------------------------------------------------------------------

void WheelOfFortuneSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene>)
{
    mWheelController->Update(dtMillis);
    
    if (!mHasSpinnedWheel)
    {
        mSpinButton->Update(dtMillis);
    }
    
    if (mContinueButton)
    {
        mContinueButton->Update(dtMillis);
    }
    
    auto guiObjectManager = mGameSceneTransitionManager->GetSceneLogicManagerResponsibleForScene(mPreviousScene)->VGetGuiObjectManager();
    if (guiObjectManager)
    {
        guiObjectManager->Update(dtMillis, false);
    }
}

///------------------------------------------------------------------------------------------------

void WheelOfFortuneSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, FADE_IN_OUT_DURATION_SECS), [=]()
        {
            sceneObject->mInvisible = true;
        });
    }
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> WheelOfFortuneSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void WheelOfFortuneSceneLogicManager::OnWheelItemSelected(const int itemIndex, const std::shared_ptr<scene::SceneObject> selectedSceneObject)
{
    if (mWheelRewards.at(itemIndex) == REWARD_EXTRA_15_COINS_PRODUCT_NAME)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::CoinRewardEvent>(15, REWARD_ORIGIN_POSITION);
    }
    else if (mWheelRewards.at(itemIndex) == REWARD_EXTRA_50_COINS_PRODUCT_NAME)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::CoinRewardEvent>(50, REWARD_ORIGIN_POSITION);
    }
    else if (mWheelRewards.at(itemIndex) == REWARD_EXTRA_100_COINS_PRODUCT_NAME)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::CoinRewardEvent>(100, REWARD_ORIGIN_POSITION);
    }
    else if (mWheelRewards.at(itemIndex) == REWARD_EXTRA_HP_PRODUCT_NAME)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::MaxHealthGainRewardEvent>(EXTRA_HP_REWARD_VALUE);
    }
    else if (mWheelRewards.at(itemIndex) == REWARD_REFILL_HP_PRODUCT_NAME)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::HealthRefillRewardEvent>(DataRepository::GetInstance().GetStoryMaxHealth() - DataRepository::GetInstance().StoryCurrentHealth().GetValue(), REWARD_ORIGIN_POSITION);
    }
    else if (mWheelRewards.at(itemIndex) == REWARD_NORMAL_PACK_NAME)
    {
        DataRepository::GetInstance().AddPendingCardPack(CardPackType::NORMAL);
    }
    else if (mWheelRewards.at(itemIndex) == REWARD_GOLDEN_PACK_NAME)
    {
        DataRepository::GetInstance().AddPendingCardPack(CardPackType::GOLDEN);
    }
    else
    {
        events::EventSystem::GetInstance().DispatchEvent<events::RareItemCollectedEvent>(mWheelRewards.at(itemIndex), selectedSceneObject);
    }
    
    if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty())
    {
        if (mFinalBossFlow)
        {
            DataRepository::GetInstance().SetCurrentBattleSubSceneType(BattleSubSceneType::STORY_VICTORY);
        }
        else
        {
            DataRepository::GetInstance().SetCurrentBattleSubSceneType(BattleSubSceneType::CARD_SELECTION);
            DataRepository::GetInstance().SetCurrentStoryMapNodeSeed(math::GetControlSeed());
        }
        
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(game_constants::RARE_ITEM_COLLECTION_ANIMATION_DURATION_SECS), [=]()
    {
        mContinueButton = std::make_unique<AnimatedButton>
        (
            BUTTON_POSITION,
            BUTTON_SCALE,
            game_constants::DEFAULT_FONT_NAME,
            "Continue",
            CONTINUE_BUTTON_SCENE_OBJECT_NAME,
            [=]()
            {
                if (!mFinalBossFlow)
                {
                    auto guiObjectManager = mGameSceneTransitionManager->GetSceneLogicManagerResponsibleForScene(mPreviousScene)->VGetGuiObjectManager();
                    guiObjectManager->StopRewardAnimation();
                    guiObjectManager->ResetDisplayedCurrencyCoins();
                    DataRepository::GetInstance().StoryCurrentHealth().SetDisplayedValue(DataRepository::GetInstance().StoryCurrentHealth().GetValue());
                    guiObjectManager->ForceSetStoryHealthValue(DataRepository::GetInstance().StoryCurrentHealth().GetValue());
                }
                
                events::EventSystem::GetInstance().DispatchEvent<events::PopSceneModalEvent>();
            },
            *mScene
        );
    });
    
    auto productDescription = ProductRepository::GetInstance().GetProductDefinition(mWheelRewards.at(itemIndex)).mDescription;
    auto tooltipTextRows = strutils::StringSplit(productDescription, '$');
    
    for (auto i = 0U; i < tooltipTextRows.size(); ++i)
    {
        auto tooltipTextSceneObject = mScene->CreateSceneObject(strutils::StringId(REWARD_TEXT_SCENE_OBJECT_NAME_PREFIX + std::to_string(i)));
        tooltipTextSceneObject->mScale = REWARD_TEXT_SCALE;
        tooltipTextSceneObject->mPosition += REWARD_TEXT_OFFSETS[i];
        tooltipTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        tooltipTextSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + COIN_VALUE_TEXT_SHADER_FILE_NAME);
        tooltipTextSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = COIN_VALUE_TEXT_COLOR;
        
        scene::TextSceneObjectData textData;
        textData.mFontName = game_constants::DEFAULT_FONT_NAME;
        textData.mText = tooltipTextRows[i];
        tooltipTextSceneObject->mSceneObjectTypeData = std::move(textData);
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(tooltipTextSceneObject, 1.0f, FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, i * REWARD_TEXT_STAGGERED_FADE_IN_SECS), [=](){});
    }
}

///------------------------------------------------------------------------------------------------
