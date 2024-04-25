///------------------------------------------------------------------------------------------------
///  PrivacyPolicySceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 14/02/2024
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
#include <game/scenelogicmanagers/PrivacyPolicySceneLogicManager.h>
#include <SDL_events.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId CONTINUE_BUTTON_NAME = strutils::StringId("continue_button");
static const strutils::StringId TEXT_CONTAINER_SCENE_OBJECT_NAME = strutils::StringId("text_container");

static const std::string PRIVACY_POLICY_FILE_PATH = "privacy_policy/privacy_policy.txt";
static const std::string TEXT_ENTRY_SHADER_FILE_NAME = "text_container_entry.vs";

static const glm::vec3 BUTTON_SCALE = {0.0004f, 0.0004f, 0.0004f};
static const glm::vec3 CONTINUE_BUTTON_POSITION = {-0.078f, -0.211f, 23.1f};
static const glm::vec3 TEXT_SCALE = glm::vec3(0.0004f, 0.0004f, 0.0004f);
static const glm::vec3 TEXT_CONTAINER_ENTRY_SCALE = glm::vec3(0.0004f, 0.04f, 0.0004f);

static const glm::vec2 TEXT_ENTRY_CUTOFF_VALUES = {-0.193f, 0.173f};
static const glm::vec2 TEXT_CONTAINER_CUTOFF_VALUES = {-0.085f, 0.065f};

static const math::Rectangle TEXT_CONTAINER_BOUNDS = {{-0.305f, -0.205f}, {0.305f, 0.165f}};

static const float SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.01f;
static const float TEXT_ENTRY_Z = 23.2f;

static constexpr int PRIVACY_POLICY_LINE_CHAR_LIMIT = 35;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    strutils::StringId("privacy_policy_scene")
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& PrivacyPolicySceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

PrivacyPolicySceneLogicManager::PrivacyPolicySceneLogicManager(){}

///------------------------------------------------------------------------------------------------

PrivacyPolicySceneLogicManager::~PrivacyPolicySceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void PrivacyPolicySceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void PrivacyPolicySceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
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
    
    
    bool containerExists = mTextContainer != nullptr;
    if (containerExists)
    {
        for (const auto& containerItem: mTextContainer->GetItems())
        {
            for (const auto& sceneObject: containerItem.mSceneObjects)
            {
                CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
                scene->RemoveSceneObject(sceneObject->mName);
            }
        }
        
        mTextContainer = nullptr;
    }
    
    mTextContainer = std::make_unique<SwipeableContainer<TextEntry>>
    (
        ContainerType::VERTICAL_MATRIX,
        TEXT_CONTAINER_ENTRY_SCALE,
        TEXT_CONTAINER_BOUNDS,
        TEXT_CONTAINER_CUTOFF_VALUES,
        TEXT_CONTAINER_SCENE_OBJECT_NAME,
        TEXT_ENTRY_Z,
        *scene,
        10,
        true
    );
    
    auto privacyPolicyResource = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_DATA_ROOT + PRIVACY_POLICY_FILE_PATH);
    auto privacyPolicyText = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::DataFileResource>(privacyPolicyResource).GetContents();
    
    auto privacyPolicySplitByNewline = strutils::StringSplit(privacyPolicyText, '\n');
    std::stringstream textLineBuilder;
    for (const auto& line: privacyPolicySplitByNewline)
    {
        for (auto i = 0; i < line.size(); i += PRIVACY_POLICY_LINE_CHAR_LIMIT)
        {
            auto lineString = line.substr(i, PRIVACY_POLICY_LINE_CHAR_LIMIT);
            while (i + PRIVACY_POLICY_LINE_CHAR_LIMIT < line.size())
            {
                lineString += line[i + PRIVACY_POLICY_LINE_CHAR_LIMIT];
                i++;
                
                if (lineString.back() == ' ')
                {
                    break;
                }
            }
            
            scene::TextSceneObjectData textData;
            textData.mFontName = game_constants::DEFAULT_FONT_NAME;
            textData.mText = lineString;
            auto textSceneObject = scene->CreateSceneObject();
            textSceneObject->mSceneObjectTypeData = std::move(textData);
            textSceneObject->mScale = TEXT_SCALE;
            textSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = TEXT_ENTRY_CUTOFF_VALUES.s;
            textSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = TEXT_ENTRY_CUTOFF_VALUES.t;
            textSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + TEXT_ENTRY_SHADER_FILE_NAME);
            
            TextEntry textEntry;
            textEntry.mSceneObjects.push_back(textSceneObject);
            mTextContainer->AddItem(std::move(textEntry), EntryAdditionStrategy::ADD_ON_THE_BACK);
        }

        scene::TextSceneObjectData textData;
        textData.mFontName = game_constants::DEFAULT_FONT_NAME;
        textData.mText = "";
        auto textSceneObject = scene->CreateSceneObject();
        textSceneObject->mSceneObjectTypeData = std::move(textData);
        textSceneObject->mScale = TEXT_SCALE;
        textSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = TEXT_ENTRY_CUTOFF_VALUES.s;
        textSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = TEXT_ENTRY_CUTOFF_VALUES.t;
        textSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + TEXT_ENTRY_SHADER_FILE_NAME);
        
        TextEntry textEntry;
        textEntry.mSceneObjects.push_back(textSceneObject);
        mTextContainer->AddItem(std::move(textEntry), EntryAdditionStrategy::ADD_ON_THE_BACK);
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

void PrivacyPolicySceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene>)
{
    if (mTransitioning)
    {
        return;
    }
    
    mTextContainer->Update(dtMillis);
    
    // Animated buttons
    for (auto& animatedButton: mAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void PrivacyPolicySceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
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

std::shared_ptr<GuiObjectManager> PrivacyPolicySceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------
