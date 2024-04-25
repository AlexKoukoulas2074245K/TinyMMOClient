///------------------------------------------------------------------------------------------------
///  CloudDataConfirmationSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 22/01/2024
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/sound/SoundManager.h>
#include <fstream>
#include <game/AnimatedButton.h>
#include <game/Cards.h>
#include <game/events/EventSystem.h>
#include <game/DataRepository.h>
#include <game/scenelogicmanagers/CloudDataConfirmationSceneLogicManager.h>
#include <SDL_events.h>
#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#include <platform_utilities/CloudKitUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static const strutils::StringId USE_CLOUD_DATA_BUTTON_NAME = strutils::StringId("use_cloud_data_button");
static const strutils::StringId USE_LOCAL_DATA_BUTTON_NAME = strutils::StringId("use_local_data_button");
static const strutils::StringId OPTIONAL_CLOUD_DATA_DEVICE_NAME_AND_TIME_TEXT_SCENE_OBJECT_NAME = strutils::StringId("cloud_data_optional_text_1");
static const strutils::StringId MANDATORY_CLOUD_DATA_DEVICE_NAME_AND_TIME_TEXT_SCENE_OBJECT_NAME = strutils::StringId("cloud_data_mandatory_text_1");

static const std::string OPTIONAL_CLOUD_DATA_TEXT_SCENE_OBJECT_NAME_PREFIX = "cloud_data_optional_";
static const std::string MANDATORY_CLOUD_DATA_TEXT_SCENE_OBJECT_NAME_PREFIX = "cloud_data_mandatory_";

static const glm::vec3 BUTTON_SCALE = {0.00045, 0.00045, 0.00045};
static const glm::vec3 OK_BUTTON_POSITION = {-0.083f, -0.1f, 23.1};
static const glm::vec3 USE_CLOUD_DATA_BUTTON_POSITION = {-0.131f, -0.09f, 23.1f};
static const glm::vec3 USE_LOCAL_DATA_BUTTON_POSITION = {-0.151f, -0.175f, 23.1f};

static const float SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.1f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    game_constants::CLOUD_DATA_CONFIRMATION_SCENE
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& CloudDataConfirmationSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

CloudDataConfirmationSceneLogicManager::CloudDataConfirmationSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

CloudDataConfirmationSceneLogicManager::~CloudDataConfirmationSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void CloudDataConfirmationSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void CloudDataConfirmationSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mTransitioningToSubScene = false;
    
    // Update cloud data text and re-orient
    auto cloudDataUsageType = DataRepository::GetInstance().GetForeignProgressionDataFound();
    auto cloudDataText = DataRepository::GetInstance().GetCloudDataDeviceNameAndTime();
    auto cloudDataTextSceneObject = scene->FindSceneObject(cloudDataUsageType == ForeignCloudDataFoundType::OPTIONAL ? OPTIONAL_CLOUD_DATA_DEVICE_NAME_AND_TIME_TEXT_SCENE_OBJECT_NAME : MANDATORY_CLOUD_DATA_DEVICE_NAME_AND_TIME_TEXT_SCENE_OBJECT_NAME);
    std::get<scene::TextSceneObjectData>(cloudDataTextSceneObject->mSceneObjectTypeData).mText = cloudDataText;
    auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*cloudDataTextSceneObject);
    auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
    cloudDataTextSceneObject->mPosition.x -= textLength/2.0f;
    
    mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        cloudDataUsageType == ForeignCloudDataFoundType::OPTIONAL ? USE_CLOUD_DATA_BUTTON_POSITION : OK_BUTTON_POSITION,
        BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        cloudDataUsageType == ForeignCloudDataFoundType::OPTIONAL ? "Use Cloud Data" : "Continue",
        USE_CLOUD_DATA_BUTTON_NAME,
        [=]()
        {
            OnUseCloudDataButtonPressed();
            events::EventSystem::GetInstance().DispatchEvent<events::PopSceneModalEvent>();
            mTransitioningToSubScene = true;
        },
        *scene
    ));
    
    if (cloudDataUsageType == ForeignCloudDataFoundType::OPTIONAL)
    {
        mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
        (
            USE_LOCAL_DATA_BUTTON_POSITION,
            BUTTON_SCALE,
            game_constants::DEFAULT_FONT_NAME,
            "Keep Local Data",
            USE_LOCAL_DATA_BUTTON_NAME,
            [=]()
            {
                events::EventSystem::GetInstance().DispatchEvent<events::PopSceneModalEvent>();
                mTransitioningToSubScene = true;
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
        
        if (cloudDataUsageType == ForeignCloudDataFoundType::OPTIONAL && strutils::StringStartsWith(sceneObject->mName.GetString(), MANDATORY_CLOUD_DATA_TEXT_SCENE_OBJECT_NAME_PREFIX))
        {
            sceneObject->mInvisible = true;
        }
        else if (cloudDataUsageType == ForeignCloudDataFoundType::MANDATORY && strutils::StringStartsWith(sceneObject->mName.GetString(), OPTIONAL_CLOUD_DATA_TEXT_SCENE_OBJECT_NAME_PREFIX))
        {
            sceneObject->mInvisible = true;
        }
        
        sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=]()
        {
            mTransitioningToSubScene = false;
        });
    }
    
    DataRepository::GetInstance().SetForeignProgressionDataFound(ForeignCloudDataFoundType::NONE);
}

///------------------------------------------------------------------------------------------------

void CloudDataConfirmationSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene>)
{
    if (mTransitioningToSubScene)
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

void CloudDataConfirmationSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
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

std::shared_ptr<GuiObjectManager> CloudDataConfirmationSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void CloudDataConfirmationSceneLogicManager::OnUseCloudDataButtonPressed()
{
#if defined(MACOS) || defined(MOBILE_FLOW)
    auto checkAndReplacePersistentDataFile = [](const std::string& dataFileNameWithoutExtension)
    {
        std::string dataFileExtension = ".json";
        
        auto cloudFilePath = apple_utils::GetPersistentDataDirectoryPath() + "cloud_" + dataFileNameWithoutExtension + dataFileExtension;
        auto filePath = apple_utils::GetPersistentDataDirectoryPath() + dataFileNameWithoutExtension + dataFileExtension;
        
        std::ifstream cloudFile(cloudFilePath);
        if (cloudFile.is_open())
        {
            std::stringstream buffer;
            buffer << cloudFile.rdbuf();
            auto contents = buffer.str();
            
            std::ofstream dataFile(filePath);
            dataFile << contents;
            dataFile.close();
        }
        
        cloudFile.close();
        std::remove(cloudFilePath.c_str());
    };
    
    checkAndReplacePersistentDataFile("persistent");
    checkAndReplacePersistentDataFile("story");
    checkAndReplacePersistentDataFile("last_battle");
    
    DataRepository::GetInstance().ReloadProgressionDataFromFile();
    CoreSystemsEngine::GetInstance().GetSoundManager().SetAudioEnabled(DataRepository::GetInstance().IsAudioEnabled());
    DataRepository::GetInstance().FlushStateToFile();
#endif
}

///------------------------------------------------------------------------------------------------
