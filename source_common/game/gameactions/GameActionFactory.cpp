///------------------------------------------------------------------------------------------------
///  GameActionFactory.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/09/2023                                                       
///------------------------------------------------------------------------------------------------

#include <game/gameactions/BattleInitialSetupAndAnimationGameAction.h>
#include <game/gameactions/CardAttackGameAction.h>
#include <game/gameactions/CardBuffedDebuffedAnimationGameAction.h>
#include <game/gameactions/CardEffectGameAction.h>
#include <game/gameactions/CardDestructionGameAction.h>
#include <game/gameactions/CardHistoryEntryAdditionGameAction.h>
#include <game/gameactions/CardPlayedParticleEffectGameAction.h>
#include <game/gameactions/DemonPunchGameAction.h>
#include <game/gameactions/DrawCardGameAction.h>
#include <game/gameactions/GameActionFactory.h>
#include <game/gameactions/GameOverGameAction.h>
#include <game/gameactions/GameOverResurrectionCheckGameAction.h>
#include <game/gameactions/GoldenCardPlayedEffectGameAction.h>
#include <game/gameactions/HealNextDinoDamageGameAction.h>
#include <game/gameactions/HeroCardEntryGameAction.h>
#include <game/gameactions/HoundSummoningGameAction.h>
#include <game/gameactions/IdleGameAction.h>
#include <game/gameactions/InsectDuplicationGameAction.h>
#include <game/gameactions/InsectMegaSwarmGameAction.h>
#include <game/gameactions/InsectVirusGameAction.h>
#include <game/gameactions/MeteorCardSacrificeGameAction.h>
#include <game/gameactions/MeteorDamageGameAction.h>
#include <game/gameactions/NextDinoDamageDoublingGameAction.h>
#include <game/gameactions/NextPlayerGameAction.h>
#include <game/gameactions/PlayCardGameAction.h>
#include <game/gameactions/PoisonStackApplicationGameAction.h>
#include <game/gameactions/PostNextPlayerGameAction.h>
#include <game/gameactions/RodentsDigAnimationGameAction.h>
#include <game/gameactions/TrapTriggeredAnimationGameAction.h>
#include <game/gameactions/ZeroCostTimeGameAction.h>
#include <game/gameactions/HowToPlayACardTutorialGameAction.h>
#include <game/gameactions/EndTurnTutorialGameAction.h>
#include <game/gameactions/DinoDamageReversalGameAction.h>
#include <game/gameactions/SpellKillGameAction.h>
#include <algorithm>
#include <vector>

///------------------------------------------------------------------------------------------------

#define REGISTER_ACTION(name) REGISTERED_ACTION_NAMES.push_back(strutils::StringId(#name))
#define ACTION_CASE(name) if (actionName == strutils::StringId(#name)) { return std::make_unique<name>(); }

///------------------------------------------------------------------------------------------------

static std::vector<strutils::StringId> REGISTERED_ACTION_NAMES;

///------------------------------------------------------------------------------------------------

void GameActionFactory::RegisterGameActions()
{
    REGISTERED_ACTION_NAMES.clear();
    
    REGISTER_ACTION(IdleGameAction);
    REGISTER_ACTION(BattleInitialSetupAndAnimationGameAction);
    REGISTER_ACTION(CardAttackGameAction);
    REGISTER_ACTION(CardEffectGameAction);
    REGISTER_ACTION(DemonPunchGameAction);
    REGISTER_ACTION(DrawCardGameAction);
    REGISTER_ACTION(GameOverGameAction);
    REGISTER_ACTION(CardPlayedParticleEffectGameAction);
    REGISTER_ACTION(NextPlayerGameAction);
    REGISTER_ACTION(PlayCardGameAction);
    REGISTER_ACTION(CardDestructionGameAction);
    REGISTER_ACTION(PostNextPlayerGameAction);
    REGISTER_ACTION(TrapTriggeredAnimationGameAction);
    REGISTER_ACTION(GoldenCardPlayedEffectGameAction);
    REGISTER_ACTION(HeroCardEntryGameAction);
    REGISTER_ACTION(PoisonStackApplicationGameAction);
    REGISTER_ACTION(RodentsDigAnimationGameAction);
    REGISTER_ACTION(InsectDuplicationGameAction);
    REGISTER_ACTION(InsectMegaSwarmGameAction);
    REGISTER_ACTION(InsectVirusGameAction);
    REGISTER_ACTION(NextDinoDamageDoublingGameAction);
    REGISTER_ACTION(HealNextDinoDamageGameAction);
    REGISTER_ACTION(CardBuffedDebuffedAnimationGameAction);
    REGISTER_ACTION(CardHistoryEntryAdditionGameAction);
    REGISTER_ACTION(HoundSummoningGameAction);
    REGISTER_ACTION(MeteorCardSacrificeGameAction);
    REGISTER_ACTION(MeteorDamageGameAction);
    REGISTER_ACTION(ZeroCostTimeGameAction);
    REGISTER_ACTION(GameOverResurrectionCheckGameAction);
    REGISTER_ACTION(HowToPlayACardTutorialGameAction);
    REGISTER_ACTION(EndTurnTutorialGameAction);
    REGISTER_ACTION(DinoDamageReversalGameAction);
    REGISTER_ACTION(SpellKillGameAction);
    std::sort(REGISTERED_ACTION_NAMES.begin(), REGISTERED_ACTION_NAMES.end(), [&](const strutils::StringId& lhs, const strutils::StringId& rhs)
    {
        return lhs.GetString() < rhs.GetString();
    });
}

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& GameActionFactory::GetRegisteredActions()
{
    return REGISTERED_ACTION_NAMES;
}

///------------------------------------------------------------------------------------------------

std::unique_ptr<BaseGameAction> GameActionFactory::CreateGameAction(const strutils::StringId& actionName)
{
    ACTION_CASE(IdleGameAction);
    ACTION_CASE(BattleInitialSetupAndAnimationGameAction);
    ACTION_CASE(CardAttackGameAction);
    ACTION_CASE(CardEffectGameAction);
    ACTION_CASE(CardDestructionGameAction);
    ACTION_CASE(DemonPunchGameAction);
    ACTION_CASE(DrawCardGameAction);
    ACTION_CASE(GameOverGameAction);
    ACTION_CASE(CardPlayedParticleEffectGameAction);
    ACTION_CASE(NextPlayerGameAction);
    ACTION_CASE(PlayCardGameAction);
    ACTION_CASE(PostNextPlayerGameAction);
    ACTION_CASE(TrapTriggeredAnimationGameAction);
    ACTION_CASE(GoldenCardPlayedEffectGameAction);
    ACTION_CASE(HeroCardEntryGameAction);
    ACTION_CASE(PoisonStackApplicationGameAction);
    ACTION_CASE(RodentsDigAnimationGameAction);
    ACTION_CASE(InsectDuplicationGameAction);
    ACTION_CASE(InsectMegaSwarmGameAction);
    ACTION_CASE(InsectVirusGameAction);
    ACTION_CASE(NextDinoDamageDoublingGameAction);
    ACTION_CASE(HealNextDinoDamageGameAction);
    ACTION_CASE(CardBuffedDebuffedAnimationGameAction);
    ACTION_CASE(CardHistoryEntryAdditionGameAction);
    ACTION_CASE(HoundSummoningGameAction);
    ACTION_CASE(MeteorCardSacrificeGameAction);
    ACTION_CASE(MeteorDamageGameAction);
    ACTION_CASE(ZeroCostTimeGameAction);
    ACTION_CASE(GameOverResurrectionCheckGameAction);
    ACTION_CASE(HowToPlayACardTutorialGameAction);
    ACTION_CASE(EndTurnTutorialGameAction);
    ACTION_CASE(DinoDamageReversalGameAction);
    ACTION_CASE(SpellKillGameAction);
    assert(false && "Invalid game action name");
    return nullptr;
}

///------------------------------------------------------------------------------------------------
