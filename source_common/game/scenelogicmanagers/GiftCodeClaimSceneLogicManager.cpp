///------------------------------------------------------------------------------------------------
///  GiftCodeClaimSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 06/02/2024
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/SceneObjectUtils.h>
#include <game/AnimatedButton.h>
#include <game/Cards.h>
#include <game/DataRepository.h>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/GiftCodeClaimSceneLogicManager.h>
#include <SDL_events.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId GIFT_CODE_CLAIM_SCENE_NAME = strutils::StringId("gift_code_claim_scene");
static const strutils::StringId CONTINUE_BUTTON_NAME = strutils::StringId("continue_button");
static const strutils::StringId GIFT_CODE_CLAIM_TITLE_SCENE_OBJECT_NAME = strutils::StringId("gift_code_claim_title");
static const strutils::StringId GIFT_CODE_CLAIM_RESULT_TEXT_TOP_NAME = strutils::StringId("gift_code_claim_result_text_top");
static const strutils::StringId GIFT_CODE_CLAIM_RESULT_TEXT_BOT_NAME = strutils::StringId("gift_code_claim_result_text_bot");

static const std::string RESULT_TEXT_SHADER_FILE_NAME = "basic_custom_color.vs";

static const glm::vec3 BUTTON_SCALE = {0.0005f, 0.0005f, 0.0005f};
static const glm::vec3 CONTINUE_BUTTON_POSITION = {-0.091f, -0.114f, 23.1f};
static const glm::vec3 GIFT_CODE_CLAIM_RESULT_TEXT_TOP_POSITION = {0.0f, 0.07f, 23.1f};
static const glm::vec3 GIFT_CODE_CLAIM_RESULT_TEXT_BOT_POSITION = {0.0f, 0.00f, 23.1f};
static const glm::vec3 GIFT_CODE_RESULT_SUCCESS_COLOR = {0.0f, 0.7f, 0.0f};
static const glm::vec3 GIFT_CODE_RESULT_FAILURE_COLOR = {0.8f, 0.0f, 0.0f};

static const float SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.1f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    GIFT_CODE_CLAIM_SCENE_NAME
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    GIFT_CODE_CLAIM_TITLE_SCENE_OBJECT_NAME,
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

static const std::unordered_map<GiftCodeClaimedResultType, std::string> CLAIM_RESULT_FAILURE_TYPE_TO_TEXT =
{
    { GiftCodeClaimedResultType::FAILURE_INVALID_CODE, "Gift Code invalid code!"},
    { GiftCodeClaimedResultType::FAILURE_INVALID_PRODUCT, "Gift Code invalid product!"},
    { GiftCodeClaimedResultType::FAILURE_USED_ALREADY, "Gift Code used already!"}
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& GiftCodeClaimSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

GiftCodeClaimSceneLogicManager::GiftCodeClaimSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

GiftCodeClaimSceneLogicManager::~GiftCodeClaimSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void GiftCodeClaimSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void GiftCodeClaimSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mTransitioning = false;
    
    scene->RemoveAllSceneObjectsButTheOnesNamed(STATIC_SCENE_ELEMENTS);
    mAnimatedButtons.clear();
    
    auto claimResult = DataRepository::GetInstance().GetCurrentGiftCodeClaimedResultType();
    
    auto textClaimResultTopSceneObject = scene->CreateSceneObject(GIFT_CODE_CLAIM_RESULT_TEXT_TOP_NAME);
    auto textClaimResultBotSceneObject = scene->CreateSceneObject(GIFT_CODE_CLAIM_RESULT_TEXT_BOT_NAME);
    textClaimResultTopSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + RESULT_TEXT_SHADER_FILE_NAME);
    textClaimResultBotSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + RESULT_TEXT_SHADER_FILE_NAME);
    
    if (claimResult == GiftCodeClaimedResultType::SUCCESS)
    {
        scene::TextSceneObjectData textClaimResultTop;
        textClaimResultTop.mFontName = game_constants::DEFAULT_FONT_NAME;
        textClaimResultTop.mText = "Gift Code claimed successfully!";
        textClaimResultTopSceneObject->mSceneObjectTypeData = std::move(textClaimResultTop);
        textClaimResultTopSceneObject->mPosition = GIFT_CODE_CLAIM_RESULT_TEXT_TOP_POSITION;
        textClaimResultTopSceneObject->mScale = BUTTON_SCALE;
        textClaimResultTopSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = GIFT_CODE_RESULT_SUCCESS_COLOR;
        
        scene::TextSceneObjectData textClaimResultBot;
        textClaimResultBot.mFontName = game_constants::DEFAULT_FONT_NAME;
        textClaimResultBot.mText = "";
        textClaimResultBotSceneObject->mSceneObjectTypeData = std::move(textClaimResultBot);
        textClaimResultBotSceneObject->mPosition = GIFT_CODE_CLAIM_RESULT_TEXT_BOT_POSITION;
        textClaimResultBotSceneObject->mScale = BUTTON_SCALE;
    }
    else
    {
        scene::TextSceneObjectData textClaimResultTop;
        textClaimResultTop.mFontName = game_constants::DEFAULT_FONT_NAME;
        textClaimResultTop.mText = "Gift Code claim failure:";
        textClaimResultTopSceneObject->mSceneObjectTypeData = std::move(textClaimResultTop);
        textClaimResultTopSceneObject->mPosition = GIFT_CODE_CLAIM_RESULT_TEXT_TOP_POSITION;
        textClaimResultTopSceneObject->mScale = BUTTON_SCALE;
        textClaimResultTopSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = GIFT_CODE_RESULT_FAILURE_COLOR;
        
        scene::TextSceneObjectData textClaimResultBot;
        textClaimResultBot.mFontName = game_constants::DEFAULT_FONT_NAME;
        textClaimResultBot.mText = CLAIM_RESULT_FAILURE_TYPE_TO_TEXT.at(claimResult);
        textClaimResultBotSceneObject->mSceneObjectTypeData = std::move(textClaimResultBot);
        textClaimResultBotSceneObject->mPosition = GIFT_CODE_CLAIM_RESULT_TEXT_BOT_POSITION;
        textClaimResultBotSceneObject->mScale = BUTTON_SCALE;
        textClaimResultBotSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = GIFT_CODE_RESULT_FAILURE_COLOR;
    }
    
    auto boundingRectTopText = scene_object_utils::GetSceneObjectBoundingRect(*textClaimResultTopSceneObject);
    auto textLength = boundingRectTopText.topRight.x - boundingRectTopText.bottomLeft.x;
    textClaimResultTopSceneObject->mPosition.x -= textLength/2.0f;
    
    if (claimResult != GiftCodeClaimedResultType::SUCCESS)
    {
        auto boundingRectBotText = scene_object_utils::GetSceneObjectBoundingRect(*textClaimResultBotSceneObject);
        auto textLength = boundingRectBotText.topRight.x - boundingRectBotText.bottomLeft.x;
        textClaimResultBotSceneObject->mPosition.x -= textLength/2.0f;
    }
    
    mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        CONTINUE_BUTTON_POSITION,
        BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        "Continue",
        CONTINUE_BUTTON_NAME,
        [=]()
        {
            events::EventSystem::GetInstance().DispatchEvent<events::PopSceneModalEvent>();
            mTransitioning = true;
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
            mTransitioning = false;
        });
    }
}

///------------------------------------------------------------------------------------------------

void GiftCodeClaimSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene>)
{
    if (mTransitioning)
    {
        return;
    }
    
    // Animated buttons
    for (auto& animatedButton: mAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void GiftCodeClaimSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            sceneObject->mInvisible = true;
        });
    }
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> GiftCodeClaimSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------
