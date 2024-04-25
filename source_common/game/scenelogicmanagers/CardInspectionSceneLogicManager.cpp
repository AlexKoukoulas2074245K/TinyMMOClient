///------------------------------------------------------------------------------------------------
///  CardInspectionSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 01/04/2024
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <game/AnimatedButton.h>
#include <game/Cards.h>
#include <game/CardTooltipController.h>
#include <game/CardUtils.h>
#include <game/DataRepository.h>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/CardInspectionSceneLogicManager.h>
#include <SDL_events.h>

///------------------------------------------------------------------------------------------------

static const std::string INSPECTED_CARD_NAME_PREFIX = "inspected_card";

static const strutils::StringId CARD_INSPECTION_SCENE_NAME = strutils::StringId("card_inspection_scene");
static const strutils::StringId CONTINUE_BUTTON_NAME = strutils::StringId("continue_button");

static const glm::vec3 BUTTON_SCALE = {0.0005f, 0.0005f, 0.0005f};
static const glm::vec3 CONTINUE_BUTTON_POSITION = {-0.071f, -0.163f, 23.1f};
static const glm::vec3 INSPECTED_CARD_SCALE = {-0.273f, 0.2512f, 2.0f};
static const glm::vec3 INSPECTED_CARD_POSITION = {0.0f, -0.0f, 23.2f};
static const glm::vec3 CARD_TOOLTIP_POSITION_OFFSET = {0.0f, 0.1f, 2.0f};
static const glm::vec3 CARD_TOOLTIP_BASE_SCALE = {0.3f, 0.274f, 1/10.0f};

static const float SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.1f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    CARD_INSPECTION_SCENE_NAME
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& CardInspectionSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

CardInspectionSceneLogicManager::CardInspectionSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

CardInspectionSceneLogicManager::~CardInspectionSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void CardInspectionSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void CardInspectionSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mTransitioning = false;
    
    // Create card wrapper
    const auto& cardData = CardDataRepository::GetInstance().GetCardData(DataRepository::GetInstance().GetNextInspectedCardId(), game_constants::REMOTE_PLAYER_INDEX);
    bool isOpponentHeroCard = cardData.mCardName == strutils::StringId(DataRepository::GetInstance().GetNextStoryOpponentName());
    mCardSoWrapper = card_utils::CreateCardSoWrapper(&cardData, INSPECTED_CARD_POSITION, INSPECTED_CARD_NAME_PREFIX, CardOrientation::FRONT_FACE, isOpponentHeroCard ? CardRarity::GOLDEN : CardRarity::NORMAL, true, false, true, {}, {}, *scene);
    mCardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    mCardSoWrapper->mSceneObject->mScale = INSPECTED_CARD_SCALE;
    
    mCardTooltipController = nullptr;
    if (cardData.IsSpell())
    {
        mCardTooltipController = std::make_unique<CardTooltipController>
        (
            mCardSoWrapper->mSceneObject->mPosition + CARD_TOOLTIP_POSITION_OFFSET,
            CARD_TOOLTIP_BASE_SCALE,
            mCardSoWrapper->mCardData.mCardEffectTooltip,
            false,
            false,
            false,
            *scene
        );
    }

    mAnimatedButtons.clear();
    mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        CONTINUE_BUTTON_POSITION,
        BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        "Continue",
        CONTINUE_BUTTON_NAME,
        [=]()
        {
            mTransitioning = true;
            events::EventSystem::GetInstance().DispatchEvent<events::PopSceneModalEvent>();
        },
        *scene
    ));

    size_t sceneObjectIndex = 0;
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
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=]()
        {
        });
    }
}

///------------------------------------------------------------------------------------------------

void CardInspectionSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene>)
{
    static float time = 0.0f;
    time += dtMillis * 0.001f;
    
    mCardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
    
    if (mTransitioning)
    {
        return;
    }
    
    if (mCardTooltipController)
    {
        mCardTooltipController->Update(dtMillis);
    }
    
    // Animated buttons
    for (auto& animatedButton: mAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void CardInspectionSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    if (mCardTooltipController)
    {
        for (auto sceneObject: mCardTooltipController->GetSceneObjects())
        {
            scene->RemoveSceneObject(sceneObject->mName);
        }
    }
    
    mCardTooltipController = nullptr;
    
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            if (sceneObject->mName == mCardSoWrapper->mSceneObject->mName || sceneObject->mName == mAnimatedButtons.front()->GetSceneObject()->mName)
            {
                scene->RemoveSceneObject(sceneObject->mName);
            }
            else
            {
                sceneObject->mInvisible = true;
            }
        });
    }
    
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto previousScene = sceneManager.FindScene(mPreviousScene);
    
    animationManager.StopAnimation(game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenValueAnimation>(previousScene->GetUpdateTimeSpeedFactor(), 1.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> CardInspectionSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------
