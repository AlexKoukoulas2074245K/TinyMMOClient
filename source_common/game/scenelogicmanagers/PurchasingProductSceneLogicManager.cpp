///------------------------------------------------------------------------------------------------
///  PurchasingProductSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 30/01/2024
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
#include <game/IAPProductIds.h>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/PurchasingProductSceneLogicManager.h>
#include <SDL_events.h>
#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#include <platform_utilities/CloudKitUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static const strutils::StringId PURCHASING_PRODUCT_SCENE_NAME = strutils::StringId("purchasing_product_scene");
static const strutils::StringId TITLE_SCENE_OBJECT_NAME = strutils::StringId("purchasing_product_title");
static const strutils::StringId SPINNER_SCENE_OBJECT_NAME = strutils::StringId("spinner");
static const strutils::StringId CONTINUE_BUTTON_NAME = strutils::StringId("continue_button");
static const strutils::StringId PURCHASE_OUTCOME_TEXT_0_SCENE_OBJECT_NAME = strutils::StringId("purchase_outcome_text_0");
static const strutils::StringId PURCHASE_OUTCOME_TEXT_1_SCENE_OBJECT_NAME = strutils::StringId("purchase_outcome_text_1");

static const std::string PAYMENT_SUCCESSFUL_ICON_TEXTURE_FILE_NAME = "spinner_success.png";
static const std::string PAYMENT_UNSUCCESSFUL_ICON_TEXTURE_FILE_NAME = "spinner_failure.png";
static const std::string PAYMENT_PENDING_ICON_TEXTURE_FILE_NAME = "spinner.png";

static const glm::vec3 BUTTON_SCALE = {0.0005f, 0.0005f, 0.0005f};
static const glm::vec3 CONTINUE_BUTTON_POSITION = {-0.071f, -0.141f, 23.1f};
static const glm::vec3 PURCHASE_OUTCOME_TEXT_0_POSITION = {0.0f, 0.140f, 23.1f};
static const glm::vec3 PURCHASE_OUTCOME_TEXT_1_POSITION = {0.0f, 0.088f, 23.1f};
static const glm::vec3 PURCHASE_PENDING_TEXT_POSITION = {0.0f, 0.117f, 23.1f};

static const float SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.1f;
static const float SPINNER_ROTATION_SPEED = 0.003f;
static const float MIN_TIME_BEFORE_TRANSITIONING_TO_SUBSCENE_SECS = 3.0f;
static const float SUCCESSFUL_COINS_PURCHASE_TEXT_Y_OFFSET = -0.02f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    PURCHASING_PRODUCT_SCENE_NAME
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    SPINNER_SCENE_OBJECT_NAME,
    TITLE_SCENE_OBJECT_NAME,
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& PurchasingProductSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

PurchasingProductSceneLogicManager::PurchasingProductSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

PurchasingProductSceneLogicManager::~PurchasingProductSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void PurchasingProductSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void PurchasingProductSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mNextSubScene = SubSceneType::NONE;
    mActiveSubScene = SubSceneType::NONE;
    
    mTransitioningToSubScene = false;
    mShouldTriggerPurchaseEndedEvent = false;
    InitSubScene(SubSceneType::MAIN, scene);
    
    mMinTimeBeforeTransitioningToSubSceneSecs = MIN_TIME_BEFORE_TRANSITIONING_TO_SUBSCENE_SECS;
    
#if defined(MACOS) || defined(MOBILE_FLOW)
    apple_utils::InitiateProductPurchase(DataRepository::GetInstance().GetPermaShopProductNameToPurchase(), [=](apple_utils::PurchaseResultData purchaseResultData)
    {
        if (purchaseResultData.mWasSuccessful)
        {
            auto successfulTransactionIds = DataRepository::GetInstance().GetSuccessfulTransactionIds();
            successfulTransactionIds.push_back(purchaseResultData.mTransactionId);
            DataRepository::GetInstance().SetSuccessfulTransactionIds(successfulTransactionIds);
            DataRepository::GetInstance().FlushStateToFile();
            mNextSubScene = SubSceneType::PURCHASE_SUCCESSFUL;
        }
        else
        {
            mNextSubScene = SubSceneType::PURCHASE_UNSUCCESSFUL;
        }
        
        mShouldTriggerPurchaseEndedEvent = true;
    });
#endif
    
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto previousScene = sceneManager.FindScene(mPreviousScene);
    previousScene->GetUpdateTimeSpeedFactor() = 0.0f;
    
}

///------------------------------------------------------------------------------------------------

void PurchasingProductSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    if (mActiveSubScene == SubSceneType::MAIN)
    {
        auto spinnerSceneObject = scene->FindSceneObject(SPINNER_SCENE_OBJECT_NAME);
        spinnerSceneObject->mRotation.z -= dtMillis * SPINNER_ROTATION_SPEED;
        if (spinnerSceneObject->mRotation.z <= -2.0f * math::PI)
        {
            spinnerSceneObject->mRotation.z += 2.0f * math::PI;
        }
    }
    
    if (mShouldTriggerPurchaseEndedEvent)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::ProductPurchaseEndedEvent>(mNextSubScene == SubSceneType::PURCHASE_SUCCESSFUL || mActiveSubScene == SubSceneType::PURCHASE_SUCCESSFUL);
        mShouldTriggerPurchaseEndedEvent = false;
    }
    
    if (mTransitioningToSubScene)
    {
        return;
    }
    
    // Animated buttons
    for (auto& animatedButton: mAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
    
    if (mActiveSubScene == SubSceneType::MAIN)
    {
        mMinTimeBeforeTransitioningToSubSceneSecs = math::Max(0.0f, mMinTimeBeforeTransitioningToSubSceneSecs - dtMillis/1000.0f);
        if (mNextSubScene != SubSceneType::NONE && mMinTimeBeforeTransitioningToSubSceneSecs <= 0.0f)
        {
            TransitionToSubScene(mNextSubScene, scene);
        }
    }
}

///------------------------------------------------------------------------------------------------

void PurchasingProductSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
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
    
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto previousScene = sceneManager.FindScene(mPreviousScene);
    previousScene->GetUpdateTimeSpeedFactor() = 1.0f;
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> PurchasingProductSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void PurchasingProductSceneLogicManager::InitSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene)
{
    if (mActiveSubScene == subSceneType)
    {
        return;
    }
    
    mActiveSubScene = subSceneType;
    
    scene->RemoveAllSceneObjectsButTheOnesNamed(STATIC_SCENE_ELEMENTS);
    mAnimatedButtons.clear();
    
    auto spinnerSceneObject = scene->FindSceneObject(SPINNER_SCENE_OBJECT_NAME);
    spinnerSceneObject->mRotation.z = 0.0f;
    
    switch (subSceneType)
    {
        case SubSceneType::MAIN:
        {
            scene->FindSceneObject(SPINNER_SCENE_OBJECT_NAME)->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + PAYMENT_PENDING_ICON_TEXTURE_FILE_NAME);
        
            scene::TextSceneObjectData textDataPurchaseOutcomeTop;
            textDataPurchaseOutcomeTop.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataPurchaseOutcomeTop.mText = "Hang on while we process your order";
            auto purchaseOutcomeTextTopSceneObject = scene->CreateSceneObject(PURCHASE_OUTCOME_TEXT_0_SCENE_OBJECT_NAME);
            purchaseOutcomeTextTopSceneObject->mSceneObjectTypeData = std::move(textDataPurchaseOutcomeTop);
            purchaseOutcomeTextTopSceneObject->mPosition = PURCHASE_PENDING_TEXT_POSITION;
            purchaseOutcomeTextTopSceneObject->mScale = BUTTON_SCALE;
            
            std::get<scene::TextSceneObjectData>(scene->FindSceneObject(TITLE_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = "Purchasing Product!";
        } break;
            
        case SubSceneType::PURCHASE_SUCCESSFUL:
        {
            scene->FindSceneObject(SPINNER_SCENE_OBJECT_NAME)->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + PAYMENT_SUCCESSFUL_ICON_TEXTURE_FILE_NAME);
        
            std::get<scene::TextSceneObjectData>(scene->FindSceneObject(TITLE_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = "Purchase Successful!";
            
            auto purchasedProductName = DataRepository::GetInstance().GetPermaShopProductNameToPurchase();
            if (purchasedProductName == iap_product_ids::COINS_S || purchasedProductName == iap_product_ids::COINS_M || purchasedProductName == iap_product_ids::COINS_L)
            {
                scene::TextSceneObjectData textDataPurchaseOutcomeTop;
                textDataPurchaseOutcomeTop.mFontName = game_constants::DEFAULT_FONT_NAME;
                textDataPurchaseOutcomeTop.mText = "Enjoy your shiny new gold coins!";
                auto purchaseOutcomeTextTopSceneObject = scene->CreateSceneObject(PURCHASE_OUTCOME_TEXT_0_SCENE_OBJECT_NAME);
                purchaseOutcomeTextTopSceneObject->mSceneObjectTypeData = std::move(textDataPurchaseOutcomeTop);
                purchaseOutcomeTextTopSceneObject->mPosition = PURCHASE_OUTCOME_TEXT_0_POSITION;
                purchaseOutcomeTextTopSceneObject->mPosition.y += SUCCESSFUL_COINS_PURCHASE_TEXT_Y_OFFSET;
                purchaseOutcomeTextTopSceneObject->mScale = BUTTON_SCALE;
            }
            else if (purchasedProductName == iap_product_ids::STORY_HEALTH_REFILL)
            {
                scene::TextSceneObjectData textDataPurchaseOutcomeTop;
                textDataPurchaseOutcomeTop.mFontName = game_constants::DEFAULT_FONT_NAME;
                textDataPurchaseOutcomeTop.mText = "Story health is fully restored!";
                auto purchaseOutcomeTextTopSceneObject = scene->CreateSceneObject(PURCHASE_OUTCOME_TEXT_0_SCENE_OBJECT_NAME);
                purchaseOutcomeTextTopSceneObject->mSceneObjectTypeData = std::move(textDataPurchaseOutcomeTop);
                purchaseOutcomeTextTopSceneObject->mPosition = PURCHASE_OUTCOME_TEXT_0_POSITION;
                purchaseOutcomeTextTopSceneObject->mPosition.y += SUCCESSFUL_COINS_PURCHASE_TEXT_Y_OFFSET;
                purchaseOutcomeTextTopSceneObject->mScale = BUTTON_SCALE;
            }
            else
            {
                scene::TextSceneObjectData textDataPurchaseOutcomeTop;
                textDataPurchaseOutcomeTop.mFontName = game_constants::DEFAULT_FONT_NAME;
                textDataPurchaseOutcomeTop.mText = "Your packs will automatically open";
                auto purchaseOutcomeTextTopSceneObject = scene->CreateSceneObject(PURCHASE_OUTCOME_TEXT_0_SCENE_OBJECT_NAME);
                purchaseOutcomeTextTopSceneObject->mSceneObjectTypeData = std::move(textDataPurchaseOutcomeTop);
                purchaseOutcomeTextTopSceneObject->mPosition = PURCHASE_OUTCOME_TEXT_0_POSITION;
                purchaseOutcomeTextTopSceneObject->mScale = BUTTON_SCALE;
                
                scene::TextSceneObjectData textDataPurchaseOutcomeBot;
                textDataPurchaseOutcomeBot.mFontName = game_constants::DEFAULT_FONT_NAME;
                textDataPurchaseOutcomeBot.mText = "next time you go to the main menu!";
                auto purchaseOutcomeTextBotSceneObject = scene->CreateSceneObject(PURCHASE_OUTCOME_TEXT_1_SCENE_OBJECT_NAME);
                purchaseOutcomeTextBotSceneObject->mSceneObjectTypeData = std::move(textDataPurchaseOutcomeBot);
                purchaseOutcomeTextBotSceneObject->mPosition = PURCHASE_OUTCOME_TEXT_1_POSITION;
                purchaseOutcomeTextBotSceneObject->mScale = BUTTON_SCALE;
            }
        } break;
        
        case SubSceneType::PURCHASE_UNSUCCESSFUL:
        {
            scene->FindSceneObject(SPINNER_SCENE_OBJECT_NAME)->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + PAYMENT_UNSUCCESSFUL_ICON_TEXTURE_FILE_NAME);
            
            std::get<scene::TextSceneObjectData>(scene->FindSceneObject(TITLE_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = "Purchase Failure!";
            
            scene::TextSceneObjectData textDataPurchaseOutcomeTop;
            textDataPurchaseOutcomeTop.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataPurchaseOutcomeTop.mText = "The purchase was unsuccessful.";
            auto purchaseOutcomeTextTopSceneObject = scene->CreateSceneObject(PURCHASE_OUTCOME_TEXT_0_SCENE_OBJECT_NAME);
            purchaseOutcomeTextTopSceneObject->mSceneObjectTypeData = std::move(textDataPurchaseOutcomeTop);
            purchaseOutcomeTextTopSceneObject->mPosition = PURCHASE_OUTCOME_TEXT_0_POSITION;
            purchaseOutcomeTextTopSceneObject->mScale = BUTTON_SCALE;
            
            scene::TextSceneObjectData textDataPurchaseOutcomeBot;
            textDataPurchaseOutcomeBot.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataPurchaseOutcomeBot.mText = "Check your details and try again.";
            auto purchaseOutcomeTextBotSceneObject = scene->CreateSceneObject(PURCHASE_OUTCOME_TEXT_1_SCENE_OBJECT_NAME);
            purchaseOutcomeTextBotSceneObject->mSceneObjectTypeData = std::move(textDataPurchaseOutcomeBot);
            purchaseOutcomeTextBotSceneObject->mPosition = PURCHASE_OUTCOME_TEXT_1_POSITION;
            purchaseOutcomeTextBotSceneObject->mScale = BUTTON_SCALE;
        } break;
            
        default: break;
    }
    
    if (subSceneType != SubSceneType::MAIN)
    {
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
            },
            *scene
        ));
    }
    
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
        
        if (sceneObject->mName == TITLE_SCENE_OBJECT_NAME || sceneObject->mName == PURCHASE_OUTCOME_TEXT_0_SCENE_OBJECT_NAME || sceneObject->mName == PURCHASE_OUTCOME_TEXT_1_SCENE_OBJECT_NAME)
        {
            auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*sceneObject);
            auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
            sceneObject->mPosition.x = -textLength/2.0f;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=]()
        {
            mTransitioningToSubScene = false;
        });
    }
}

///------------------------------------------------------------------------------------------------

void PurchasingProductSceneLogicManager::TransitionToSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene)
{
    mTransitioningToSubScene = true;
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            InitSubScene(subSceneType, scene);
        });
    }
}

///------------------------------------------------------------------------------------------------
