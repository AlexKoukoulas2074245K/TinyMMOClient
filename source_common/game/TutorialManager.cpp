///------------------------------------------------------------------------------------------------
///  TutorialManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 06/03/2024
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <engine/utils/Logging.h>
#include <game/AnimatedButton.h>
#include <game/DataRepository.h>
#include <game/GameSymbolicGlyphNames.h>
#include <game/TutorialManager.h>
#include <nlohmann/json.hpp>

///------------------------------------------------------------------------------------------------

static constexpr int TUTORIAL_TEXT_ROWS_COUNT = 9;

static const strutils::StringId TUTORIAL_BASE_SCENE_OBJECT_NAME = strutils::StringId("tutorial_base");
static const strutils::StringId TUTORIAL_ARROW_SCENE_OBJECT_NAME = strutils::StringId("tutorial_arrow");
static const strutils::StringId TUTORIAL_REVEAL_THRESHOLD_UNIFORM_NAME = strutils::StringId("reveal_threshold");
static const strutils::StringId TUTORIAL_REVEAL_RGB_EXPONENT_UNIFORM_NAME = strutils::StringId("reveal_rgb_exponent");
static const strutils::StringId TUTORIAL_TEXT_SCENE_OBJECT_NAMES[TUTORIAL_TEXT_ROWS_COUNT] =
{
    strutils::StringId("tutorial_text_0"),
    strutils::StringId("tutorial_text_1"),
    strutils::StringId("tutorial_text_2"),
    strutils::StringId("tutorial_text_3"),
    strutils::StringId("tutorial_text_4"),
    strutils::StringId("tutorial_text_5"),
    strutils::StringId("tutorial_text_6"),
    strutils::StringId("tutorial_text_7"),
    strutils::StringId("tutorial_text_8")
};

static const std::string TUTORIAL_TEXTURE_FILE_NAME = "tutorial.png";
static const std::string TUTORIAL_SHADER_FILE_NAME = "diagonal_reveal.vs";
static const std::string TUTORIAL_ARROW_TEXTURE_FILE_NAME = "tutorial_arrow.png";
static const std::string CHECKBOX_EMPTY_TEXTURE_FILE_NAME = "checkbox_empty.png";
static const std::string CHECKBOX_FILLED_TEXTURE_FILE_NAME = "checkbox_filled_black.png";

static const glm::vec3 TUTORIAL_BASE_POSITION = {0.0f, 0.0f, 27.0f};
static const glm::vec3 TUTORIAL_TEXT_SCALE = {0.00032f, 0.00032f, 0.00032f};
static const glm::vec3 TUTORIAL_BASE_SCALE = {0.4f, 0.4f, 0.4f};
static const glm::vec3 CHECKBOX_SCALE = {0.07f, 0.07f, 0.07f};
static const glm::vec3 ARROW_SCALE = {0.14f, 0.14f, 0.14f};
static const glm::vec3 TUTORIAL_TEXT_OFFSETS[TUTORIAL_TEXT_ROWS_COUNT] =
{
    { -0.117f, 0.137f, 0.1f}, // Tutorial Title
    {  0.119f, 0.132f, 0.1f }, // Tutorials checkbox
    { -0.139f, 0.097f, 0.1f },
    { -0.139f, 0.063f, 0.1f },
    { -0.139f, 0.029f, 0.1f },
    { -0.139f, -0.005f, 0.1f },
    { -0.139f, -0.039f, 0.1f },
    { -0.139f, -0.073f, 0.1f },
    { -0.044f, -0.121f, 0.1f }, // Continue button
};

static const float TUTORIAL_MAX_REVEAL_THRESHOLD = 2.5f;
static const float TUTORIAL_REVEAL_SPEED = 1.0f/200.0f;
static const float TUTORIAL_TEXT_REVEAL_SPEED = 1.0f/500.0f;
static const float TUTORIAL_REVEAL_RGB_EXPONENT = 1.127f;
static const float TUTORIAL_FADE_OUT_DURATION_SECS = 0.5f;
static const float TUTORIAL_DELETION_DELAY_SECS = 0.6f;
static const float TUTORIAL_ARROW_SPEED = 0.0001f;
static const float TUTORIAL_ARROW_BOUNCE_DURATION_SECS = 1.0f;
static const float TUTORIAL_ARROW_Z = 27.2f;

///------------------------------------------------------------------------------------------------

TutorialManager::TutorialManager()
{
    events::EventSystem::GetInstance().RegisterForEvent<events::TutorialTriggerEvent>(this, &TutorialManager::OnTutorialTrigger);
}

///------------------------------------------------------------------------------------------------

TutorialManager::~TutorialManager()
{
}

///------------------------------------------------------------------------------------------------

const std::unordered_map<strutils::StringId, TutorialDefinition, strutils::StringIdHasher>& TutorialManager::GetTutorialDefinitions() const
{
    return mTutorialDefinitions;
}

///------------------------------------------------------------------------------------------------

bool TutorialManager::HasAnyActiveTutorial() const
{
    return !mActiveTutorials.empty();
}

///------------------------------------------------------------------------------------------------

bool TutorialManager::IsTutorialActive(const strutils::StringId& tutorialName) const
{
    return std::find_if(mActiveTutorials.cbegin(), mActiveTutorials.cend(), [&](const events::TutorialTriggerEvent& event){ return event.mTutorialName == tutorialName; }) != mActiveTutorials.cend();
}

///------------------------------------------------------------------------------------------------

void TutorialManager::LoadTutorialDefinitions()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto tutorialDefinitionJsonResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_DATA_ROOT + "tutorial_definitions.json", resources::DONT_RELOAD);
    const auto tutorialsJson =  nlohmann::json::parse(systemsEngine.GetResourceLoadingService().GetResource<resources::DataFileResource>(tutorialDefinitionJsonResourceId).GetContents());
    
    for (const auto& tutorialDefinitionObject: tutorialsJson["tutorial_definitions"])
    {
        strutils::StringId tutorialName = strutils::StringId(tutorialDefinitionObject["name"].get<std::string>());
        std::string tutorialDescription = tutorialDefinitionObject["description"].get<std::string>();
        bool showArrow = false;
        if (tutorialDefinitionObject.count("show_arrow"))
        {
            showArrow = tutorialDefinitionObject["show_arrow"].get<bool>();
        }
        
        mTutorialDefinitions.emplace(std::make_pair(tutorialName, TutorialDefinition(tutorialName, tutorialDescription, showArrow)));
    }
}

///------------------------------------------------------------------------------------------------

void TutorialManager::Update(const float dtMillis)
{
    if (!mActiveTutorials.empty())
    {
        // Tutorial active but not created yet. Create it.
        if (mTutorialSceneObjects.empty())
        {
            CreateTutorial();
        }
        // Tutorial active and created. Update it
        else
        {
            UpdateActiveTutorial(dtMillis);
        }
    }
}

///------------------------------------------------------------------------------------------------

void TutorialManager::CreateTutorial()
{
    const auto& tutorialDefinition = mTutorialDefinitions.at(mActiveTutorials.front().mTutorialName);
    
    // Add tutorial to be surfaced to perm. seen tutorials
    auto seenTutorials = DataRepository::GetInstance().GetSeenTutorials();
    seenTutorials.push_back(mActiveTutorials.front().mTutorialName);
    DataRepository::GetInstance().SetSeenTutorials(seenTutorials);
    DataRepository::GetInstance().FlushStateToFile();

    // Create custom scene
    auto tutorialScene = CoreSystemsEngine::GetInstance().GetSceneManager().CreateScene(game_constants::TUTORIAL_SCENE);
    tutorialScene->SetLoaded(true);
    
    auto tutorialSceneObject = tutorialScene->CreateSceneObject(TUTORIAL_BASE_SCENE_OBJECT_NAME);
    
    tutorialSceneObject->mPosition = TUTORIAL_BASE_POSITION;
    tutorialSceneObject->mScale = TUTORIAL_BASE_SCALE;
    tutorialSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TUTORIAL_TEXTURE_FILE_NAME);
    tutorialSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + TUTORIAL_SHADER_FILE_NAME);
    
    tutorialSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    tutorialSceneObject->mShaderFloatUniformValues[TUTORIAL_REVEAL_THRESHOLD_UNIFORM_NAME] = 0.0f;
    tutorialSceneObject->mShaderFloatUniformValues[TUTORIAL_REVEAL_RGB_EXPONENT_UNIFORM_NAME] = TUTORIAL_REVEAL_RGB_EXPONENT;
    
    mTutorialSceneObjects.push_back(tutorialSceneObject);
    
    auto tutorialTextRows = strutils::StringSplit(tutorialDefinition.mTutorialDescription, '$');
    tutorialTextRows.insert(tutorialTextRows.begin(), "Tutorials Enabled");
    tutorialTextRows.insert(tutorialTextRows.begin() + 1, "");
    
    // Tutorials checkbox
    {
        auto tutorialCheckboxSceneObject = tutorialScene->CreateSceneObject(TUTORIAL_TEXT_SCENE_OBJECT_NAMES[1]);
        tutorialCheckboxSceneObject->mScale = CHECKBOX_SCALE;
        tutorialCheckboxSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_EMPTY_TEXTURE_FILE_NAME);
        tutorialCheckboxSceneObject->mPosition = tutorialSceneObject->mPosition;
        tutorialCheckboxSceneObject->mPosition += TUTORIAL_TEXT_OFFSETS[1];
        tutorialCheckboxSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        tutorialCheckboxSceneObject->mBoundingRectMultiplier /= 2.0f;
        
        mTutorialSceneObjects.push_back(tutorialCheckboxSceneObject);
        SetCheckboxValue(true);
    }
    
    tutorialTextRows.resize(TUTORIAL_TEXT_ROWS_COUNT);
    
    for (auto i = 0U; i < tutorialTextRows.size(); ++i)
    {
        if (tutorialTextRows[i].empty())
        {
            continue;
        }
        
        auto tutorialTextSceneObject = tutorialScene->CreateSceneObject(TUTORIAL_TEXT_SCENE_OBJECT_NAMES[i]);
        tutorialTextSceneObject->mScale = TUTORIAL_TEXT_SCALE;
        tutorialTextSceneObject->mPosition = tutorialSceneObject->mPosition;
        tutorialTextSceneObject->mPosition += TUTORIAL_TEXT_OFFSETS[i];
        tutorialTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        
        scene::TextSceneObjectData textData;
        textData.mFontName = game_constants::DEFAULT_FONT_BLACK_NAME;
        textData.mText = tutorialTextRows[i];
        
        for (const auto& symbolicNameEntry: symbolic_glyph_names::SYMBOLIC_NAMES)
        {
            strutils::StringReplaceAllOccurences("<" + symbolicNameEntry.first.GetString() + ">", std::string(1, symbolicNameEntry.second), textData.mText);
        }
        
        tutorialTextSceneObject->mSceneObjectTypeData = std::move(textData);
        
        mTutorialSceneObjects.push_back(tutorialTextSceneObject);
    }
   
    // Continue button
    {
        mContinueButton = std::make_unique<AnimatedButton>
        (
            tutorialSceneObject->mPosition + TUTORIAL_TEXT_OFFSETS[8],
            TUTORIAL_TEXT_SCALE,
            game_constants::DEFAULT_FONT_BLACK_NAME,
            "Continue",
            TUTORIAL_TEXT_SCENE_OBJECT_NAMES[8],
            [=]()
            {
                auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
                animationManager.StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(TUTORIAL_DELETION_DELAY_SECS), [=]()
                {
                    DestroyTutorial();
                });
                FadeOutTutorial();
            },
            *tutorialScene
        );
        
        mContinueButton->GetSceneObject()->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        mTutorialSceneObjects.push_back(mContinueButton->GetSceneObject());
    }
    
    // Arrow
    {
        if (tutorialDefinition.mShowArrow)
        {
            auto arrowSceneObject = tutorialScene->CreateSceneObject(TUTORIAL_ARROW_SCENE_OBJECT_NAME);
            arrowSceneObject->mScale = ARROW_SCALE;
            arrowSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TUTORIAL_ARROW_TEXTURE_FILE_NAME);
            arrowSceneObject->mPosition.x = mActiveTutorials.front().mArrowOriginPosition.x;
            arrowSceneObject->mPosition.y = mActiveTutorials.front().mArrowOriginPosition.y;
            arrowSceneObject->mPosition.z = TUTORIAL_ARROW_Z;
            auto vecToTarget = mActiveTutorials.front().mArrowTargetPosition - arrowSceneObject->mPosition;
            vecToTarget.z = 0.0f;
            arrowSceneObject->mRotation.z = -math::Arctan2(vecToTarget.x, vecToTarget.y);
            arrowSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::BouncePositionAnimation>(arrowSceneObject, vecToTarget * TUTORIAL_ARROW_SPEED, TUTORIAL_ARROW_BOUNCE_DURATION_SECS, animation_flags::ANIMATE_CONTINUOUSLY), [](){});
            mTutorialSceneObjects.push_back(arrowSceneObject);
        }
    }
}

///------------------------------------------------------------------------------------------------

void TutorialManager::FadeOutTutorial()
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    for (auto sceneObject: mTutorialSceneObjects)
    {
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, TUTORIAL_FADE_OUT_DURATION_SECS), [=]()
        {
            sceneObject->mInvisible = true;
        });
    }
}

///------------------------------------------------------------------------------------------------

void TutorialManager::DestroyTutorial()
{
    CoreSystemsEngine::GetInstance().GetSceneManager().RemoveScene(game_constants::TUTORIAL_SCENE);
    mActiveTutorials.erase(mActiveTutorials.begin());
    
    // Suppress any other queued up tutorials
    if (!DataRepository::GetInstance().AreTutorialsEnabled())
    {
        mActiveTutorials.clear();
    }
    
    mTutorialSceneObjects.clear();
    mContinueButton = nullptr;
}

///------------------------------------------------------------------------------------------------

void TutorialManager::UpdateActiveTutorial(const float dtMillis)
{
    mTutorialSceneObjects[0]->mShaderFloatUniformValues[TUTORIAL_REVEAL_THRESHOLD_UNIFORM_NAME] += dtMillis * TUTORIAL_REVEAL_SPEED;
    if (mTutorialSceneObjects[0]->mShaderFloatUniformValues[TUTORIAL_REVEAL_THRESHOLD_UNIFORM_NAME] >= TUTORIAL_MAX_REVEAL_THRESHOLD)
    {
        mTutorialSceneObjects[0]->mShaderFloatUniformValues[TUTORIAL_REVEAL_THRESHOLD_UNIFORM_NAME] = TUTORIAL_MAX_REVEAL_THRESHOLD;
        
        for (auto i = 1U; i < mTutorialSceneObjects.size(); ++i)
        {
            auto tutorialTextSceneObject = mTutorialSceneObjects[i];
            tutorialTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = math::Min(1.0f, tutorialTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] +  dtMillis * TUTORIAL_TEXT_REVEAL_SPEED);
        }
        
        // Continue button interaction
        mContinueButton->Update(dtMillis);
        
        // Checkbox interaction
        auto tutorialScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::TUTORIAL_SCENE);
        const auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
        auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(tutorialScene->GetCamera().GetViewMatrix(), tutorialScene->GetCamera().GetProjMatrix());

        auto checkboxSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*tutorialScene->FindSceneObject(TUTORIAL_TEXT_SCENE_OBJECT_NAMES[1]));
        auto checkboxTextSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*tutorialScene->FindSceneObject(TUTORIAL_TEXT_SCENE_OBJECT_NAMES[1]));
        
        if
        (
            inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON) &&
            (math::IsPointInsideRectangle(checkboxSceneObjectRect.bottomLeft, checkboxSceneObjectRect.topRight, worldTouchPos) ||
            math::IsPointInsideRectangle(checkboxTextSceneObjectRect.bottomLeft, checkboxTextSceneObjectRect.topRight, worldTouchPos))
        )
        {
            ToggleCheckbox();
        }
    }
}

///------------------------------------------------------------------------------------------------

void TutorialManager::OnTutorialTrigger(const events::TutorialTriggerEvent& event)
{
    // Tutorials not active
    if (!DataRepository::GetInstance().AreTutorialsEnabled())
    {
        return;
    }
    
    // Tutorial seen already
    const auto& seenTutorials = DataRepository::GetInstance().GetSeenTutorials();
    if (std::find(seenTutorials.cbegin(), seenTutorials.cend(), event.mTutorialName) != seenTutorials.cend())
    {
        return;
    }
    
    // Tutorial already queued up
    if (IsTutorialActive(event.mTutorialName))
    {
        return;
    }
    
    // Tutorial definition not found
    if (mTutorialDefinitions.count(event.mTutorialName) == 0)
    {
        logging::Log(logging::LogType::ERROR, "Tried to surface unknown tutorial %s", event.mTutorialName.GetString().c_str());
        assert(false);
        return;
    }
    
    mActiveTutorials.push_back(event);
}

///------------------------------------------------------------------------------------------------

void TutorialManager::ToggleCheckbox()
{
    auto tutorialScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::TUTORIAL_SCENE);
    auto checkBoxSceneObject = tutorialScene->FindSceneObject(TUTORIAL_TEXT_SCENE_OBJECT_NAMES[1]);
    
    bool checkBoxValue = checkBoxSceneObject->mTextureResourceId == CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_FILLED_TEXTURE_FILE_NAME) ? false : true;
    SetCheckboxValue(checkBoxValue);
    
    DataRepository::GetInstance().SetTutorialsEnabled(checkBoxValue);
    DataRepository::GetInstance().FlushStateToFile();
}

///------------------------------------------------------------------------------------------------

void TutorialManager::SetCheckboxValue(const bool checkboxValue)
{
    resources::ResourceId checkboxFilledTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_FILLED_TEXTURE_FILE_NAME);
    resources::ResourceId checkboxEmptyTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_EMPTY_TEXTURE_FILE_NAME);
    
    auto tutorialScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::TUTORIAL_SCENE);
    auto checkBoxSceneObject = tutorialScene->FindSceneObject(TUTORIAL_TEXT_SCENE_OBJECT_NAMES[1]);
    checkBoxSceneObject->mTextureResourceId = checkboxValue ? checkboxFilledTextureResourceId : checkboxEmptyTextureResourceId;
}

///------------------------------------------------------------------------------------------------
