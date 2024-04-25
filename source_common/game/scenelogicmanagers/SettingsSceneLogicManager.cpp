///------------------------------------------------------------------------------------------------
///  SettingsSceneLogicManager.cpp
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
#include <game/Cards.h>
#include <game/DataRepository.h>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/SettingsSceneLogicManager.h>
#include <SDL_events.h>

///------------------------------------------------------------------------------------------------

static const std::string SELECTABLE_BUTTON_SHADER_FILE_NAME = "basic_custom_color.vs";
static const std::string DECK_ENTRY_SHADER = "card_family_selection_swipe_entry.vs";
static const std::string DECK_ENTRY_MASK_TEXTURE_FILE_NAME = "trap_mask.png";
static const std::string CHECKBOX_EMPTY_TEXTURE_FILE_NAME = "checkbox_empty.png";
static const std::string CHECKBOX_FILLED_TEXTURE_FILE_NAME = "checkbox_filled.png";

static const strutils::StringId MAIN_MENU_SCENE = strutils::StringId("main_menu_scene");
static const strutils::StringId CONTINUE_BUTTON_NAME = strutils::StringId("continue_button");
static const strutils::StringId QUIT_BUTTON_NAME = strutils::StringId("quit_button");
static const strutils::StringId PAUSED_TEXT_SCENE_OBJECT_NAME = strutils::StringId("paused_text");
static const strutils::StringId MUSIC_CHECKBOX_SCENE_OBJECT_NAME = strutils::StringId("music_checkbox");
static const strutils::StringId MUSIC_CHECKBOX_TEXT_SCENE_OBJECT_NAME = strutils::StringId("music_checkbox_text");
static const strutils::StringId TUTORIALS_CHECKBOX_SCENE_OBJECT_NAME = strutils::StringId("tutorials_checkbox");
static const strutils::StringId TUTORIALS_CHECKBOX_TEXT_SCENE_OBJECT_NAME = strutils::StringId("tutorials_checkbox_text");
static const strutils::StringId QUIT_CONFIRMATION_BUTTON_NAME = strutils::StringId("quit_confirmation");
static const strutils::StringId QUIT_CANCELLATION_BUTTON_NAME = strutils::StringId("quit_cancellation");
static const strutils::StringId QUIT_CONFIRMATION_TEXT_TOP_NAME = strutils::StringId("quit_confirmation_text_top");
static const strutils::StringId QUIT_CONFIRMATION_TEXT_BOT_NAME = strutils::StringId("quit_confirmation_text_bot");

static const glm::vec3 TUTORIALS_CHECKBOX_TEXT_POSITION = {-0.111f, 0.1f, 23.1f};
static const glm::vec3 TUTORIALS_CHECKBOX_POSITION = {0.077f, 0.087f, 23.1f};
static const glm::vec3 MUSIC_CHECKBOX_TEXT_POSITION = {-0.085f, 0.028f, 23.1f};
static const glm::vec3 MUSIC_CHECKBOX_POSITION = {0.04f, 0.018f, 23.1f};
static const glm::vec3 BUTTON_SCALE = {0.0005f, 0.0005f, 0.0005f};
static const glm::vec3 CONTINUE_BUTTON_POSITION = {-0.091f, -0.044f, 23.1f};
static const glm::vec3 QUIT_BUTTON_POSITION = {-0.041f, -0.116f, 23.1f};
static const glm::vec3 QUIT_CONFIRMATION_BUTTON_POSITION = {-0.132f, -0.083f, 23.1f};
static const glm::vec3 QUIT_CANCELLATION_BUTTON_POSITION = {0.036f, -0.083f, 23.1f};
static const glm::vec3 QUIT_CONFIRMATION_TEXT_TOP_POSITION = {-0.225f, 0.07f, 23.1f};
static const glm::vec3 QUIT_CONFIRMATION_TEXT_BOT_POSITION = {-0.32f, 0.019f, 23.1f};
static const glm::vec3 CHECKBOX_SCALE = {0.1f, 0.1f, 0.1f};

static const float SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.1f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    game_constants::SETTINGS_SCENE
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    PAUSED_TEXT_SCENE_OBJECT_NAME,
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& SettingsSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

SettingsSceneLogicManager::SettingsSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

SettingsSceneLogicManager::~SettingsSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void SettingsSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void SettingsSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mActiveSubScene = SubSceneType::NONE;
    mTransitioningToSubScene = false;
    InitSubScene(SubSceneType::MAIN, scene);
}

///------------------------------------------------------------------------------------------------

void SettingsSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    if (mTransitioningToSubScene)
    {
        return;
    }
    
    const auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
    auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(scene->GetCamera().GetViewMatrix(), scene->GetCamera().GetProjMatrix());

    auto musicCheckboxSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*scene->FindSceneObject(MUSIC_CHECKBOX_SCENE_OBJECT_NAME));
    auto musicCheckboxTextSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*scene->FindSceneObject(MUSIC_CHECKBOX_TEXT_SCENE_OBJECT_NAME));
    
    if
    (
        inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON) &&
        (math::IsPointInsideRectangle(musicCheckboxSceneObjectRect.bottomLeft, musicCheckboxSceneObjectRect.topRight, worldTouchPos) ||
        math::IsPointInsideRectangle(musicCheckboxTextSceneObjectRect.bottomLeft, musicCheckboxTextSceneObjectRect.topRight, worldTouchPos))
    )
    {
        ToggleMusicCheckbox(scene);
    }
    
    auto tutorialsCheckboxSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*scene->FindSceneObject(TUTORIALS_CHECKBOX_SCENE_OBJECT_NAME));
    auto tutorialsCheckboxTextSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*scene->FindSceneObject(TUTORIALS_CHECKBOX_TEXT_SCENE_OBJECT_NAME));
    
    if
    (
        inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON) &&
        (math::IsPointInsideRectangle(tutorialsCheckboxSceneObjectRect.bottomLeft, tutorialsCheckboxSceneObjectRect.topRight, worldTouchPos) ||
        math::IsPointInsideRectangle(tutorialsCheckboxTextSceneObjectRect.bottomLeft, tutorialsCheckboxTextSceneObjectRect.topRight, worldTouchPos))
    )
    {
        ToggleTutorialsCheckbox(scene);
    }
    
    // Animated buttons
    for (auto& animatedButton: mAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void SettingsSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
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
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto previousScene = sceneManager.FindScene(mPreviousScene);
    
    animationManager.StopAnimation(game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenValueAnimation>(previousScene->GetUpdateTimeSpeedFactor(), 1.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> SettingsSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void SettingsSceneLogicManager::InitSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene)
{
    if (mActiveSubScene == subSceneType)
    {
        return;
    }
    
    mActiveSubScene = subSceneType;
    
    scene->RemoveAllSceneObjectsButTheOnesNamed(STATIC_SCENE_ELEMENTS);
    mAnimatedButtons.clear();
    
    switch (subSceneType)
    {
        case SubSceneType::MAIN:
        {
            // Tutorials checkbox
            auto tutorialsCheckboxSceneObject = scene->CreateSceneObject(TUTORIALS_CHECKBOX_SCENE_OBJECT_NAME);
            tutorialsCheckboxSceneObject->mPosition = TUTORIALS_CHECKBOX_POSITION;
            tutorialsCheckboxSceneObject->mScale = CHECKBOX_SCALE;
            tutorialsCheckboxSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_EMPTY_TEXTURE_FILE_NAME);
            tutorialsCheckboxSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            tutorialsCheckboxSceneObject->mInvisible = false;
            tutorialsCheckboxSceneObject->mBoundingRectMultiplier /= 2.0f;
            
            scene::TextSceneObjectData tutorialsCheckboxTextData;
            tutorialsCheckboxTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
            tutorialsCheckboxTextData.mText = "Tutorials";
            
            auto tutorialsCheckboxTextSceneObject = scene->CreateSceneObject(TUTORIALS_CHECKBOX_TEXT_SCENE_OBJECT_NAME);
            tutorialsCheckboxTextSceneObject->mSceneObjectTypeData = std::move(tutorialsCheckboxTextData);
            tutorialsCheckboxTextSceneObject->mPosition = TUTORIALS_CHECKBOX_TEXT_POSITION;
            tutorialsCheckboxTextSceneObject->mScale = BUTTON_SCALE;
            tutorialsCheckboxTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            tutorialsCheckboxTextSceneObject->mInvisible = false;
            
            SetTutorialsCheckboxValue(scene, DataRepository::GetInstance().AreTutorialsEnabled());
            
            // Music checkbox
            auto musicCheckboxSceneObject = scene->CreateSceneObject(MUSIC_CHECKBOX_SCENE_OBJECT_NAME);
            musicCheckboxSceneObject->mPosition = MUSIC_CHECKBOX_POSITION;
            musicCheckboxSceneObject->mScale = CHECKBOX_SCALE;
            musicCheckboxSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_EMPTY_TEXTURE_FILE_NAME);
            musicCheckboxSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            musicCheckboxSceneObject->mInvisible = false;
            musicCheckboxSceneObject->mBoundingRectMultiplier /= 2.0f;
            
            scene::TextSceneObjectData musicCheckboxTextData;
            musicCheckboxTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
            musicCheckboxTextData.mText = "Music";
            
            auto musicCheckboxTextSceneObject = scene->CreateSceneObject(MUSIC_CHECKBOX_TEXT_SCENE_OBJECT_NAME);
            musicCheckboxTextSceneObject->mSceneObjectTypeData = std::move(musicCheckboxTextData);
            musicCheckboxTextSceneObject->mPosition = MUSIC_CHECKBOX_TEXT_POSITION;
            musicCheckboxTextSceneObject->mScale = BUTTON_SCALE;
            musicCheckboxTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            musicCheckboxTextSceneObject->mInvisible = false;
            
            SetMusicCheckboxValue(scene, DataRepository::GetInstance().IsAudioEnabled());
            
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
                    mTransitioningToSubScene = true;
                },
                *scene
            ));
            
            if (mPreviousScene != MAIN_MENU_SCENE)
            {
                mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
                (
                    QUIT_BUTTON_POSITION,
                    BUTTON_SCALE,
                    game_constants::DEFAULT_FONT_NAME,
                    "Quit",
                    QUIT_BUTTON_NAME,
                    [=]()
                    {
                        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::MAIN_MENU_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
                    },
                    *scene
                ));
            }
        } break;
            
        case SubSceneType::QUIT_CONFIRMATION:
        {
            scene::TextSceneObjectData textDataQuitTop;
            textDataQuitTop.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataQuitTop.mText = "Are you sure you want to quit?";
            auto textQuitTopSceneObject = scene->CreateSceneObject(QUIT_CONFIRMATION_TEXT_TOP_NAME);
            textQuitTopSceneObject->mSceneObjectTypeData = std::move(textDataQuitTop);
            textQuitTopSceneObject->mPosition = QUIT_CONFIRMATION_TEXT_TOP_POSITION;
            textQuitTopSceneObject->mScale = BUTTON_SCALE;
            
            scene::TextSceneObjectData textDataQuitBot;
            textDataQuitBot.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataQuitBot.mText = "Any active battle progress will be lost.";
            auto textQuitBotSceneObject = scene->CreateSceneObject(QUIT_CONFIRMATION_TEXT_BOT_NAME);
            textQuitBotSceneObject->mSceneObjectTypeData = std::move(textDataQuitBot);
            textQuitBotSceneObject->mPosition = QUIT_CONFIRMATION_TEXT_BOT_POSITION;
            textQuitBotSceneObject->mScale = BUTTON_SCALE;
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                QUIT_CONFIRMATION_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Yes",
                QUIT_CONFIRMATION_BUTTON_NAME,
                [=]()
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::MAIN_MENU_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
                },
                *scene
            ));
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                QUIT_CANCELLATION_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Cancel",
                QUIT_CANCELLATION_BUTTON_NAME,
                [=]()
                {
                    TransitionToSubScene(SubSceneType::MAIN, scene);
                },
                *scene
            ));
        } break;
            
        default: break;
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
            mTransitioningToSubScene = false;
        });
    }
}

///------------------------------------------------------------------------------------------------

void SettingsSceneLogicManager::TransitionToSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene)
{
    mTransitioningToSubScene = true;
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (STATIC_SCENE_ELEMENTS.count(sceneObject->mName))
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

void SettingsSceneLogicManager::ToggleMusicCheckbox(std::shared_ptr<scene::Scene> scene)
{
    auto checkBoxSceneObject = scene->FindSceneObject(MUSIC_CHECKBOX_SCENE_OBJECT_NAME);
    
    bool checkBoxValue = checkBoxSceneObject->mTextureResourceId == CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_FILLED_TEXTURE_FILE_NAME) ? false : true;
    SetMusicCheckboxValue(scene, checkBoxValue);
    
    DataRepository::GetInstance().SetAudioEnabled(checkBoxValue);
    DataRepository::GetInstance().FlushStateToFile();
    CoreSystemsEngine::GetInstance().GetSoundManager().SetAudioEnabled(checkBoxValue);
}

///------------------------------------------------------------------------------------------------

void SettingsSceneLogicManager::SetMusicCheckboxValue(std::shared_ptr<scene::Scene> scene, const bool checkboxValue)
{
    resources::ResourceId checkboxFilledTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_FILLED_TEXTURE_FILE_NAME);
    resources::ResourceId checkboxEmptyTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_EMPTY_TEXTURE_FILE_NAME);
    
    auto checkBoxSceneObject = scene->FindSceneObject(MUSIC_CHECKBOX_SCENE_OBJECT_NAME);
    checkBoxSceneObject->mTextureResourceId = checkboxValue ? checkboxFilledTextureResourceId : checkboxEmptyTextureResourceId;
}

///------------------------------------------------------------------------------------------------

void SettingsSceneLogicManager::ToggleTutorialsCheckbox(std::shared_ptr<scene::Scene> scene)
{
    auto checkBoxSceneObject = scene->FindSceneObject(TUTORIALS_CHECKBOX_SCENE_OBJECT_NAME);
    
    bool checkBoxValue = checkBoxSceneObject->mTextureResourceId == CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_FILLED_TEXTURE_FILE_NAME) ? false : true;
    SetTutorialsCheckboxValue(scene, checkBoxValue);
    
    DataRepository::GetInstance().SetTutorialsEnabled(checkBoxValue);
    DataRepository::GetInstance().FlushStateToFile();
}

///------------------------------------------------------------------------------------------------

void SettingsSceneLogicManager::SetTutorialsCheckboxValue(std::shared_ptr<scene::Scene> scene, const bool checkboxValue)
{
    resources::ResourceId checkboxFilledTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_FILLED_TEXTURE_FILE_NAME);
    resources::ResourceId checkboxEmptyTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_EMPTY_TEXTURE_FILE_NAME);
    
    auto checkBoxSceneObject = scene->FindSceneObject(TUTORIALS_CHECKBOX_SCENE_OBJECT_NAME);
    checkBoxSceneObject->mTextureResourceId = checkboxValue ? checkboxFilledTextureResourceId : checkboxEmptyTextureResourceId;
}

///------------------------------------------------------------------------------------------------
