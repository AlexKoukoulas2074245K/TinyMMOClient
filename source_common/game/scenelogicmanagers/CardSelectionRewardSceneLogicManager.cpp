///------------------------------------------------------------------------------------------------
///  CardSelectionRewardSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 14/12/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/sound/SoundManager.h>
#include <game/AnimatedButton.h>
#include <game/CardUtils.h>
#include <game/CardTooltipController.h>
#include <game/GuiObjectManager.h>
#include <game/events/EventSystem.h>
#include <game/GameSceneTransitionManager.h>
#include <game/DataRepository.h>
#include <game/scenelogicmanagers/CardSelectionRewardSceneLogicManager.h>
#include <game/TutorialManager.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId CARD_SELECTION_REWARD_SCENE_NAME = strutils::StringId("card_selection_reward_scene");
static const strutils::StringId REWARD_HIGHLIGHTER_SCENE_OBJECT_NAME = strutils::StringId("reward_highlighter");
static const strutils::StringId SKIP_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("skip_button");
static const strutils::StringId CONFIRMATION_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("confirmation_button");
static const strutils::StringId CARD_SELECTION_TITLE_SCENE_OBJECT_NAME = strutils::StringId("card_selection_title");
static const strutils::StringId DARKEN_UNIFORM_NAME = strutils::StringId("darken");
static const strutils::StringId CARD_SELECTION_ANIMATION_NAME = strutils::StringId("card_selection_animation");

static const std::string CARD_REWARD_SCENE_OBJECT_NAME_PREFIX = "card_reward_";
static const std::string CARD_REWARD_SHADER_FILE_NAME = "card_reward.vs";
static const std::string CARD_COLLECTED_SFX = "sfx_collected";
static const std::string CARD_SWIPE_SFX = "sfx_swipe";

static const glm::vec3 CONFIRMATION_BUTTON_POSITION = {-0.10f, -0.18f, 23.1f};
static const glm::vec3 SKIP_BUTTON_SCALE = {0.00035f, 0.00035f, 0.00035f};
static const glm::vec3 BUTTON_SCALE = {0.0005f, 0.0005f, 0.0005f};
static const glm::vec3 CARD_REWARD_DEFAULT_SCALE = glm::vec3(-0.273f, 0.2512f, 2.0f);
static const glm::vec3 CARD_HIGHLIGHTER_SCALE = glm::vec3(0.08f, 0.13f, 1.0f) * 2.35f;
static const glm::vec3 CARD_REWARD_EXPANDED_SCALE = 1.25f * CARD_REWARD_DEFAULT_SCALE;
static const glm::vec3 CARD_TOOLTIP_POSITION_OFFSET = {0.0f, 0.1f, 0.0f};
static const glm::vec3 CARD_TOOLTIP_BASE_SCALE = {0.3f, 0.274f, 1/10.0f};
static const glm::vec3 SKIP_BUTTON_POSITION = {0.0f, -0.186f, 23.1f};

static const glm::vec2 CARD_BOUGHT_ANIMATION_MIN_MAX_OFFSETS = {-0.15f, 0.15f};

static const float CARD_BOUGHT_ANIMATION_DURATION_SECS = 1.0f;
static const float CARD_BOUGHT_ANIMATION_MIN_ALPHA = 0.3f;
static const float CARD_BOUGHT_ANIMATION_LIBRARY_ICON_PULSE_FACTOR = 1.25f;
static const float CARD_BOUGHT_ANIMATION_LIBRARY_ICON_PULSE_DURATION_SECS = 0.1f;
static const float FADE_IN_OUT_DURATION_SECS = 0.5f;
static const float INITIAL_SURFACING_DELAY_SECS = 1.0f;
static const float CARD_HIGHLIGHTER_X_OFFSET = -0.003f;
static const float CARD_HIGHLIGHT_ANIMATION_DURATION_SECS = 0.5f;
static const float CARD_REWARD_SURFACE_DELAY_SECS = 0.5f;
static const float SKIP_BUTTON_SNAP_TO_EDGE_FACTOR = 1850000.0f;
static const float SKIP_BUTTON_MIN_ALPHA = 0.3f;
static const float SUSPENDED_FOR_GUI_FLOW_Z_REDUCTION = 2.0f;
static const float SELECTED_CARD_FLYING_Z = 24.0f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    CARD_SELECTION_REWARD_SCENE_NAME
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    CARD_SELECTION_TITLE_SCENE_OBJECT_NAME,
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& CardSelectionRewardSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

CardSelectionRewardSceneLogicManager::CardSelectionRewardSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

CardSelectionRewardSceneLogicManager::~CardSelectionRewardSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mCardRewards.clear();
    mCardTooltipController = nullptr;
    mSceneState = SceneState::PENDING_PRESENTATION;
    mInitialSurfacingDelaySecs = INITIAL_SURFACING_DELAY_SECS;
    mGoldenCardLightPosX = game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.s;
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(CARD_COLLECTED_SFX);
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(CARD_SWIPE_SFX);
    
    mSkipButton = std::make_unique<AnimatedButton>
    (
        SKIP_BUTTON_POSITION,
        SKIP_BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        "Skip Rewards",
        SKIP_BUTTON_SCENE_OBJECT_NAME,
        [=](){ OnLeavingCardSelection(); },
        *scene,
        scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE,
        SKIP_BUTTON_SNAP_TO_EDGE_FACTOR
    );
    
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        sceneObject->mInvisible = true;
        sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    }

    RegisterForEvents();
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    static float time = 0.0f;
    time += dtMillis * 0.001f;
    
    for (auto& cardReward: mCardRewards)
    {
        cardReward->mSceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
        cardReward->mSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME] = mGoldenCardLightPosX;
    }
    
    switch (mSceneState)
    {
        case SceneState::PENDING_PRESENTATION:
        {
            mInitialSurfacingDelaySecs -= dtMillis/1000.0f;
            if (mInitialSurfacingDelaySecs <= 0.0f)
            {
                CoreSystemsEngine::GetInstance().GetSceneManager().RemoveScene(game_constants::WHEEL_OF_FORTUNE_SCENE);
                
                if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty())
                {
                    DataRepository::GetInstance().SetCurrentBattleSubSceneType(BattleSubSceneType::CARD_SELECTION);
                    DataRepository::GetInstance().SetCurrentStoryMapNodeSeed(math::GetControlSeed());
                    DataRepository::GetInstance().FlushStateToFile();
                }
                
                auto guiObjectManager = mGameSceneTransitionManager->GetSceneLogicManagerResponsibleForScene(mPreviousScene)->VGetGuiObjectManager();
                if (guiObjectManager)
                {
                    guiObjectManager->ResetDisplayedCurrencyCoins();
                    DataRepository::GetInstance().StoryCurrentHealth().SetDisplayedValue(DataRepository::GetInstance().StoryCurrentHealth().GetValue());
                    guiObjectManager->ForceSetStoryHealthValue(DataRepository::GetInstance().StoryCurrentHealth().GetValue());
                }
                
                for (auto sceneObject: scene->GetSceneObjects())
                {
                    if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
                    {
                        continue;
                    }
                    
                    sceneObject->mInvisible = false;
                    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, FADE_IN_OUT_DURATION_SECS, animation_flags::NONE), [=](){});
                }
                
                CreateCardRewards(scene);
                mSceneState = SceneState::PENDING_CARD_SELECTION;
            }
        } break;
            
        case SceneState::PENDING_CARD_SELECTION:
        {
            auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
            auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
            
            auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(scene->GetCamera().GetViewMatrix(), scene->GetCamera().GetProjMatrix());
            
            for (auto i = 0U; i < mCardRewards.size(); ++i)
            {
                auto cardSoWrapper = mCardRewards[i];
                auto sceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*cardSoWrapper->mSceneObject);
                
                bool cursorInSceneObject = math::IsPointInsideRectangle(sceneObjectRect.bottomLeft, sceneObjectRect.topRight, worldTouchPos);
                if (cursorInSceneObject && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON))
                {
                    CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(CARD_SWIPE_SFX);
                    
                    if (cardSoWrapper->mState == CardSoState::IDLE)
                    {
                        cardSoWrapper->mState = CardSoState::HIGHLIGHTED;
                        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardSoWrapper->mSceneObject, cardSoWrapper->mSceneObject->mPosition, CARD_REWARD_EXPANDED_SCALE, 0.5f, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=](){}, CARD_SELECTION_ANIMATION_NAME);
                    }
                    
                    for (auto j = 0U; j < mCardRewards.size(); ++j)
                    {
                        if (j == i)
                        {
                            continue;
                        }
                        
                        mCardRewards[j]->mSceneObject->mShaderBoolUniformValues[DARKEN_UNIFORM_NAME] = true;
                    }
                    
                    auto cardHighlighterSo = scene->CreateSceneObject(REWARD_HIGHLIGHTER_SCENE_OBJECT_NAME);
                    cardHighlighterSo->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::ACTION_HIGHLIGHTER_SHADER_NAME);
                    cardHighlighterSo->mShaderFloatUniformValues[game_constants::PERLIN_TIME_SPEED_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_TIME_SPEED;
                    cardHighlighterSo->mShaderFloatUniformValues[game_constants::PERLIN_RESOLUTION_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_RESOLUTION;
                    cardHighlighterSo->mShaderFloatUniformValues[game_constants::PERLIN_CLARITY_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_CLARITY;
                    cardHighlighterSo->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
                    cardHighlighterSo->mShaderBoolUniformValues[game_constants::CARD_HIGHLIGHTER_INVALID_ACTION_UNIFORM_NAME] = false;
                    cardHighlighterSo->mPosition = cardSoWrapper->mSceneObject->mPosition;
                    cardHighlighterSo->mPosition.x += CARD_HIGHLIGHTER_X_OFFSET;
                    cardHighlighterSo->mPosition.z += game_constants::ACTION_HIGLIGHTER_Z_OFFSET;
                    cardHighlighterSo->mScale = CARD_HIGHLIGHTER_SCALE;
                    
                    mConfirmationButton = std::make_unique<AnimatedButton>
                    (
                        CONFIRMATION_BUTTON_POSITION,
                        BUTTON_SCALE,
                        game_constants::DEFAULT_FONT_NAME,
                        "Confirm",
                        CONFIRMATION_BUTTON_SCENE_OBJECT_NAME,
                        [=]()
                        {
                            DestroyCardTooltip(scene);
                            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(cardHighlighterSo, 0.0f, 0.25f), [=](){ cardHighlighterSo->mInvisible = true; });
                            OnConfirmationButtonPressed();
                        },
                        *scene
                    );
                    mConfirmationButton->GetSceneObject()->mBoundingRectMultiplier.y *= 1.5f;
                    
                    if (cardSoWrapper->mCardData.IsSpell())
                    {
                        CreateCardTooltip(cardSoWrapper->mSceneObject->mPosition, cardSoWrapper->mCardData.mCardEffectTooltip, i, scene);
                    }
                    
                    mSceneState = SceneState::PENDING_CARD_SELECTION_CONFIRMATION;
                }
                
#if !defined(MOBILE_FLOW)
                if (cursorInSceneObject && cardSoWrapper->mState == CardSoState::IDLE)
                {
                    cardSoWrapper->mState = CardSoState::HIGHLIGHTED;
                    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardSoWrapper->mSceneObject, cardSoWrapper->mSceneObject->mPosition, CARD_REWARD_EXPANDED_SCALE, CARD_HIGHLIGHT_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=](){}, CARD_SELECTION_ANIMATION_NAME);
                }
                else if (!cursorInSceneObject && cardSoWrapper->mState == CardSoState::HIGHLIGHTED)
                {
                    cardSoWrapper->mState = CardSoState::IDLE;
                    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardSoWrapper->mSceneObject, cardSoWrapper->mSceneObject->mPosition, CARD_REWARD_DEFAULT_SCALE, CARD_HIGHLIGHT_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=](){}, CARD_SELECTION_ANIMATION_NAME);
                }
#endif
            }
            
            mSkipButton->GetSceneObject()->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
            mSkipButton->Update(dtMillis);
        } break;
            
        case SceneState::PENDING_CARD_SELECTION_CONFIRMATION:
        {
            mSkipButton->GetSceneObject()->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = SKIP_BUTTON_MIN_ALPHA;
            mConfirmationButton->Update(dtMillis);
            
            if (mCardTooltipController)
            {
                mCardTooltipController->Update(dtMillis);
            }
            
            auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
            auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(scene->GetCamera().GetViewMatrix(), scene->GetCamera().GetProjMatrix());
            
            auto sceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*mConfirmationButton->GetSceneObject());
            bool cursorInConfirmationButton = math::IsPointInsideRectangle(sceneObjectRect.bottomLeft, sceneObjectRect.topRight, worldTouchPos);
            
            if (!cursorInConfirmationButton && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON))
            {
                // Confirmation button was not pressed at this point
                for (auto i = 0U; i < mCardRewards.size(); ++i)
                {
                    mCardRewards[i]->mSceneObject->mShaderBoolUniformValues[DARKEN_UNIFORM_NAME] = false;
                    
#if defined(MOBILE_FLOW)
                    mCardRewards[i]->mState = CardSoState::IDLE;
                    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(mCardRewards[i]->mSceneObject, mCardRewards[i]->mSceneObject->mPosition, CARD_REWARD_DEFAULT_SCALE, CARD_HIGHLIGHT_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=](){}, CARD_SELECTION_ANIMATION_NAME);
#endif
                }
                
                DestroyCardTooltip(scene);
                scene->RemoveSceneObject(mConfirmationButton->GetSceneObject()->mName);
                mConfirmationButton = nullptr;
                scene->RemoveSceneObject(REWARD_HIGHLIGHTER_SCENE_OBJECT_NAME);
                mSceneState = SceneState::PENDING_CARD_SELECTION;
            }
        } break;
            
        case SceneState::CARD_SELECTION_CONFIRMATION_ANIMATION:
        {
            return;
        } break;
    }
    
    auto guiObjectManager = mGameSceneTransitionManager->GetSceneLogicManagerResponsibleForScene(mPreviousScene)->VGetGuiObjectManager();
    if (guiObjectManager)
    {
        guiObjectManager->Update(dtMillis, true);
    }
    
    auto cardHighlighterObject = scene->FindSceneObject(strutils::StringId(REWARD_HIGHLIGHTER_SCENE_OBJECT_NAME));
    if (cardHighlighterObject)
    {
        cardHighlighterObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
    }
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    events::EventSystem::GetInstance().UnregisterAllEventsForListener(this);
    DestroyCardTooltip(scene);
    mCardRewards.clear();
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> CardSelectionRewardSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::RegisterForEvents()
{
    auto& eventSystem = events::EventSystem::GetInstance();
    
    eventSystem.RegisterForEvent<events::WindowResizeEvent>(this, &CardSelectionRewardSceneLogicManager::OnWindowResize);
    eventSystem.RegisterForEvent<events::SceneChangeEvent>(this, &CardSelectionRewardSceneLogicManager::OnSceneChange);
    eventSystem.RegisterForEvent<events::PopSceneModalEvent>(this, &CardSelectionRewardSceneLogicManager::OnPopSceneModal);
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::OnWindowResize(const events::WindowResizeEvent&)
{
    CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(CARD_SELECTION_REWARD_SCENE_NAME)->RecalculatePositionOfEdgeSnappingSceneObjects();
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::OnSceneChange(const events::SceneChangeEvent& event)
{
    if (event.mNewSceneName == game_constants::SETTINGS_SCENE || event.mNewSceneName == game_constants::CARD_LIBRARY_SCENE || event.mNewSceneName == game_constants::INVENTORY_SCENE)
    {
        CoreSystemsEngine::GetInstance().GetAnimationManager().StopAnimation(CARD_SELECTION_ANIMATION_NAME);
        auto scene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(CARD_SELECTION_REWARD_SCENE_NAME);
        for (auto sceneObject: scene->GetSceneObjects())
        {
            sceneObject->mPosition.z -= SUSPENDED_FOR_GUI_FLOW_Z_REDUCTION;
        }
    }
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::OnPopSceneModal(const events::PopSceneModalEvent&)
{
    if (CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WHEEL_OF_FORTUNE_SCENE))
    {
        // Coming in from wheel
        return;
    }
    
    auto scene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(CARD_SELECTION_REWARD_SCENE_NAME);
    for (auto sceneObject: scene->GetSceneObjects())
    {
        sceneObject->mPosition.z += SUSPENDED_FOR_GUI_FLOW_Z_REDUCTION;
    }
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::CreateCardRewards(std::shared_ptr<scene::Scene> scene)
{
    auto cardRewardsPool = CardDataRepository::GetInstance().GetStoryUnlockedCardRewardsPool();
    for (size_t i = 0; i < 3; ++i)
    {
        auto randomCardIndex = math::ControlledRandomInt() % cardRewardsPool.size();
        auto cardData = CardDataRepository::GetInstance().GetCardData(cardRewardsPool[randomCardIndex], game_constants::LOCAL_PLAYER_INDEX);
        while (std::find_if(mCardRewards.begin(), mCardRewards.end(), [&](std::shared_ptr<CardSoWrapper> cardReward){ return cardData.mCardId == cardReward->mCardData.mCardId; }) != mCardRewards.end())
        {
            randomCardIndex = math::ControlledRandomInt() % cardRewardsPool.size();
            cardData = CardDataRepository::GetInstance().GetCardData(cardRewardsPool[randomCardIndex], game_constants::LOCAL_PLAYER_INDEX);
        }
        
        const auto& goldenCardIds = DataRepository::GetInstance().GetGoldenCardIdMap();
        bool isGoldenCard = goldenCardIds.count(cardRewardsPool[randomCardIndex]) && goldenCardIds.at(cardRewardsPool[randomCardIndex]);
        
        mCardRewards.push_back(card_utils::CreateCardSoWrapper(&cardData, glm::vec3(-0.2f + 0.17 * i, -0.0f, 23.2f), CARD_REWARD_SCENE_OBJECT_NAME_PREFIX + std::to_string(i), CardOrientation::FRONT_FACE, isGoldenCard ? CardRarity::GOLDEN : CardRarity::NORMAL, true, false, true, {}, {}, *scene));
        mCardRewards.back()->mSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        mCardRewards.back()->mSceneObject->mScale = CARD_REWARD_DEFAULT_SCALE;
        mCardRewards.back()->mSceneObject->mShaderBoolUniformValues[DARKEN_UNIFORM_NAME] = false;
        mCardRewards.back()->mSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_REWARD_SHADER_FILE_NAME);
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mCardRewards.back()->mSceneObject, 1.0f, FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, CARD_REWARD_SURFACE_DELAY_SECS + i * CARD_REWARD_SURFACE_DELAY_SECS), [=]()
        {
            if (i == 2)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::BATTLE_CARD_SELECTION_REWARD_TUTORIAL);
            }
        });
        
        if (cardRewardsPool.size() > 1)
        {
            cardRewardsPool.erase(cardRewardsPool.begin() + randomCardIndex);
        }
    }
    
    // Start a light ray in case of golden cards
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mGoldenCardLightPosX, game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.t, 1.0f, animation_flags::NONE, CARD_REWARD_SURFACE_DELAY_SECS + mCardRewards.size() * CARD_REWARD_SURFACE_DELAY_SECS), [](){});
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::CreateCardTooltip(const glm::vec3& cardOriginPostion, const std::string& tooltipText, const size_t cardIndex, std::shared_ptr<scene::Scene> scene)
{
    bool shouldBeHorFlipped = cardIndex > 1;
    mCardTooltipController = std::make_unique<CardTooltipController>
    (
        cardOriginPostion + CARD_TOOLTIP_POSITION_OFFSET,
        CARD_TOOLTIP_BASE_SCALE,
        tooltipText,
        false,
        shouldBeHorFlipped,
        false,
        *scene
    );
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::DestroyCardTooltip(std::shared_ptr<scene::Scene> scene)
{
    if (mCardTooltipController)
    {
        for (auto sceneObject: mCardTooltipController->GetSceneObjects())
        {
            scene->RemoveSceneObject(sceneObject->mName);
        }
    }
    
    mCardTooltipController = nullptr;
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::OnConfirmationButtonPressed()
{
    for (auto& cardReward: mCardRewards)
    {
        if (cardReward->mState == CardSoState::HIGHLIGHTED)
        {
            auto currentPlayerDeck = DataRepository::GetInstance().GetCurrentStoryPlayerDeck();
            currentPlayerDeck.push_back(cardReward->mCardData.mCardId);
            DataRepository::GetInstance().SetCurrentStoryPlayerDeck(currentPlayerDeck);
            DataRepository::GetInstance().SetCurrentBattleSubSceneType(BattleSubSceneType::BATTLE);
            DataRepository::GetInstance().FlushStateToFile();
            mSceneState = SceneState::CARD_SELECTION_CONFIRMATION_ANIMATION;
            
            auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
            auto previousScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(mPreviousScene);
            
            // Calculate bezier points for card animation
            auto firstPosition = cardReward->mSceneObject->mPosition;
            firstPosition.z = SELECTED_CARD_FLYING_Z;
            
            auto cardLibraryIconSceneObject = previousScene->FindSceneObject(game_constants::GUI_STORY_CARDS_BUTTON_SCENE_OBJECT_NAME);
            auto cardLibraryIconPosition = cardLibraryIconSceneObject->mPosition;
            cardLibraryIconPosition.z = SELECTED_CARD_FLYING_Z;
            
            glm::vec3 midPosition = glm::vec3(cardReward->mSceneObject->mPosition + cardLibraryIconPosition)/2.0f;
            midPosition.y += math::RandomSign() == 1 ? CARD_BOUGHT_ANIMATION_MIN_MAX_OFFSETS.t : CARD_BOUGHT_ANIMATION_MIN_MAX_OFFSETS.s;
            midPosition.z = SELECTED_CARD_FLYING_Z;
            
            if (mPreviousScene == game_constants::BATTLE_SCENE)
            {
                midPosition.x *= game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR;
                midPosition.y *= game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR;
                cardLibraryIconPosition.x *= game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR;
                cardLibraryIconPosition.y *= game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR;
            }
            
            math::BezierCurve curve({firstPosition, midPosition, cardLibraryIconPosition});
            
            // Animate bought card to card library icon
            animationManager.StartAnimation(std::make_unique<rendering::BezierCurveAnimation>(cardReward->mSceneObject, curve, CARD_BOUGHT_ANIMATION_DURATION_SECS), [=](){});
            
            // And its alpha
            animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(cardReward->mSceneObject, CARD_BOUGHT_ANIMATION_MIN_ALPHA, CARD_BOUGHT_ANIMATION_DURATION_SECS), [=](){ cardReward->mSceneObject->mInvisible = true; });
            
            // And its scale
            auto targetScale = cardReward->mSceneObject->mScale/3.0f;
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardReward->mSceneObject, glm::vec3(), targetScale, CARD_BOUGHT_ANIMATION_DURATION_SECS, animation_flags::IGNORE_X_COMPONENT | animation_flags::IGNORE_Y_COMPONENT | animation_flags::IGNORE_Z_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
            {
                
                // And pulse card library icon
                CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(CARD_COLLECTED_SFX);
                
                auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
                auto previousScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(mPreviousScene);
                auto cardLibraryIconSceneObject = previousScene->FindSceneObject(game_constants::GUI_STORY_CARDS_BUTTON_SCENE_OBJECT_NAME);
                auto originalScale = cardLibraryIconSceneObject->mScale;
                animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardLibraryIconSceneObject, cardLibraryIconSceneObject->mPosition, originalScale * CARD_BOUGHT_ANIMATION_LIBRARY_ICON_PULSE_FACTOR, CARD_BOUGHT_ANIMATION_LIBRARY_ICON_PULSE_DURATION_SECS, animation_flags::IGNORE_X_COMPONENT | animation_flags::IGNORE_Y_COMPONENT | animation_flags::IGNORE_Z_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
                {
                    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
                    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardLibraryIconSceneObject, cardLibraryIconSceneObject->mPosition, originalScale, CARD_BOUGHT_ANIMATION_LIBRARY_ICON_PULSE_DURATION_SECS, animation_flags::IGNORE_X_COMPONENT | animation_flags::IGNORE_Y_COMPONENT | animation_flags::IGNORE_Z_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
                    {
                        cardLibraryIconSceneObject->mScale = originalScale;
                        OnLeavingCardSelection();
                    });
                });
            });
            break;
        }
    }
}

///------------------------------------------------------------------------------------------------

void CardSelectionRewardSceneLogicManager::OnLeavingCardSelection()
{
    auto isStoryTutorialBoss = DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::TUTORIAL_MAP && DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::TUTORIAL_MAP_BOSS_COORD;
    
    if (isStoryTutorialBoss)
    {
        DataRepository::GetInstance().SetStoryMapGenerationSeed(0);
        DataRepository::GetInstance().SetCurrentStoryMapType(StoryMapType::NORMAL_MAP);
        DataRepository::GetInstance().SetCurrentStoryMapNodeCoord(game_constants::STORY_MAP_INIT_COORD);
        DataRepository::GetInstance().SetCurrentStoryMapSceneType(StoryMapSceneType::STORY_MAP);
        
        if (!DataRepository::GetInstance().DoesCurrentStoryHaveMutation(game_constants::MUTATION_NO_HEAL_AFTER_FIRST_BOSS))
        {
            DataRepository::GetInstance().StoryCurrentHealth().SetValue(DataRepository::GetInstance().GetStoryMaxHealth());
            DataRepository::GetInstance().StoryCurrentHealth().SetDisplayedValue(DataRepository::GetInstance().GetStoryMaxHealth());
        }
    }
    
    DataRepository::GetInstance().SetCurrentBattleSubSceneType(BattleSubSceneType::BATTLE);
    DataRepository::GetInstance().FlushStateToFile();
    
    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::STORY_MAP_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
}

///------------------------------------------------------------------------------------------------
