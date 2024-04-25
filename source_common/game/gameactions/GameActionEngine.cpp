///------------------------------------------------------------------------------------------------
///  GameActionEngine.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/09/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/utils/Logging.h>
#include <engine/utils/MathUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/GameActionFactory.h>
#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId IDLE_GAME_ACTION_NAME = strutils::StringId("IdleGameAction");

///------------------------------------------------------------------------------------------------

GameActionEngine::GameActionEngine(const EngineOperationMode operationMode, const int gameSeed, BoardState* boardState, BattleSceneLogicManager* battleSceneLogicManager, GameRuleEngine* gameRuleEngine)
    : mOperationMode(operationMode)
    , mGameSeed(gameSeed)
    , mBoardState(boardState)
    , mBattleSceneLogicManager(battleSceneLogicManager)
    , mGameRuleEngine(gameRuleEngine)
    , mActiveActionHasSetState(false)
    , mLoggingActionTransitions(false)
{
    math::SetControlSeed(mGameSeed);
    
    GameActionFactory::RegisterGameActions();
    
    CreateAndPushGameAction(IDLE_GAME_ACTION_NAME, {});
}

///------------------------------------------------------------------------------------------------

GameActionEngine::~GameActionEngine()
{    
}

///------------------------------------------------------------------------------------------------

void GameActionEngine::Update(const float dtMillis)
{
    if (mOperationMode == EngineOperationMode::HEADLESS)
    {
        if (GetActiveGameActionName() != IDLE_GAME_ACTION_NAME)
        {
            auto sizeBefore = mGameActions.size();
            mGameActions.front()->VSetNewGameState();
            ReadjustActionQueue(sizeBefore);
            
            mGameActions.pop();
            
            if (mGameActions.empty())
            {
                CreateAndPushGameAction(IDLE_GAME_ACTION_NAME, {});
            }
        }
    }
    else if (mOperationMode == EngineOperationMode::ANIMATED)
    {
        if (GetActiveGameActionName() != IDLE_GAME_ACTION_NAME)
        {
            if (!mActiveActionHasSetState)
            {
                LogActionTransition("Setting state and initializing animation of action " + mGameActions.front()->VGetName().GetString());
                
                auto sizeBefore = mGameActions.size();
                mGameActions.front()->VSetNewGameState();
                mGameActions.front()->VInitAnimation();
                mActiveActionHasSetState = true;
                ReadjustActionQueue(sizeBefore);
            }
            
            if (mGameActions.front()->VUpdateAnimation(dtMillis) == ActionAnimationUpdateResult::FINISHED)
            {
                LogActionTransition("Removing post finished animation action " + mGameActions.front()->VGetName().GetString());
                mGameActions.pop();
                mActiveActionHasSetState = false;
            }
            
            if (mGameActions.empty())
            {
                CreateAndPushGameAction(IDLE_GAME_ACTION_NAME, {});
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void GameActionEngine::AddGameAction(const strutils::StringId& actionName, const std::unordered_map<std::string, std::string> extraActionParams /* = {} */)
{
    if (GetActiveGameActionName() == IDLE_GAME_ACTION_NAME)
    {
        mGameActions.pop();
    }
    
    CreateAndPushGameAction(actionName, extraActionParams);
}

///------------------------------------------------------------------------------------------------

void GameActionEngine::SetLoggingActionTransitions(const bool logActionTransitions)
{
    mLoggingActionTransitions = logActionTransitions;
}

///------------------------------------------------------------------------------------------------

const strutils::StringId& GameActionEngine::GetActiveGameActionName() const
{
    return mGameActions.front()->VGetName();
}

///------------------------------------------------------------------------------------------------

size_t GameActionEngine::GetActionCount() const
{
    return mGameActions.size();
}

///------------------------------------------------------------------------------------------------

bool GameActionEngine::LoggingActionTransitions() const
{
    return mLoggingActionTransitions;
}

///------------------------------------------------------------------------------------------------

void GameActionEngine::CreateAndPushGameAction(const strutils::StringId& actionName, const ExtraActionParams& extraActionParams)
{
    auto action = GameActionFactory::CreateGameAction(actionName);
    action->SetName(actionName);
    action->SetDependencies(mBoardState, mBattleSceneLogicManager, mGameRuleEngine, this);
    action->SetExtraActionParams(extraActionParams);
    mGameActions.push(std::move(action));
    
    if (mGameActions.back()->VShouldBeSerialized())
    {
        events::EventSystem::GetInstance().DispatchEvent<events::SerializableGameActionEvent>(actionName, extraActionParams);
    }
    
    LogActionTransition("Pushed and logged action " + actionName.GetString());
}

///------------------------------------------------------------------------------------------------

void GameActionEngine::LogActionTransition(const std::string& actionTransition)
{
    if (mLoggingActionTransitions)
    {
        logging::Log(logging::LogType::INFO, "%s", actionTransition.c_str());
    }
}

///------------------------------------------------------------------------------------------------

void GameActionEngine::ReadjustActionQueue(const size_t sizeBeforeNewState)
{
    if (sizeBeforeNewState == mGameActions.size()) return;
    
    // On replay scenarios in particular, dynamically added actions
    // could be pushed to the end of the queue erroneously, if more
    // actions have been registered between the creator action and the createe,
    // hence the need for readjustment
    assert(sizeBeforeNewState < mGameActions.size());
    
    auto actionsToInterjectCount = static_cast<int>(mGameActions.size() - sizeBeforeNewState);
    auto intermediateActionCount = static_cast<int>(mGameActions.size() - 1 - actionsToInterjectCount);
    
    assert(intermediateActionCount >= 0);
    
    std::queue<std::unique_ptr<IGameAction>> intermediateActions;
    std::queue<std::unique_ptr<IGameAction>> finalActionQueue;
    
    // Push current action to final queue
    finalActionQueue.push(std::move(mGameActions.front()));
    mGameActions.pop();
    
    // Push intermediate actions to midActions queue
    for (int i = 0; i < intermediateActionCount; ++i)
    {
        intermediateActions.push(std::move(mGameActions.front()));
        mGameActions.pop();
    }
    
    // Add the dynamically created actions to the final queue
    for (int i = 0; i < actionsToInterjectCount; ++i)
    {
        finalActionQueue.push(std::move(mGameActions.front()));
        mGameActions.pop();
    }
    
    // Finally add all intermediate actions to the final queue
    while (!intermediateActions.empty())
    {
        finalActionQueue.push(std::move(intermediateActions.front()));
        intermediateActions.pop();
    }
    
    mGameActions = std::move(finalActionQueue);
}

///------------------------------------------------------------------------------------------------
