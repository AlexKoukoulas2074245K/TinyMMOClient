///------------------------------------------------------------------------------------------------
///  EventSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 28/12/2023
///------------------------------------------------------------------------------------------------

#ifndef EventSceneLogicManager_h
#define EventSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>
#include <functional>
#include <vector>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class GuiObjectManager;
class EventSceneLogicManager final: public ISceneLogicManager, public events::IListener
{
    friend class Game;
public:
    EventSceneLogicManager();
    ~EventSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    void RegisterForEvents();
    void OnWindowResize(const events::WindowResizeEvent& event);
    void SelectRandomStoryEvent(const bool debugDeterministic = false);
    void TransitionToEventScreen(const int screenIndex);
    void CreateEventScreen(const int screenIndex);
    void CollectRareItem(const strutils::StringId& rareItemName);
    void AnimateAndDeleteArtifact(const strutils::StringId& artifactName);
    void AnimateAndAddCardToDeck(const int cardRewardId);
    void AnimateAndDeleteCardFromDeck(const int deckCardIndex, const bool unlockBlockingAtEndOfDeletion);
    
private:
    class StoryRandomEventButtonData
    {
    public:
        StoryRandomEventButtonData(const std::string& buttonText, const int nextScreenIndex, const float nextScreenDelaySecs = 0.0f, const std::function<void()> onClickCallback = nullptr)
            : mButtonText(buttonText)
            , mNextScreenIndex(nextScreenIndex)
            , mNextScreenDelaySecs(nextScreenDelaySecs)
            , mOnClickCallback(onClickCallback)
        {
        }
        
        const std::string mButtonText;
        const int mNextScreenIndex;
        const float mNextScreenDelaySecs;
        const std::function<void()> mOnClickCallback;
    };
    
    class StoryRandomEventScreenData
    {
    public:
        StoryRandomEventScreenData
        (
            const std::string& eventScreenPortraitTextureFilename,
            const std::vector<std::string>& eventScreenDescriptionSentences,
            const std::vector<StoryRandomEventButtonData>& eventScreenButtons
        )
            : mEventScreenPortraitTextureFilename(eventScreenPortraitTextureFilename)
            , mEventScreenDescriptionSentences(eventScreenDescriptionSentences)
            , mEventScreenButtons(eventScreenButtons)
        {
        }
        
        const std::string mEventScreenPortraitTextureFilename;
        const std::vector<std::string> mEventScreenDescriptionSentences;
        const std::vector<StoryRandomEventButtonData> mEventScreenButtons;
    };
    
    class StoryRandomEventData
    {
    public:
        StoryRandomEventData(const strutils::StringId& eventName, const std::vector<StoryRandomEventScreenData>& eventScreens, std::function<bool()> applicabilityFunction)
            : mEventName(eventName)
            , mEventScreens(eventScreens)
            , mApplicabilityFunction(applicabilityFunction)
        {
        }
    
        const std::vector<StoryRandomEventScreenData> mEventScreens;
        const std::function<bool()> mApplicabilityFunction;
        const strutils::StringId mEventName;
    };
    
private:
    const std::vector<StoryRandomEventData>& GetRegisteredEvents() const;
    
private:
    std::vector<std::unique_ptr<AnimatedButton>> mCurrentEventButtons;
    std::vector<StoryRandomEventData> mRegisteredStoryEvents;
    std::shared_ptr<GuiObjectManager> mGuiManager;
    std::shared_ptr<scene::Scene> mScene;
    std::shared_ptr<CardSoWrapper> mCardSoWrapper;
    int mCurrentEventIndex;
    int mCurrentEventScreenIndex;
    bool mTransitioning;
    bool mBlockInteraction;
    bool mHasSentTutorialTrigger;
    bool mSkipNormalEventFinishingSceneChange;
};

///------------------------------------------------------------------------------------------------

#endif /* EventSceneLogicManager_h */
