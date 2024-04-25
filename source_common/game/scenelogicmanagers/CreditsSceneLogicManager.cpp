///------------------------------------------------------------------------------------------------
///  CreditsSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 07/03/2024
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <game/AnimatedButton.h>
#include <game/Cards.h>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/CreditsSceneLogicManager.h>
#include <SDL_events.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId CONTINUE_BUTTON_NAME = strutils::StringId("continue_button");
static const strutils::StringId TEXT_CONTAINER_SCENE_OBJECT_NAME = strutils::StringId("text_container");

static const std::string CREDITS_FILE_PATH = "credits/credits.txt";
static const std::string TEXT_ENTRY_SHADER_FILE_NAME = "text_container_entry.vs";

static const glm::vec3 BUTTON_SCALE = {0.0004f, 0.0004f, 0.0004f};
static const glm::vec3 CONTINUE_BUTTON_POSITION = {-0.078f, -0.211f, 23.1f};
static const glm::vec3 TEXT_SCALE = glm::vec3(0.0004f, 0.0004f, 0.0004f);
static const glm::vec3 TEXT_INIT_POSITION = {0.0f, -0.2f, 23.2f};

static const glm::vec2 TEXT_ENTRY_CUTOFF_VALUES = {-0.193f, 0.173f};

static const float SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.01f;
static const float TEXT_SPEED = 0.00006f;
static const float WARP_Y = 2.2f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    strutils::StringId("credits_scene")
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& CreditsSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

CreditsSceneLogicManager::CreditsSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

CreditsSceneLogicManager::~CreditsSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void CreditsSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void CreditsSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mTransitioning = false;
    
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
    
    bool textElementsExist = !mTextSceneObjects.empty();
    if (textElementsExist)
    {
        for (const auto& sceneObject: mTextSceneObjects)
        {
            CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
            scene->RemoveSceneObject(sceneObject->mName);
        }
        
        mTextSceneObjects.clear();
    }
    
    
    auto creditsResource = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_DATA_ROOT + CREDITS_FILE_PATH);
    auto creditsText = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::DataFileResource>(creditsResource).GetContents();
    
    auto creditsSplitByNewline = strutils::StringSplit(creditsText, '\n');
    auto lineCounter = 0;
    for (const auto& line: creditsSplitByNewline)
    {
        scene::TextSceneObjectData textData;
        textData.mFontName = game_constants::DEFAULT_FONT_NAME;
        textData.mText = line;
        auto textSceneObject = scene->CreateSceneObject(strutils::StringId("credits_text_" + std::to_string(lineCounter)));
        textSceneObject->mSceneObjectTypeData = std::move(textData);
        textSceneObject->mPosition = TEXT_INIT_POSITION;
        textSceneObject->mPosition.y -= lineCounter * 0.05f;
        textSceneObject->mScale = TEXT_SCALE;
        
        auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*textSceneObject);
        auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
        textSceneObject->mPosition.x -= textLength/2.0f;
        
        textSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = TEXT_ENTRY_CUTOFF_VALUES.s;
        textSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = TEXT_ENTRY_CUTOFF_VALUES.t;
        textSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + TEXT_ENTRY_SHADER_FILE_NAME);
        mTextSceneObjects.push_back(textSceneObject);
        lineCounter++;
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
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=]()
        {
        });
    }
}

///------------------------------------------------------------------------------------------------

void CreditsSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene>)
{
    if (mTransitioning)
    {
        return;
    }
    
    for (int i = 0; i < mTextSceneObjects.size(); ++i)
    {
        auto& sceneObject = mTextSceneObjects[i];
        sceneObject->mPosition.y += dtMillis * TEXT_SPEED;
        
        if (sceneObject->mPosition.y > WARP_Y)
        {
            sceneObject->mPosition.y = TEXT_INIT_POSITION.y;
        }
    }
    
    // Animated buttons
    for (auto& animatedButton: mAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void CreditsSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
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
            
            if (sceneObject->mName == CONTINUE_BUTTON_NAME)
            {
                scene->RemoveSceneObject(sceneObject->mName);
            }
        });
    }
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> CreditsSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------
