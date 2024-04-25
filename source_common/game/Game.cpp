///------------------------------------------------------------------------------------------------
///  Game.cpp                                                                                        
///  Predators
///
///  Created by Alex Koukoulas on 19/09/2023
///------------------------------------------------------------------------------------------------

#include <bitset>
#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/Camera.h>
#include <engine/rendering/Fonts.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <engine/utils/Date.h>
#include <engine/utils/Logging.h>
#include <engine/utils/FileUtils.h>
#include <engine/utils/MathUtils.h>
#include <engine/utils/OSMessageBox.h>
#include <engine/utils/PlatformMacros.h>
#include <fstream>
#include <game/AchievementManager.h>
#include <game/BoardState.h>
#include <game/Game.h>
#include <game/GameConstants.h>
#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/GameSceneTransitionManager.h>
#include <game/DataRepository.h>
#include <game/gameactions/BaseGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/GameActionFactory.h>
#include <game/GuiObjectManager.h>
#include <game/ProductRepository.h>
#include <game/scenelogicmanagers/AchievementsSceneLogicManager.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <game/scenelogicmanagers/BunnyHopSceneLogicManager.h>
#include <game/scenelogicmanagers/CardInspectionSceneLogicManager.h>
#include <game/scenelogicmanagers/CardLibrarySceneLogicManager.h>
#include <game/scenelogicmanagers/CardPackRewardSceneLogicManager.h>
#include <game/scenelogicmanagers/CardSelectionRewardSceneLogicManager.h>
#include <game/scenelogicmanagers/CloudDataConfirmationSceneLogicManager.h>
#include <game/scenelogicmanagers/CreditsSceneLogicManager.h>
#include <game/scenelogicmanagers/DefeatSceneLogicManager.h>
#include <game/scenelogicmanagers/DisconnectedSceneLogicManager.h>
#include <game/scenelogicmanagers/EventSceneLogicManager.h>
#include <game/scenelogicmanagers/FirstGameLockSceneLogicManager.h>
#include <game/scenelogicmanagers/GiftCodeClaimSceneLogicManager.h>
#include <game/scenelogicmanagers/InventorySceneLogicManager.h>
#include <game/scenelogicmanagers/LoadingSceneLogicManager.h>
#include <game/scenelogicmanagers/MainMenuSceneLogicManager.h>
#include <game/scenelogicmanagers/PrivacyPolicySceneLogicManager.h>
#include <game/scenelogicmanagers/PurchasingProductSceneLogicManager.h>
#include <game/scenelogicmanagers/ReleaseNotesSceneLogicManager.h>
#include <game/scenelogicmanagers/SettingsSceneLogicManager.h>
#include <game/scenelogicmanagers/ShopSceneLogicManager.h>
#include <game/scenelogicmanagers/StatsSceneLogicManager.h>
#include <game/scenelogicmanagers/StoryMapSceneLogicManager.h>
#include <game/scenelogicmanagers/UnseenSpellSceneLogicManager.h>
#include <game/scenelogicmanagers/VictorySceneLogicManager.h>
#include <game/scenelogicmanagers/VisitMapNodeSceneLogicManager.h>
#include <game/scenelogicmanagers/WheelOfFortuneSceneLogicManager.h>
#include <game/TutorialManager.h>

#if defined(MOBILE_FLOW)
#include <platform_specific/IOSUtils.h>
#endif

#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#include <platform_utilities/CloudKitUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static const strutils::StringId MAIN_MENU_SCENE = strutils::StringId("main_menu_scene");

///------------------------------------------------------------------------------------------------

Game::Game(const int argc, char** argv)
{
    if (argc > 0)
    {
        logging::Log(logging::LogType::INFO, "Initializing from CWD : %s", argv[0]);
    }
    
#if defined(MACOS) || defined(MOBILE_FLOW)
    apple_utils::SetAssetFolder();
#endif
    CardDataRepository::GetInstance().LoadCardData(false);
    ProductRepository::GetInstance().LoadProductDefinitions();
    
    CoreSystemsEngine::GetInstance().Start([&](){ Init(); }, [&](const float dtMillis){ Update(dtMillis); }, [&](){ ApplicationMovedToBackground(); }, [&](){ WindowResize(); }, [&](){ CreateDebugWidgets(); }, [&](){ OnOneSecondElapsed(); });
}

///------------------------------------------------------------------------------------------------

Game::~Game(){}

///------------------------------------------------------------------------------------------------

void Game::Init()
{
    DataRepository::GetInstance();
    
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    systemsEngine.GetFontRepository().LoadFont(game_constants::DEFAULT_FONT_NAME.GetString(), resources::ResourceReloadMode::DONT_RELOAD);
    systemsEngine.GetFontRepository().LoadFont(game_constants::DEFAULT_FONT_BLACK_NAME.GetString(), resources::ResourceReloadMode::DONT_RELOAD);
    systemsEngine.GetFontRepository().LoadFont(game_constants::FONT_PLACEHOLDER_DAMAGE_NAME.GetString(), resources::ResourceReloadMode::DONT_RELOAD);
    systemsEngine.GetFontRepository().LoadFont(game_constants::FONT_PLACEHOLDER_WEIGHT_NAME.GetString(), resources::ResourceReloadMode::DONT_RELOAD);
    
    systemsEngine.GetSoundManager().SetAudioEnabled(DataRepository::GetInstance().IsAudioEnabled());
    
    auto& eventSystem = events::EventSystem::GetInstance();
    mSceneChangeEventListener = eventSystem.RegisterForEvent<events::SceneChangeEvent>([=](const events::SceneChangeEvent& event)
    {
        mGameSceneTransitionManager->ChangeToScene(event.mNewSceneName, event.mSceneChangeType, event.mPreviousSceneDestructionType);
    });
    
    mPopModalSceneEventListener = eventSystem.RegisterForEvent<events::PopSceneModalEvent>([=](const events::PopSceneModalEvent&)
    {
        mGameSceneTransitionManager->PopModalScene();
    });
    
    mRequestReviewEventListener = eventSystem.RegisterForEvent<events::TriggerRequestReviewEvent>([=](const events::TriggerRequestReviewEvent&)
    {
#if defined(MACOS) || defined(MOBILE_FLOW)
        apple_utils::RequestReview();
#endif
    });
    
    mSendPlayMessageEventListener = eventSystem.RegisterForEvent<events::SendPlayMessageEvent>([=](const events::SendPlayMessageEvent&)
    {
#if defined(MACOS) || defined(MOBILE_FLOW)
        nlohmann::json playMessageJson;
        
        using namespace date;
        std::stringstream dateNow;
        dateNow << std::chrono::system_clock::now();
        
        playMessageJson["datetime"] = dateNow.str();
        playMessageJson["gold"] = DataRepository::GetInstance().CurrencyCoins().GetValue();
        playMessageJson["mutation_level"] = DataRepository::GetInstance().GetCurrentStoryMutationLevel();
        
#if defined(MACOS)
        playMessageJson["platform"] = "MacOS";
#endif
        
#if defined(MOBILE_FLOW)
        playMessageJson["platform"] = "iOS";
#endif
        playMessageJson["game_version"] = apple_utils::GetAppVersion();
        playMessageJson["normal_map"] = DataRepository::GetInstance().GetCurrentStoryMapType() != StoryMapType::TUTORIAL_MAP ? "true" : "false";
        playMessageJson["story_coords"] = std::to_string(DataRepository::GetInstance().GetCurrentStoryMapNodeCoord().x) + "," + std::to_string(DataRepository::GetInstance().GetCurrentStoryMapNodeCoord().y);
        apple_utils::SendPlayMessage(playMessageJson);
#endif
    });
                                                                                         
    
    mTutorialManager = std::make_unique<TutorialManager>();
    mTutorialManager->LoadTutorialDefinitions();
    
    AchievementManager::GetInstance().LoadAchievementDefinitions();
    
    mGameSceneTransitionManager = std::make_unique<GameSceneTransitionManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<AchievementsSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<BattleSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<BunnyHopSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<CardInspectionSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<CardLibrarySceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<CardPackRewardSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<CardSelectionRewardSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<CloudDataConfirmationSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<CreditsSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<DefeatSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<DisconnectedSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<EventSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<FirstGameLockSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<GiftCodeClaimSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<InventorySceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<LoadingSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<MainMenuSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<PrivacyPolicySceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<PurchasingProductSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<ReleaseNotesSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<SettingsSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<ShopSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<StatsSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<StoryMapSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<UnseenSpellSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<VictorySceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<VisitMapNodeSceneLogicManager>();
    mGameSceneTransitionManager->RegisterSceneLogicManager<WheelOfFortuneSceneLogicManager>();
    
#if defined(MOBILE_FLOW)
    if (ios_utils::IsIPad())
    {
        game_constants::GAME_BOARD_BASED_SCENE_ZOOM_FACTOR = 120.0f;
        game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR = 2.45f;
        game_constants::IN_GAME_MOBILE_ONLY_FREE_MOVING_CARD_Y_OFFSET = 0.025f;
    }
    else
    {
        game_constants::GAME_BOARD_BASED_SCENE_ZOOM_FACTOR = 130.0f;
        game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR = 2.16666f;
        game_constants::IN_GAME_MOBILE_ONLY_FREE_MOVING_CARD_Y_OFFSET = 0.05f;
    }
#else
    game_constants::GAME_BOARD_BASED_SCENE_ZOOM_FACTOR = 120.0f;
    game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR = 2.0f;
#endif

    mGameSceneTransitionManager->ChangeToScene(MAIN_MENU_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
}

///------------------------------------------------------------------------------------------------

void Game::Update(const float dtMillis)
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto cardPackRewardScene = sceneManager.FindScene(game_constants::CARD_PACK_REWARD_SCENE);
    
    // Cloud Data Sync
    if
    (
        DataRepository::GetInstance().CanSurfaceCloudDataScene() &&
        DataRepository::GetInstance().GetForeignProgressionDataFound() != ForeignCloudDataFoundType::NONE &&
        mGameSceneTransitionManager->GetActiveSceneStack().top().mActiveSceneName == MAIN_MENU_SCENE &&
        !sceneManager.FindScene(game_constants::LOADING_SCENE) &&
        !animationManager.IsAnimationPlaying(game_constants::OVERLAY_DARKENING_ANIMATION_NAME)
    )
    {
        mGameSceneTransitionManager->ChangeToScene(game_constants::CLOUD_DATA_CONFIRMATION_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
    }
    // Pending Card Packs
    else if
    (
        !DataRepository::GetInstance().GetPendingCardPacks().empty() &&
        mGameSceneTransitionManager->GetActiveSceneStack().top().mActiveSceneName == MAIN_MENU_SCENE &&
        (!cardPackRewardScene || !cardPackRewardScene->FindSceneObject(game_constants::OVERLAY_SCENE_OBJECT_NAME)) &&
        !sceneManager.FindScene(game_constants::LOADING_SCENE) &&
        !animationManager.IsAnimationPlaying(game_constants::OVERLAY_DARKENING_ANIMATION_NAME)
    )
    {
        mGameSceneTransitionManager->ChangeToScene(game_constants::CARD_PACK_REWARD_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
    }

    mTutorialManager->Update(dtMillis);
    AchievementManager::GetInstance().Update(dtMillis, mGameSceneTransitionManager->GetActiveSceneLogicManager()->VGetGuiObjectManager());
    mGameSceneTransitionManager->Update(dtMillis);
}

///------------------------------------------------------------------------------------------------

void Game::ApplicationMovedToBackground()
{
    DataRepository::GetInstance().FlushStateToFile();
    events::EventSystem::GetInstance().DispatchEvent<events::ApplicationMovedToBackgroundEvent>();
}

///------------------------------------------------------------------------------------------------

void Game::OnOneSecondElapsed()
{
    if (DataRepository::GetInstance().IsCurrentlyPlayingStoryMode())
    {
        DataRepository::GetInstance().SetCurrentStorySecondPlayed(DataRepository::GetInstance().GetCurrentStorySecondsPlayed() + 1);
        DataRepository::GetInstance().SetTotalSecondsPlayed(DataRepository::GetInstance().GetTotalSecondsPlayed() + 1);
    }
}

///------------------------------------------------------------------------------------------------

void Game::WindowResize()
{
    events::EventSystem::GetInstance().DispatchEvent<events::WindowResizeEvent>();
}

///------------------------------------------------------------------------------------------------

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #include <imgui/backends/imgui_impl_sdl2.h>
    #define CREATE_DEBUG_WIDGETS
#elif __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
        #undef CREATE_DEBUG_WIDGETS
    #else
        #include <imgui/backends/imgui_impl_sdl2.h>
        #define CREATE_DEBUG_WIDGETS
    #endif
#endif

#if ((!defined(NDEBUG)) || defined(IMGUI_IN_RELEASE)) && (defined(CREATE_DEBUG_WIDGETS))
static void CreateImGuiCardVecEntry(const std::string& cardIdPrefix, std::string& cardVec, const std::vector<CardStatOverrides>& cardOverrides, const effects::EffectBoardModifierMask boardModifierMask, const bool forRemotePlayer, const bool forBoardCards)
{
    cardVec.erase(cardVec.begin());
    cardVec.erase(cardVec.end() - 1);
    auto splitByNewLine = strutils::StringSplit(cardVec, ',');
    
    if (!forBoardCards)
    {
        std::reverse(splitByNewLine.begin(), splitByNewLine.end());
    }
    
    ImGui::Text("[");
    ImGui::SameLine();
    for (size_t i = 0; i < splitByNewLine.size(); ++i)
    {
        auto cardData = CardDataRepository::GetInstance().GetCardData(std::stoi(splitByNewLine[i]), forRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX);
        ImGui::PushID((cardIdPrefix + std::to_string(i)).c_str());
        ImGui::Text((i == 0 ? "%s" : ",%s"), splitByNewLine[i].c_str());
        
        if (cardData.IsSpell())
        {
            ImGui::SetItemTooltip("(Name: %s, Family: %s, Effect: %s, Weight: %d)",
                                  cardData.mCardName.GetString().c_str(),
                                  cardData.mCardFamily.GetString().c_str(),
                                  cardData.mCardEffect.c_str(),
                                  cardData.mCardWeight);
        }
        else
        {
            ImGui::SetItemTooltip("(Name: %s, Family: %s, Damage: %d, Weight: %d)",
                                  cardData.mCardName.GetString().c_str(),
                                  cardData.mCardFamily.GetString().c_str(),
                                  cardData.mCardDamage,
                                  cardData.mCardWeight);
        }
        
        ImGui::PopID();
        ImGui::SameLine();
    }
    ImGui::Text("]");
    ImGui::NewLine();
    
    if (!cardOverrides.empty())
    {
        std::stringstream overridesString;
        overridesString << "[";
        
        auto cardOverridesCopy = cardOverrides;
        if (!forBoardCards)
        {
            std::reverse(cardOverridesCopy.begin(), cardOverridesCopy.end());
        }
        
        for (size_t i = 0; i < cardOverridesCopy.size(); ++i)
        {
            if (i != 0)
            {
                overridesString << ", ";
            }
            
            overridesString << (forBoardCards ? i : cardOverridesCopy.size() - 1 - i) << ":{";
            bool hasSeenFirstInnerEntry = false;
            for (const auto& statOverrideEntry: cardOverridesCopy[i])
            {
                if (hasSeenFirstInnerEntry)
                {
                    overridesString << ", ";
                }
                else
                {
                    hasSeenFirstInnerEntry = true;
                }
                
                switch (statOverrideEntry.first)
                {
                    case CardStatType::DAMAGE: overridesString << "DAMAGE="; break;
                    case CardStatType::WEIGHT: overridesString << "WEIGHT="; break;
                }
                
                overridesString << statOverrideEntry.second;
            }
            
            overridesString << "}";
        }
        overridesString << "}";
        ImGui::Text("Overrides: %s", overridesString.str().c_str());
    }
    
    if (boardModifierMask > 0)
    {
        std::stringstream maskString;
        maskString << std::bitset<32>(boardModifierMask);
        ImGui::Text("Board Modifier Mask: %s", maskString.str().c_str());
    }
}

void Game::CreateDebugWidgets()
{
    // Create game configs
    static bool printGameActionTransitions = false;
    
    ImGui::Begin("Scene Transitions", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::SeparatorText("Active Scene Stack");
    auto activeSceneStack = mGameSceneTransitionManager->GetActiveSceneStack();
    while (!activeSceneStack.empty())
    {
        const auto& topEntry = activeSceneStack.top();
        ImGui::Text("%s: %s", typeid(*topEntry.mActiveSceneLogicManager).name(), topEntry.mActiveSceneName.GetString().c_str());
        activeSceneStack.pop();
    }
    
    ImGui::SeparatorText("Scene Logic Managers");
    const auto& registeredSceneManagers = mGameSceneTransitionManager->GetRegisteredSceneLogicManagers();
    for (const auto& sceneManagerEntry: registeredSceneManagers)
    {
        auto* sceneLogicManager = sceneManagerEntry.mSceneLogicManager.get();
        std::stringstream initStatusStr;
        for (const auto& initStatusEntry: sceneManagerEntry.mSceneInitStatusMap)
        {
            if (!initStatusStr.str().empty())
            {
                initStatusStr << ", ";
            }
            initStatusStr << initStatusEntry.first.GetString() << ":" << (initStatusEntry.second ? "true" : "false");
        }
        ImGui::Text("%s: [%s]", typeid(*sceneLogicManager).name(), initStatusStr.str().c_str());
    }
    ImGui::End();
    
    // Playground
//    ImGui::Begin("Playground", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
//    ImGui::SliderFloat("Pitch", &PITCH, 0.0f, 4.0f);
//    ImGui::SliderFloat("Gain", &GAIN, 0.0f, 2.0f);
//    ImGui::SliderInt("Frequency", &FREQUENCY, 40000, 65000);
//    if (ImGui::Button("Play Sound"))
//    {
//        CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx("sfx_attack_light");
//        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound("sfx_attack_light");
//    }
//    ImGui::End();
    ImGui::Begin("Card Data Export", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    {
        static size_t selectedExpansionIndex = 0;
        static std::vector<strutils::StringId> expansionIds;
        if (expansionIds.empty())
        {
            for (const auto& expansion: CardDataRepository::GetInstance().GetCardExpansions())
            {
                expansionIds.push_back(expansion.second.mExpansionId);
            }
        }
        
        ImGui::Text("Expansion");
        ImGui::SameLine();
        ImGui::PushID("Expansions");
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::BeginCombo(" ", expansionIds.at(selectedExpansionIndex).GetString().c_str()))
        {
            for (size_t n = 0U; n < expansionIds.size(); n++)
            {
                const bool isSelected = (selectedExpansionIndex == n);
                if (ImGui::Selectable(expansionIds.at(n).GetString().c_str(), isSelected))
                {
                    selectedExpansionIndex = n;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
        ImGui::SameLine();
        if (ImGui::Button("Export Cards"))
        {
            card_utils::ExportCardData(expansionIds.at(selectedExpansionIndex), CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(mGameSceneTransitionManager->GetActiveSceneStack().top().mActiveSceneName));
        }
    }
    
    ImGui::End();
    
    // Manipulating Story Data
    ImGui::Begin("Story Data", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    {
        ImGui::SeparatorText("Cards");
        static size_t selectedCardIndex = 0;
        static std::vector<std::pair<std::string, int>> cardNamesAndIds;
        if (cardNamesAndIds.empty())
        {
            auto allCardIds = CardDataRepository::GetInstance().GetAllCardIds();
            for (auto cardId: allCardIds)
            {
                const auto& cardData = CardDataRepository::GetInstance().GetCardData(cardId, 0);
                cardNamesAndIds.emplace_back(std::make_pair(cardData.mCardName.GetString(), cardId));
            }
        }
        
        if (ImGui::BeginCombo(" ", cardNamesAndIds.at(selectedCardIndex).first.c_str()))
        {
            for (size_t n = 0U; n < cardNamesAndIds.size(); n++)
            {
                const bool isSelected = (selectedCardIndex == n);
                if (ImGui::Selectable(cardNamesAndIds.at(n).first.c_str(), isSelected))
                {
                    selectedCardIndex = n;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Add Card to Deck"))
        {
            auto currentStoryDeck = DataRepository::GetInstance().GetCurrentStoryPlayerDeck();
            currentStoryDeck.push_back(cardNamesAndIds.at(selectedCardIndex).second);
            DataRepository::GetInstance().SetCurrentStoryPlayerDeck(currentStoryDeck);
            DataRepository::GetInstance().FlushStateToFile();
        }
    }
    {
        static size_t artifactIndex = 0;
        static std::vector<strutils::StringId> artifactNames = ProductRepository::GetInstance().GetRareItemProductNames();
        
        ImGui::SeparatorText("Artifacts");
        ImGui::PushID("AddArtifact");
        if (ImGui::BeginCombo(" ", artifactNames.at(artifactIndex).GetString().c_str()))
        {
            for (size_t n = 0U; n < artifactNames.size(); n++)
            {
                const bool isSelected = (artifactIndex == n);
                if (ImGui::Selectable(artifactNames.at(n).GetString().c_str(), isSelected))
                {
                    artifactIndex = n;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
        ImGui::SameLine();
        if (ImGui::Button("Add Artifact"))
        {
            static const std::string RARE_ITEM_SHADER = "rare_item.vs";
            static const glm::vec3 RARE_ITEM_TARGET_SCALE = glm::vec3(0.3f, 0.3f, 0.3f);
            
            const auto& rareItemDefinition = ProductRepository::GetInstance().GetProductDefinition(artifactNames.at(artifactIndex));
            
            auto activeScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(mGameSceneTransitionManager->GetActiveSceneStack().top().mActiveSceneName);
            auto activeGui = mGameSceneTransitionManager->GetActiveSceneLogicManager()->VGetGuiObjectManager();
            
            if (activeGui)
            {
                auto rareItemSceneObject = activeScene->CreateSceneObject();
                rareItemSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + RARE_ITEM_SHADER);
                rareItemSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + std::get<std::string>(rareItemDefinition.mProductTexturePathOrCardId));
                rareItemSceneObject->mPosition = {0.0f, 0.0f, 5.0f};
                rareItemSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
                rareItemSceneObject->mScale = {0.1f, 0.1f, 0.1f};
                
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(rareItemSceneObject, rareItemSceneObject->mPosition, RARE_ITEM_TARGET_SCALE, 1.0f), [=]()
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::RareItemCollectedEvent>(artifactNames.at(artifactIndex), rareItemSceneObject);
                });
                
            }
            else
            {
                DataRepository::GetInstance().AddStoryArtifact(artifactNames.at(artifactIndex));
            }
            
            DataRepository::GetInstance().FlushStateToFile();
        }
    }
    
    {
        static size_t artifactIndex = 0;
        static std::vector<strutils::StringId> artifactNames = ProductRepository::GetInstance().GetRareItemProductNames();
        
        ImGui::PushID("RemoveArtifact");
        if (ImGui::BeginCombo(" ", artifactNames.at(artifactIndex).GetString().c_str()))
        {
            for (size_t n = 0U; n < artifactNames.size(); n++)
            {
                const bool isSelected = (artifactIndex == n);
                if (ImGui::Selectable(artifactNames.at(n).GetString().c_str(), isSelected))
                {
                    artifactIndex = n;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
        ImGui::SameLine();
        if (ImGui::Button("Remove Artifact"))
        {
            auto currentStoryArtifacts = DataRepository::GetInstance().GetCurrentStoryArtifacts();
            currentStoryArtifacts.erase(std::remove_if(currentStoryArtifacts.begin(), currentStoryArtifacts.end(), [=](const std::pair<strutils::StringId, int>& artifactEntry)
            {
                return artifactEntry.first == artifactNames.at(artifactIndex);
            }), currentStoryArtifacts.end());
            DataRepository::GetInstance().SetCurrentStoryArtifacts(currentStoryArtifacts);
            DataRepository::GetInstance().FlushStateToFile();
        }
    }
    
    ImGui::End();
    
    // Manipulating Persistent Data
    ImGui::Begin("Persistent Data", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    
    static std::string achievementName;
    if (achievementName.empty())
    {
        achievementName.resize(30);
    }
    
    ImGui::SeparatorText("Achievements");
    
    {
        static size_t selectedAchievementNameIndex = 0;
        static std::vector<std::string> achievementNames;
        if (achievementNames.empty())
        {
            auto achievementDefinitions = AchievementManager::GetInstance().GetAchievementDefinitions();
            for (const auto& achievementDefinition: achievementDefinitions)
            {
                achievementNames.push_back(achievementDefinition.first.GetString());
            }
        }
        
        {
            ImGui::Text("Achievement Name");
            ImGui::SameLine();
            ImGui::PushID("UnlockAchievement");
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::BeginCombo(" ", achievementNames.at(selectedAchievementNameIndex).c_str()))
            {
                for (size_t n = 0U; n < achievementNames.size(); n++)
                {
                    const bool isSelected = (selectedAchievementNameIndex == n);
                    if (ImGui::Selectable(achievementNames.at(n).c_str(), isSelected))
                    {
                        selectedAchievementNameIndex = n;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopID();
            ImGui::SameLine();
            if (ImGui::Button("Unlock Achievement"))
            {
                auto achievementNameId = strutils::StringId(achievementNames.at(selectedAchievementNameIndex));
                auto unlnockedAchievements = DataRepository::GetInstance().GetUnlockedAchievements();
                unlnockedAchievements.erase(std::remove(unlnockedAchievements.begin(), unlnockedAchievements.end(), achievementNameId), unlnockedAchievements.end());
                DataRepository::GetInstance().SetUnlockedAchievements(unlnockedAchievements);
                events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievementNameId);
            }
        }
    }
    
    {
        static size_t selectedAchievementNameIndex = 0;
        static std::vector<std::string> achievementNames;
        if (achievementNames.empty())
        {
            auto achievementDefinitions = AchievementManager::GetInstance().GetAchievementDefinitions();
            for (const auto& achievementDefinition: achievementDefinitions)
            {
                achievementNames.push_back(achievementDefinition.first.GetString());
            }
        }
        
        {
            ImGui::Text("Achievement Name");
            ImGui::SameLine();
            ImGui::PushID("ClearUnlockedAchievement");
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::BeginCombo(" ", achievementNames.at(selectedAchievementNameIndex).c_str()))
            {
                for (size_t n = 0U; n < achievementNames.size(); n++)
                {
                    const bool isSelected = (selectedAchievementNameIndex == n);
                    if (ImGui::Selectable(achievementNames.at(n).c_str(), isSelected))
                    {
                        selectedAchievementNameIndex = n;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopID();
            ImGui::SameLine();
            if (ImGui::Button("Reset Achievement"))
            {
                auto achievementNameId = strutils::StringId(achievementNames.at(selectedAchievementNameIndex));
                auto unlnockedAchievements = DataRepository::GetInstance().GetUnlockedAchievements();
                unlnockedAchievements.erase(std::remove(unlnockedAchievements.begin(), unlnockedAchievements.end(), achievementNameId), unlnockedAchievements.end());
                DataRepository::GetInstance().SetUnlockedAchievements(unlnockedAchievements);
                DataRepository::GetInstance().FlushStateToFile();
            }
        }
    }
    
    if (ImGui::Button("Clear Unlocked Achievements"))
    {
        DataRepository::GetInstance().SetUnlockedAchievements({});
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    static std::string tutorialName;
    if (tutorialName.empty())
    {
        tutorialName.resize(30);
    }
    
    ImGui::SeparatorText("Tutorials");
    
    {
        static size_t selectedTutorialNameIndex = 0;
        static std::vector<std::string> tutorialNames;
        if (tutorialNames.empty())
        {
            auto tutorialDefinitions = mTutorialManager->GetTutorialDefinitions();
            for (const auto& tutorialDefinition: tutorialDefinitions)
            {
                tutorialNames.push_back(tutorialDefinition.first.GetString());
            }
        }
        
        {
            ImGui::Text("Tutorial Name");
            ImGui::SameLine();
            ImGui::PushID("ShowTutorial");
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::BeginCombo(" ", tutorialNames.at(selectedTutorialNameIndex).c_str()))
            {
                for (size_t n = 0U; n < tutorialNames.size(); n++)
                {
                    const bool isSelected = (selectedTutorialNameIndex == n);
                    if (ImGui::Selectable(tutorialNames.at(n).c_str(), isSelected))
                    {
                        selectedTutorialNameIndex = n;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopID();
            ImGui::SameLine();
            if (ImGui::Button("Show Tutorial"))
            {
                auto tutorialNameId = strutils::StringId(tutorialNames.at(selectedTutorialNameIndex));
                auto seenTutorials = DataRepository::GetInstance().GetSeenTutorials();
                seenTutorials.erase(std::remove(seenTutorials.begin(), seenTutorials.end(), tutorialNameId), seenTutorials.end());
                DataRepository::GetInstance().SetSeenTutorials(seenTutorials);
                events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorialNameId);
            }
        }
    }
    
    {
        static size_t selectedTutorialToResetNameIndex = 0;
        static std::vector<std::string> tutorialNamesToReset;
        if (tutorialNamesToReset.empty())
        {
            auto tutorialDefinitions = mTutorialManager->GetTutorialDefinitions();
            for (const auto& tutorialDefinition: tutorialDefinitions)
            {
                tutorialNamesToReset.push_back(tutorialDefinition.first.GetString());
            }
        }
        
        {
            ImGui::Text("Tutorial Name");
            ImGui::SameLine();
            ImGui::PushID("ClearTutorial");
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::BeginCombo(" ", tutorialNamesToReset.at(selectedTutorialToResetNameIndex).c_str()))
            {
                for (size_t n = 0U; n < tutorialNamesToReset.size(); n++)
                {
                    const bool isSelected = (selectedTutorialToResetNameIndex == n);
                    if (ImGui::Selectable(tutorialNamesToReset.at(n).c_str(), isSelected))
                    {
                        selectedTutorialToResetNameIndex = n;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopID();
            ImGui::SameLine();
            if (ImGui::Button("Reset Tutorial"))
            {
                auto tutorialNameId = strutils::StringId(tutorialNamesToReset.at(selectedTutorialToResetNameIndex));
                auto seenTutorials = DataRepository::GetInstance().GetSeenTutorials();
                seenTutorials.erase(std::remove(seenTutorials.begin(), seenTutorials.end(), tutorialNameId), seenTutorials.end());
                DataRepository::GetInstance().SetSeenTutorials(seenTutorials);
                DataRepository::GetInstance().FlushStateToFile();
            }
        }
    }
    
    if (ImGui::Button("Clear Seen Tutorials"))
    {
        DataRepository::GetInstance().SetSeenTutorials({});
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    static std::string mutationLevelString; mutationLevelString.resize(3);
    static std::string victoriesString; victoriesString.resize(4);
    static std::string bestTimeSecondsString; bestTimeSecondsString.resize(6);
    
    {
        ImGui::SeparatorText("Game Stats");
        ImGui::PushID("MutationLevel");
        ImGui::Text("Mutation Level");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(30.0f);
        ImGui::InputText("##hidelabel", &mutationLevelString[0], mutationLevelString.size());
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID("Victories");
        ImGui::Text("Victories");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40.0f);
        ImGui::InputText("##hidelabel", &victoriesString[0], victoriesString.size());
        ImGui::PopID();
        
        ImGui::SameLine();
        if (ImGui::Button("Set Victories"))
        {
            auto mutationLevel = std::stoi(mutationLevelString);
            auto victoryCount = std::stoi(victoriesString);
            DataRepository::GetInstance().SetMutationLevelVictories(mutationLevel, victoryCount);
            DataRepository::GetInstance().FlushStateToFile();
        }
    }
    
    {
        ImGui::PushID("MutationLevelBestTime");
        ImGui::Text("Mutation Level");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(30.0f);
        ImGui::InputText("##hidelabel", &mutationLevelString[0], mutationLevelString.size());
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID("BestTimeSecs");
        ImGui::Text("Best Time Secs");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputText("##hidelabel", &bestTimeSecondsString[0], bestTimeSecondsString.size());
        ImGui::PopID();
        
        ImGui::SameLine();
        if (ImGui::Button("Set Best Time"))
        {
            auto mutationLevel = std::stoi(mutationLevelString);
            auto bestTimeSecs = std::stoi(bestTimeSecondsString);
            DataRepository::GetInstance().SetMutationLevelBestTime(mutationLevel, bestTimeSecs);
            DataRepository::GetInstance().FlushStateToFile();
        }
    }
    
    if (ImGui::Button("Clear All Victories"))
    {
        for (auto i = 0; i <= game_constants::MAX_MUTATION_LEVEL; ++i)
        {
            DataRepository::GetInstance().SetMutationLevelVictories(i, 0);
        }
        
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    
    static std::string gamesFinishedString; gamesFinishedString.resize(4);
    ImGui::PushID("GamesFinished");
    ImGui::Text("Games");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(40.0f);
    ImGui::InputText("##hidelabel", &gamesFinishedString[0], gamesFinishedString.size());
    ImGui::PopID();
    ImGui::SameLine();
    if (ImGui::Button("Set Games Finished"))
    {
        auto gamesFinishedCount = std::stoi(gamesFinishedString);
        DataRepository::GetInstance().SetGamesFinishedCount(gamesFinishedCount);
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    if (ImGui::Button("Clear Games Finished"))
    {
        DataRepository::GetInstance().SetGamesFinishedCount(0);
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    static size_t selectedCardIndex = 0;
    static std::vector<std::pair<std::string, int>> cardNamesAndIds;
    if (cardNamesAndIds.empty())
    {
        auto allCardIds = CardDataRepository::GetInstance().GetAllCardIds();
        for (auto cardId: allCardIds)
        {
            const auto& cardData = CardDataRepository::GetInstance().GetCardData(cardId, 0);
            cardNamesAndIds.emplace_back(std::make_pair(cardData.mCardName.GetString(), cardId));
        }
    }
    
    ImGui::SeparatorText("Cards");
    if (ImGui::Button("Unlock All Cards"))
    {
        auto rodentCards = CardDataRepository::GetInstance().GetCardIdsByFamily(game_constants::RODENTS_FAMILY_NAME);
        auto dinosaurCards = CardDataRepository::GetInstance().GetCardIdsByFamily(game_constants::DINOSAURS_FAMILY_NAME);
        auto insectCards = CardDataRepository::GetInstance().GetCardIdsByFamily(game_constants::INSECTS_FAMILY_NAME);
        
        auto unlockedCards = DataRepository::GetInstance().GetUnlockedCardIds();
        
        for (auto cardId: rodentCards)
        {
            if (std::find(unlockedCards.cbegin(), unlockedCards.cend(), cardId) == unlockedCards.cend())
            {
                unlockedCards.push_back(cardId);
            }
        }
        
        for (auto cardId: dinosaurCards)
        {
            if (std::find(unlockedCards.cbegin(), unlockedCards.cend(), cardId) == unlockedCards.cend())
            {
                unlockedCards.push_back(cardId);
            }
        }
        
        for (auto cardId: insectCards)
        {
            if (std::find(unlockedCards.cbegin(), unlockedCards.cend(), cardId) == unlockedCards.cend())
            {
                unlockedCards.push_back(cardId);
            }
        }
        
        DataRepository::GetInstance().SetUnlockedCardIds(unlockedCards);
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    ImGui::PushID("CardUnlock");
    if (ImGui::BeginCombo(" ", cardNamesAndIds.at(selectedCardIndex).first.c_str()))
    {
        for (size_t n = 0U; n < cardNamesAndIds.size(); n++)
        {
            const bool isSelected = (selectedCardIndex == n);
            if (ImGui::Selectable(cardNamesAndIds.at(n).first.c_str(), isSelected))
            {
                selectedCardIndex = n;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    
    ImGui::SameLine();
    if (ImGui::Button("Unlock Card"))
    {
        auto unlockedCardIds = DataRepository::GetInstance().GetUnlockedCardIds();
        unlockedCardIds.push_back(cardNamesAndIds.at(selectedCardIndex).second);
        DataRepository::GetInstance().SetUnlockedCardIds(unlockedCardIds);
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    if (ImGui::Button("Clear Unlocked & Golden Cards"))
    {
        DataRepository::GetInstance().ClearGoldenCardIdMap();
        DataRepository::GetInstance().SetUnlockedCardIds(CardDataRepository::GetInstance().GetFreshAccountUnlockedCardIds());
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    if (ImGui::Button("Clear Transaction IDs"))
    {
        DataRepository::GetInstance().SetSuccessfulTransactionIds({});
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    ImGui::SeparatorText("Coins");
    static int coinAmount = 0;
    ImGui::SliderInt("Coin Value", &coinAmount, -1000, 1000);
    ImGui::SameLine();
    if (ImGui::Button("Add Coins"))
    {
        DataRepository::GetInstance().CurrencyCoins().SetValue(DataRepository::GetInstance().CurrencyCoins().GetValue() + coinAmount);
        DataRepository::GetInstance().CurrencyCoins().SetDisplayedValue(DataRepository::GetInstance().CurrencyCoins().GetValue());
        DataRepository::GetInstance().FlushStateToFile();
        
        if (mGameSceneTransitionManager->GetActiveSceneLogicManager()->VGetGuiObjectManager())
        {
            mGameSceneTransitionManager->GetActiveSceneLogicManager()->VGetGuiObjectManager()->ResetDisplayedCurrencyCoins();
        }
    }
    
    if (ImGui::Button("Reset Coins"))
    {
        DataRepository::GetInstance().CurrencyCoins().SetValue(0);
        DataRepository::GetInstance().FlushStateToFile();
        
        if (mGameSceneTransitionManager->GetActiveSceneLogicManager()->VGetGuiObjectManager())
        {
            mGameSceneTransitionManager->GetActiveSceneLogicManager()->VGetGuiObjectManager()->ResetDisplayedCurrencyCoins();
        }
    }
    
    static size_t cardPackIndex = 0;
    static std::vector<std::pair<std::string, CardPackType>> cardPackNamesAndTypes =
    {
        { "None", CardPackType::NONE },
        { "Normal", CardPackType::NORMAL },
        { "Golden", CardPackType::GOLDEN }
    };
    
    ImGui::SeparatorText("Card Packs");
    ImGui::PushID("CardPacks");
    if (ImGui::BeginCombo(" ", cardPackNamesAndTypes.at(cardPackIndex).first.c_str()))
    {
        for (size_t n = 0U; n < cardPackNamesAndTypes.size(); n++)
        {
            const bool isSelected = (cardPackIndex == n);
            if (ImGui::Selectable(cardPackNamesAndTypes.at(n).first.c_str(), isSelected))
            {
                cardPackIndex = n;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    
    ImGui::SameLine();
    if (ImGui::Button("Add Pack"))
    {
        DataRepository::GetInstance().AddPendingCardPack(cardPackNamesAndTypes.at(cardPackIndex).second);
        DataRepository::GetInstance().FlushStateToFile();
    }
    
    if (ImGui::Button("Clear Pending Card Packs"))
    {
        while (!DataRepository::GetInstance().GetPendingCardPacks().empty())
        {
            DataRepository::GetInstance().PopFrontPendingCardPack();
        }
        DataRepository::GetInstance().FlushStateToFile();
    }
    ImGui::End();
    
    
    auto* activeSceneLogicManager = mGameSceneTransitionManager->GetActiveSceneLogicManager();
    activeSceneLogicManager->VCreateDebugWidgets();
    
    if (mGameSceneTransitionManager->GetActiveSceneStack().top().mActiveSceneName == game_constants::STORY_MAP_SCENE)
    {
        static EventSceneLogicManager dummyEventSceneLogicManager;
        if (dummyEventSceneLogicManager.GetRegisteredEvents().empty())
        {
            dummyEventSceneLogicManager.SelectRandomStoryEvent(true);
        }
        
        const auto& registeredEvents = dummyEventSceneLogicManager.GetRegisteredEvents();
        
        // Events Widget
        ImGui::Begin("Events", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
        
        {
            static size_t selectedEventNameIndex = 0;
            static std::vector<std::string> eventNames;
            if (eventNames.empty())
            {
                for (const auto& eventData: registeredEvents)
                {
                    eventNames.push_back(eventData.mEventName.GetString());
                }
            }
            
            {
                ImGui::Text("Event Name");
                ImGui::SameLine();
                ImGui::PushID("EventName");
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::BeginCombo(" ", eventNames.at(selectedEventNameIndex).c_str()))
                {
                    for (size_t n = 0U; n < eventNames.size(); n++)
                    {
                        const bool isSelected = (selectedEventNameIndex == n);
                        if (ImGui::Selectable(eventNames.at(n).c_str(), isSelected))
                        {
                            selectedEventNameIndex = n;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopID();
                ImGui::SameLine();
                if (ImGui::Button("Show Event"))
                {
                    DataRepository::GetInstance().SetCurrentEventIndex(static_cast<int>(selectedEventNameIndex));
                    DataRepository::GetInstance().SetCurrentEventScreenIndex(0);
                    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::EVENT_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
                }
            }
        }
        ImGui::End();
    }
    
    if (!dynamic_cast<BattleSceneLogicManager*>(activeSceneLogicManager))
    {
        return;
    }
    // Battle specific ImGui windows
    auto& battleSceneLogicManager = *(dynamic_cast<BattleSceneLogicManager*>(activeSceneLogicManager));
    printGameActionTransitions = battleSceneLogicManager.GetActionEngine().LoggingActionTransitions();
    ImGui::Begin("Game Runtime", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::SeparatorText("General");
    ImGui::Checkbox("Print Action Transitions", &printGameActionTransitions);
    battleSceneLogicManager.GetActionEngine().SetLoggingActionTransitions(printGameActionTransitions);
    ImGui::End();
    
    // Create Card State Viewer
    ImGui::Begin("Card State Viewer", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    static std::string cardStateName;
    const auto& localPlayerCardSoWrappers = battleSceneLogicManager.GetHeldCardSoWrappers();
    for (size_t i = 0; i < localPlayerCardSoWrappers.size(); ++i)
    {
        ImGui::SeparatorText(i == 0 ? "Remote Player" : "Local Player");
        for (size_t j = 0; j < localPlayerCardSoWrappers[i].size(); ++j)
        {
            switch (localPlayerCardSoWrappers[i][j]->mState)
            {
                case CardSoState::MOVING_TO_SET_POSITION: cardStateName = "MOVING_TO_SET_POSITION"; break;
                case CardSoState::IDLE:                   cardStateName = "IDLE"; break;
                case CardSoState::HIGHLIGHTED:            cardStateName = "HIGHLIGHTED"; break;
                case CardSoState::FREE_MOVING:            cardStateName = "FREE_MOVING"; break;
            }
            
            ImGui::Text("%d Card State: %s", static_cast<int>(j), cardStateName.c_str());
        }
    }
    
    ImGui::End();
    
    // Create action generator
    ImGui::Begin("Action Generator & Board State", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    const auto& actions = GameActionFactory::GetRegisteredActions();
    
    static size_t currentIndex = 0;
    static std::string activePlayerIndex = "";
    static std::string remotePlayerStats = "";
    static std::string remotePlayerHand = "";
    static std::string remotePlayerBoard = "";
    static std::string localPlayerStats = "";
    static std::string localPlayerBoard = "";
    static std::string localPlayerHand = "";
    static std::unordered_map<std::string, std::string> actionExtraParams;
    
    if (ImGui::BeginCombo(" ", actions.at(currentIndex).GetString().c_str()))
    {
        for (size_t n = 0U; n < actions.size(); n++)
        {
            const bool isSelected = (currentIndex == n);
            if (ImGui::Selectable(actions.at(n).GetString().c_str(), isSelected))
            {
                currentIndex = n;
                actionExtraParams.clear();
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(3 / 7.0f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(3 / 7.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(3 / 7.0f, 0.8f, 0.8f));
    if (ImGui::Button("Create"))
    {
        battleSceneLogicManager.GetActionEngine().AddGameAction(strutils::StringId(actions.at(currentIndex)), actionExtraParams);
        battleSceneLogicManager.GetActionEngine().Update(1);
    }
    
    // Hacky way of getting all action required params
    auto requiredGameActionExtraParams = GameActionFactory::CreateGameAction(actions.at(currentIndex))->VGetRequiredExtraParamNames();
    for (size_t i = 0; i < requiredGameActionExtraParams.size(); ++i)
    {
        const auto& requiredExtraParam = requiredGameActionExtraParams.at(i);
        ImGui::PushID(("RequiredExtraParam" + std::to_string(i)).c_str());
        if (actionExtraParams.count(requiredExtraParam) == 0)
        {
            actionExtraParams[requiredExtraParam].resize(128);
        }
        
        ImGui::Text("%s", requiredExtraParam.c_str());
        ImGui::SameLine();
        ImGui::InputText("##hidelabel", &actionExtraParams[requiredExtraParam][0], actionExtraParams[requiredExtraParam].size());
        ImGui::PopID();
    }
    
    const auto& boardState = battleSceneLogicManager.GetBoardState();
    activePlayerIndex = std::to_string(boardState.GetActivePlayerIndex());
    remotePlayerStats = "Health: " + std::to_string(boardState.GetPlayerStates().front().mPlayerHealth) +
                       " | Total Weight Ammo: " + std::to_string(boardState.GetPlayerStates().front().mPlayerTotalWeightAmmo) +
                       " | Current Weight Ammo: " + std::to_string(boardState.GetPlayerStates().front().mPlayerCurrentWeightAmmo);
    remotePlayerHand = strutils::VecToString(boardState.GetPlayerStates().front().mPlayerHeldCards);
    remotePlayerBoard = strutils::VecToString(boardState.GetPlayerStates().front().mPlayerBoardCards);
    localPlayerBoard = strutils::VecToString(boardState.GetPlayerStates().back().mPlayerBoardCards);
    localPlayerHand = strutils::VecToString(boardState.GetPlayerStates().back().mPlayerHeldCards);
    localPlayerStats = "Health: " + std::to_string(boardState.GetPlayerStates().back().mPlayerHealth) +
                    " | Total Weight Ammo: " + std::to_string(boardState.GetPlayerStates().back().mPlayerTotalWeightAmmo) +
                    " | Current Weight Ammo: " + std::to_string(boardState.GetPlayerStates().back().mPlayerCurrentWeightAmmo);
    
    ImGui::PopStyleColor(3);
    ImGui::SeparatorText("Output");
    ImGui::TextWrapped("Turn Counter %d", boardState.GetTurnCounter());
    ImGui::TextWrapped("Active Player %s", activePlayerIndex.c_str());
    ImGui::SeparatorText("Remote Player Stats");
    ImGui::TextWrapped("%s", remotePlayerStats.c_str());
    ImGui::SeparatorText("Remote Player Hand");
    CreateImGuiCardVecEntry("RemotePlayerHand", remotePlayerHand, boardState.GetPlayerStates()[0].mPlayerHeldCardStatOverrides, 0, true, false);
    ImGui::SeparatorText("Remote Player Board");
    CreateImGuiCardVecEntry("RemotePlayerBoard", remotePlayerBoard, boardState.GetPlayerStates()[0].mPlayerBoardCardStatOverrides, boardState.GetPlayerStates()[0].mBoardModifiers.mBoardModifierMask, true, true);
    ImGui::SeparatorText("Local Player Board");
    CreateImGuiCardVecEntry("LocalPlayerBoard", localPlayerBoard, boardState.GetPlayerStates()[1].mPlayerBoardCardStatOverrides, boardState.GetPlayerStates()[1].mBoardModifiers.mBoardModifierMask, false, true);
    ImGui::SeparatorText("Local Player Hand");
    CreateImGuiCardVecEntry("LocalPlayerHand", localPlayerHand, boardState.GetPlayerStates()[1].mPlayerHeldCardStatOverrides, 0, false, false);
    ImGui::SeparatorText("Local Player Stats");
    ImGui::TextWrapped("%s", localPlayerStats.c_str());
    ImGui::End();
}
#else
void Game::CreateDebugWidgets()
{
}
#endif

///------------------------------------------------------------------------------------------------
