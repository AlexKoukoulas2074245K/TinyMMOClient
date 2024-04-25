///------------------------------------------------------------------------------------------------
///  CardEffectGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 09/11/2023
///------------------------------------------------------------------------------------------------

#include <game/CardEffectComponents.h>
#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/CardBuffedDebuffedAnimationGameAction.h>
#include <game/gameactions/CardDestructionGameAction.h>
#include <game/gameactions/CardEffectGameAction.h>
#include <game/gameactions/CardHistoryEntryAdditionGameAction.h>
#include <game/gameactions/DemonPunchGameAction.h>
#include <game/gameactions/DrawCardGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/HoundSummoningGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>

///------------------------------------------------------------------------------------------------

const std::unordered_map<CardEffectGameAction::AffectedStatType, CardStatType> CardEffectGameAction::sAffectedStatTypeToCardStatType =
{
    {AffectedStatType::DAMAGE, CardStatType::DAMAGE},
    {AffectedStatType::WEIGHT, CardStatType::WEIGHT}
};

// Follow up game actions
static const strutils::StringId CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME = strutils::StringId("CardBuffedDebuffedAnimationGameAction");
static const strutils::StringId CARD_DESTRUCTION_GAME_ACTION_NAME = strutils::StringId("CardDestructionGameAction");
static const strutils::StringId CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME = strutils::StringId("CardHistoryEntryAdditionGameAction");
static const strutils::StringId CARD_EFFECT_GAME_ACTION_NAME = strutils::StringId("CardEffectGameAction");
static const strutils::StringId CARD_PLAYED_PARTICLE_EFFECT_GAME_ACTION_NAME = strutils::StringId("CardPlayedParticleEffectGameAction");
static const strutils::StringId INSECT_MEGASWARM_GAME_ACTION_NAME = strutils::StringId("InsectMegaSwarmGameAction");
static const strutils::StringId HOUND_SUMMONING_GAME_ACTION_NAME = strutils::StringId("HoundSummoningGameAction");
static const strutils::StringId DEMON_PUNCH_GAME_ACTION_NAME = strutils::StringId("DemonPunchGameAction");
static const strutils::StringId METEOR_CARD_SACRIFICE_GAME_ACTION_NAME = strutils::StringId("MeteorCardSacrificeGameAction");
static const strutils::StringId DINO_DAMAGE_REVERSAL_GAME_ACTION_NAME = strutils::StringId("DinoDamageReversalGameAction");

// Resources
static const std::string EFFECT_SFX = "sfx_chime";
static const std::string CARD_DISSOLVE_SHADER_FILE_NAME = "card_spell_dissolve.vs";
static const std::string DISSOLVE_TEXTURE_FILE_NAME = "dissolve.png";
static const std::string BUFFED_CARD_PARTICLE_EMITTER_NAME_PREFIX = "card_effect_emitter_";

// Uniforms
static const strutils::StringId DISSOLVE_THRESHOLD_UNIFORM_NAME = strutils::StringId("dissolve_threshold");
static const strutils::StringId DISSOLVE_MAGNITUDE_UNIFORM_NAME = strutils::StringId("dissolve_magnitude");
static const strutils::StringId CARD_ORIGIN_X_UNIFORM_NAME = strutils::StringId("card_origin_x");
static const strutils::StringId CARD_ORIGIN_Y_UNIFORM_NAME = strutils::StringId("card_origin_y");
static const strutils::StringId CARD_SPELL_EFFECT_PARTICLE_NAME = strutils::StringId("card_spell_effect");
static const strutils::StringId CARD_EFFECT_PARTICLE_EMITTER_NAME = strutils::StringId("card_effect_emitter");
static const strutils::StringId DRAW_CARD_GAME_ACTION_NAME = strutils::StringId("DrawCardGameAction");

static const float CARD_DISSOLVE_SPEED = 0.002f;
static const float MAX_CARD_DISSOLVE_VALUE = 1.2f;
static const float CARD_EFFECT_PARTICLE_EMITTER_Z_OFFSET = 21.0f;
static const float CARD_SCALE_UP_FACTOR = 1.5f;
static const float CARD_SCALE_DOWN_FACTOR = 0.5f;
static const float CARD_DISSOLVE_Z_BUMP = 0.05f;

static const glm::vec2 CARD_DISSOLVE_EFFECT_MAG_RANGE = {10.0f, 18.0f};

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
};

///------------------------------------------------------------------------------------------------

void CardEffectGameAction::VSetNewGameState()
{
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    auto cardId = activePlayerState.mPlayerBoardCards.back();
    const auto& cardEffectData = CardDataRepository::GetInstance().GetCardData(cardId, mBoardState->GetActivePlayerIndex());
    mBuffingSingleUseCardCase = false;
    
    // Handle single use spells
    if (cardEffectData.mIsSingleUse)
    {
        mBuffingSingleUseCardCase = true;
        
        // Erase spell from deck
        activePlayerState.mPlayerDeckCards.erase(std::remove(activePlayerState.mPlayerDeckCards.begin(), activePlayerState.mPlayerDeckCards.end(), cardId), activePlayerState.mPlayerDeckCards.end());
        if (activePlayerState.mPlayerDeckCards.empty())
        {
            activePlayerState.mPlayerDeckCards = { CardDataRepository::GetInstance().GetCardId(game_constants::EMPTY_DECK_TOKEN_CARD_NAME) };
        }
        
        // Find all held card indices for this card id
        std::vector<int> heldCardIndicesToDestroy;
        auto heldCardIter = activePlayerState.mPlayerHeldCards.begin();
        while ((heldCardIter = std::find_if(heldCardIter, activePlayerState.mPlayerHeldCards.end(), [=](const int heldCardId){ return heldCardId == cardId; })) != activePlayerState.mPlayerHeldCards.end())
        {
            heldCardIndicesToDestroy.push_back(static_cast<int>(std::distance(activePlayerState.mPlayerHeldCards.begin(), heldCardIter)));
            
            if (activePlayerState.mPlayerHeldCardStatOverrides.size() > heldCardIndicesToDestroy.back())
            {
                activePlayerState.mPlayerHeldCardStatOverrides.erase(activePlayerState.mPlayerHeldCardStatOverrides.begin() + heldCardIndicesToDestroy.back());
            }
            
            heldCardIter++;
        }
        
        if (!heldCardIndicesToDestroy.empty())
        {
            mGameActionEngine->AddGameAction(CARD_DESTRUCTION_GAME_ACTION_NAME,
            {
                { CardDestructionGameAction::CARD_INDICES_PARAM, strutils::VecToString(heldCardIndicesToDestroy)},
                { CardDestructionGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())},
                { CardDestructionGameAction::IS_SINGLE_CARD_USED_COPY_PARAM, "true"},
                { CardDestructionGameAction::IS_BOARD_CARD_PARAM, "false"},
                { CardDestructionGameAction::IS_TRAP_TRIGGER_PARAM, "false"}
            });
            
            for (auto heldCardIter = activePlayerState.mPlayerHeldCards.begin(); heldCardIter != activePlayerState.mPlayerHeldCards.end();)
            {
                if (*heldCardIter == cardId)
                {
                    heldCardIter = activePlayerState.mPlayerHeldCards.erase(heldCardIter);
                }
                else
                {
                    heldCardIter++;
                }
            }
            
            events::EventSystem::GetInstance().DispatchEvent<events::BlockInteractionWithHeldCardsEvent>();
        }
    }
    
    HandleCardEffect(cardEffectData.mCardEffect);
    
    // shouldn't really happen
    if (activePlayerState.mPlayerBoardCardStatOverrides.size() == activePlayerState.mPlayerBoardCards.size())
    {
        activePlayerState.mPlayerBoardCardStatOverrides.pop_back();
    }
    
    activePlayerState.mPlayerBoardCards.pop_back();
    
    // Card Token special case
    if (mCardTokenCase)
    {
        auto availableCardDataCount = static_cast<int>(activePlayerState.mPlayerInitialDeckCards.size());
        auto randomCardIndex = math::ControlledRandomInt() % availableCardDataCount;
        activePlayerState.mPlayerBoardCards.push_back(activePlayerState.mPlayerInitialDeckCards[randomCardIndex]);
        
        const auto& cardData = CardDataRepository::GetInstance().GetCardData(activePlayerState.mPlayerBoardCards.back(), mBoardState->GetActivePlayerIndex());
        
        // Card-specific particle animation
        if (!cardData.mParticleEffect.isEmpty())
        {
            mGameActionEngine->AddGameAction(CARD_PLAYED_PARTICLE_EFFECT_GAME_ACTION_NAME);
        }
        
        if (cardData.IsSpell())
        {
            mGameActionEngine->AddGameAction(CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME,
            {
                { CardHistoryEntryAdditionGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex()) },
                { CardHistoryEntryAdditionGameAction::CARD_INDEX_PARAM, std::to_string(activePlayerState.mPlayerBoardCards.size() - 1) },
                { CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_PARAM, CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_EFFECT },
                { CardHistoryEntryAdditionGameAction::IS_TURN_COUNTER_PARAM, "false"}
            });
            
            mGameActionEngine->AddGameAction(CARD_EFFECT_GAME_ACTION_NAME);
        }
    }
}

///------------------------------------------------------------------------------------------------

void CardEffectGameAction::VInitAnimation()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
        
    systemsEngine.GetSoundManager().PreloadSfx(EFFECT_SFX);
    systemsEngine.GetSoundManager().PlaySound(EFFECT_SFX);
    
    auto cardEffectBoardCardIndex = mBoardState->GetActivePlayerState().mPlayerBoardCards.size();
    if (mCardTokenCase)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::EmptyDeckCardTokenPlayedEvent>();
        cardEffectBoardCardIndex--;
    }
    
    auto cardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers().at(mBoardState->GetActivePlayerIndex()).at(cardEffectBoardCardIndex);
    cardSoWrapper->mSceneObject->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_DISSOLVE_SHADER_FILE_NAME);
    cardSoWrapper->mSceneObject->mEffectTextureResourceIds[1] = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + DISSOLVE_TEXTURE_FILE_NAME);
    cardSoWrapper->mSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] = 0.0f;
    cardSoWrapper->mSceneObject->mShaderFloatUniformValues[CARD_ORIGIN_X_UNIFORM_NAME] = cardSoWrapper->mSceneObject->mPosition.x;
    cardSoWrapper->mSceneObject->mShaderFloatUniformValues[CARD_ORIGIN_Y_UNIFORM_NAME] = cardSoWrapper->mSceneObject->mPosition.y;
    cardSoWrapper->mSceneObject->mShaderFloatUniformValues[DISSOLVE_MAGNITUDE_UNIFORM_NAME] = math::RandomFloat(CARD_DISSOLVE_EFFECT_MAG_RANGE.x, CARD_DISSOLVE_EFFECT_MAG_RANGE.y);
    cardSoWrapper->mSceneObject->mPosition.z += CARD_DISSOLVE_Z_BUMP;
    
    CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->RemoveSceneObject(CARD_EFFECT_PARTICLE_EMITTER_NAME);
    systemsEngine.GetParticleManager().CreateParticleEmitterAtPosition
    (
        CARD_SPELL_EFFECT_PARTICLE_NAME,
        glm::vec3(cardSoWrapper->mSceneObject->mPosition.x, cardSoWrapper->mSceneObject->mPosition.y, CARD_EFFECT_PARTICLE_EMITTER_Z_OFFSET), // pos
        *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE), // scene
        CARD_EFFECT_PARTICLE_EMITTER_NAME
    );
    
    // Force release all held/moving cards back to position
    for (const auto& affectedCardEntry: mAffectedCards)
    {
        if (!affectedCardEntry.mIsBoardCard && !mBuffingSingleUseCardCase)
        {
            events::EventSystem::GetInstance().DispatchEvent<events::ForceSendCardBackToPositionEvent>(static_cast<int>(affectedCardEntry.mCardIndex), affectedCardEntry.mIsBoardCard, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
        }
    }
    
    mActionState = ActionState::EFFECT_CARD_ANIMATION;
    mAnimationDelayCounterSecs = 0.0f;
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult CardEffectGameAction::VUpdateAnimation(const float dtMillis)
{ 
    switch (mActionState)
    {
        case ActionState::EFFECT_CARD_ANIMATION:
        {
            const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
            const auto& scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
            
            const auto& boardCards = mBoardState->GetActivePlayerState().mPlayerBoardCards;
            const auto& deadBoardCardIndices = mBoardState->GetActivePlayerState().mBoardCardIndicesToDestroy;
            
            const auto& heldCards = mBoardState->GetActivePlayerState().mPlayerHeldCards;
            const auto& deadHeldCardIndices = mBoardState->GetActivePlayerState().mHeldCardIndicesToDestroy;
            
            auto boardCardIndex = boardCards.size();
            if (mCardTokenCase)
            {
                boardCardIndex--;
            }
            
            auto effectCardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers().at(mBoardState->GetActivePlayerIndex()).at(boardCardIndex);
            effectCardSoWrapper->mSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] += dtMillis * CARD_DISSOLVE_SPEED;
          
            if (effectCardSoWrapper->mSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] >= MAX_CARD_DISSOLVE_VALUE/2)
            {
                // Fade particle emitter on spell
                CoreSystemsEngine::GetInstance().GetParticleManager().RemoveParticleEmitterFlag(particle_flags::CONTINUOUS_PARTICLE_GENERATION, CARD_EFFECT_PARTICLE_EMITTER_NAME, *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE));
            }
            
            if (effectCardSoWrapper->mSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] >= MAX_CARD_DISSOLVE_VALUE)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::ImmediateCardDestructionWithRepositionEvent>(static_cast<int>(boardCardIndex), true, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
                
                // Create particle emitters on affected cards
                for (size_t i = 0U; i < mAffectedCards.size(); ++i)
                {
                    auto& affectedCardEntry = mAffectedCards[i];
                    
                    auto cardSoWrapper = affectedCardEntry.mIsBoardCard ?
                        mBattleSceneLogicManager->GetBoardCardSoWrappers().at(mBoardState->GetActivePlayerIndex()).at(affectedCardEntry.mCardIndex):
                        mBattleSceneLogicManager->GetHeldCardSoWrappers().at(mBoardState->GetActivePlayerIndex()).at(affectedCardEntry.mCardIndex);
                    
                    auto targetPosition = affectedCardEntry.mIsBoardCard ?
                        card_utils::CalculateBoardCardPosition(affectedCardEntry.mCardIndex, card_utils::CalculateNonDeadCardsCount(boardCards, deadBoardCardIndices), mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX):
                        card_utils::CalculateHeldCardPosition(affectedCardEntry.mCardIndex, card_utils::CalculateNonDeadCardsCount(heldCards, deadHeldCardIndices), mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, scene->GetCamera());
                    
                    CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition
                    (
                        CARD_SPELL_EFFECT_PARTICLE_NAME,
                        glm::vec3(targetPosition.x, targetPosition.y, CARD_EFFECT_PARTICLE_EMITTER_Z_OFFSET), // pos
                        *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE), // scene
                        strutils::StringId(BUFFED_CARD_PARTICLE_EMITTER_NAME_PREFIX + std::to_string(i))
                    );
                }
                
                mActionState = ActionState::AFFECTED_CARDS_SPARKLE_ANIMATION;
            }
        } break;
            
        case ActionState::AFFECTED_CARDS_SPARKLE_ANIMATION:
        {
            switch (mCardBoardEffectMask)
            {
                case effects::board_modifier_masks::KILL_NEXT:
                case effects::board_modifier_masks::DEMON_KILL_NEXT:
                case effects::board_modifier_masks::SPELL_KILL_NEXT:
                case effects::board_modifier_masks::BOARD_SIDE_DEBUFF:
                case effects::board_modifier_masks::DOUBLE_POISON_ATTACKS:
                case effects::board_modifier_masks::INSECT_VIRUS:
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectTriggeredEvent>(mBoardState->GetActivePlayerIndex() != game_constants::REMOTE_PLAYER_INDEX, mCardBoardEffectMask);
                } break;
                    
                case effects::board_modifier_masks::DIG_NO_FAIL:
                case effects::board_modifier_masks::RODENT_LIFESTEAL:
                case effects::board_modifier_masks::DUPLICATE_NEXT_INSECT:
                case effects::board_modifier_masks::DOUBLE_NEXT_DINO_DAMAGE:
                case effects::board_modifier_masks::HEAL_NEXT_DINO_DAMAGE:
                case effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION:
                case effects::board_modifier_masks::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST:
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectTriggeredEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, mCardBoardEffectMask);
                }
            }
            
            if (std::find(mEffectComponents.cbegin(), mEffectComponents.cend(), effects::EFFECT_COMPONENT_CLEAR_EFFECTS) != mEffectComponents.cend())
            {
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true,  effects::board_modifier_masks::BOARD_SIDE_DEBUFF);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true,  effects::board_modifier_masks::KILL_NEXT);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true,  effects::board_modifier_masks::SPELL_KILL_NEXT);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true,  effects::board_modifier_masks::DEMON_KILL_NEXT);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true,  effects::board_modifier_masks::DUPLICATE_NEXT_INSECT);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true, effects::board_modifier_masks::DIG_NO_FAIL);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true, effects::board_modifier_masks::RODENT_LIFESTEAL);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true,  effects::board_modifier_masks::DOUBLE_NEXT_DINO_DAMAGE);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true,  effects::board_modifier_masks::HEAL_NEXT_DINO_DAMAGE);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true,  effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true,  effects::board_modifier_masks::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true,  effects::board_modifier_masks::INSECT_VIRUS);
                events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true,  effects::board_modifier_masks::DOUBLE_POISON_ATTACKS);
            }
            
            if (mAffectedCards.empty())
            {
                mActionState = ActionState::FINISHED;
            }
            
            mAnimationDelayCounterSecs += dtMillis/1000.0f;
            if (mAnimationDelayCounterSecs > 0.5f)
            {
                mAnimationDelayCounterSecs = 0.0f;
                mActionState = ActionState::FINISHED;
            }
        } break;
            
        default: break;
    }
    
   return mActionState == ActionState::FINISHED ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool CardEffectGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& CardEffectGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------

void CardEffectGameAction::HandleCardEffect(const std::string& effect)
{
    mCardTokenCase = false;
    mCardBoardEffectMask = effects::board_modifier_masks::NONE;
    mAffectedBoardCardsStatType = AffectedStatType::NONE;
    mEffectValue = 0;
    mAffectedCards.clear();
        
    mEffectComponents = strutils::StringSplit(effect, ' ');
    const auto& boardCards = mBoardState->GetActivePlayerState().mPlayerBoardCards;
    const auto& heldCards = mBoardState->GetActivePlayerState().mPlayerHeldCards;
    
    auto effectCardFamily = CardDataRepository::GetInstance().GetCardData(boardCards.back(), mBoardState->GetActivePlayerIndex()).mCardFamily;
    if (effectCardFamily == game_constants::DEMONS_NORMAL_FAMILY_NAME ||
        effectCardFamily == game_constants::DEMONS_MEDIUM_FAMILY_NAME ||
        effectCardFamily == game_constants::DEMONS_HARD_FAMILY_NAME ||
        effectCardFamily == game_constants::DEMONS_BOSS_FAMILY_NAME)
    {
        effectCardFamily = game_constants::DEMONS_GENERIC_FAMILY_NAME;
    }
    
    bool affectingFamilyOnly = false;
    
    std::vector<int> affectedBoardCardIndices;
    std::vector<int> affectedHeldCardIndices;
    
    for (const auto& effectComponent: mEffectComponents)
    {
        // Collection component
        if (effectComponent == effects::EFFECT_COMPONENT_FAMILY)
        {
            affectingFamilyOnly = true;
        }
        
        // Stat Type component
        else if (effectComponent == effects::EFFECT_COMPONENT_DAMAGE)
        {
            mAffectedBoardCardsStatType = AffectedStatType::DAMAGE;
        }
        else if (effectComponent == effects::EFFECT_COMPONENT_WEIGHT)
        {
            mAffectedBoardCardsStatType = AffectedStatType::WEIGHT;
        }
        
        // Random buff damage of card hand
        else if (effectComponent == effects::EFFECT_COMPONENT_RANDOM_HAND_BUFF_ATTACK)
        {
            mAffectedBoardCardsStatType = AffectedStatType::DAMAGE;
            
            if (!heldCards.empty() && std::find_if(heldCards.cbegin(), heldCards.cend(), [&](const int cardId){ return !CardDataRepository::GetInstance().GetCardData(cardId, mBoardState->GetActivePlayerIndex()).IsSpell(); }) != heldCards.cend())
            {
                auto randomHeldCardIndex = math::ControlledRandomInt() % heldCards.size();
                while (CardDataRepository::GetInstance().GetCardData(heldCards[randomHeldCardIndex], mBoardState->GetActivePlayerIndex()).IsSpell())
                {
                    randomHeldCardIndex = math::ControlledRandomInt() % heldCards.size();
                }
                affectedHeldCardIndices.emplace_back(randomHeldCardIndex);
            }
        }
        
        // Tripples lowest normal card's damage on hand
        else if (effectComponent == effects::EFFECT_COMPONENT_TRIPPLES_LOWEST_ATTACK_ON_HAND)
        {
            mAffectedBoardCardsStatType = AffectedStatType::DAMAGE;
            
            // Filter out spell cards and find lowest attack card
            int selectedCardIndex = -1;
            int minDamageFound = 20;
            for (int i = 0; i < heldCards.size(); ++i)
            {
                const auto& cardData = CardDataRepository::GetInstance().GetCardData(heldCards[i], mBoardState->GetActivePlayerIndex());
                if (!cardData.IsSpell() && cardData.mCardDamage < minDamageFound)
                {
                    minDamageFound = cardData.mCardDamage;
                    selectedCardIndex = i;
                }
            }
            
            // Adjust or create held card stat override and buff
            if (selectedCardIndex != -1)
            {
                auto& activePlayerState = mBoardState->GetActivePlayerState();
                auto& playerHeldCardStatOverrides = activePlayerState.mPlayerHeldCardStatOverrides;
                
                if (playerHeldCardStatOverrides.size() > selectedCardIndex && playerHeldCardStatOverrides[selectedCardIndex].count(CardStatType::DAMAGE))
                {
                    playerHeldCardStatOverrides[selectedCardIndex][CardStatType::DAMAGE] *= 3;
                }
                else if (playerHeldCardStatOverrides.size() <= selectedCardIndex)
                {
                    playerHeldCardStatOverrides.resize(selectedCardIndex + 1);
                    playerHeldCardStatOverrides[selectedCardIndex][CardStatType::DAMAGE] = CardDataRepository::GetInstance().GetCardData(heldCards[selectedCardIndex], mBoardState->GetActivePlayerIndex()).mCardDamage * 3;
                }
                
                affectedHeldCardIndices.push_back(selectedCardIndex);
            }
        }
        
        // Clear effects component
        else if (effectComponent == effects::EFFECT_COMPONENT_CLEAR_EFFECTS)
        {
            if ((mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::BOARD_SIDE_DEBUFF) != 0)
            {
                for (auto i = 0U; i < boardCards.size() - 1; ++i)
                {
                    mGameActionEngine->AddGameAction(CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME,
                    {
                        { CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM, std::to_string(i)},
                        { CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())},
                        { CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM, "true" },
                        { CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM, std::to_string(CARD_SCALE_UP_FACTOR) },
                        { CardBuffedDebuffedAnimationGameAction::CARD_BUFFED_REPEAT_INDEX, std::to_string(i) }
                    });
                }
            }
            else if ((mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION) != 0)
            {
                for (auto i = 0U; i < heldCards.size(); ++i)
                {
                    if (!CardDataRepository::GetInstance().GetCardData(heldCards[i], mBoardState->GetActivePlayerIndex()).IsSpell())
                    {
                        mGameActionEngine->AddGameAction(CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME,
                        {
                            { CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM, std::to_string(i)},
                            { CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())},
                            { CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM, "false" },
                            { CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM, std::to_string(CARD_SCALE_DOWN_FACTOR) },
                            { CardBuffedDebuffedAnimationGameAction::CARD_BUFFED_REPEAT_INDEX, std::to_string(i) }
                        });
                    }
                }
            }
            
            mBoardState->GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers.clear();
            mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask = effects::board_modifier_masks::NONE;
        }
        
        // Kill component
        else if (effectComponent == effects::EFFECT_COMPONENT_KILL)
        {
            mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::KILL_NEXT;
            mCardBoardEffectMask = effects::board_modifier_masks::KILL_NEXT;
        }
        
        // Spell Kill component
        else if (effectComponent == effects::EFFECT_COMPONENT_SPELL_KILL)
        {
            mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::SPELL_KILL_NEXT;
            mCardBoardEffectMask = effects::board_modifier_masks::SPELL_KILL_NEXT;
        }
        
        // Demon Kill component
        else if (effectComponent == effects::EFFECT_COMPONENT_DEMON_KILL)
        {
            mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::DEMON_KILL_NEXT;
            mCardBoardEffectMask = effects::board_modifier_masks::DEMON_KILL_NEXT;
        }
        
        // Insect Duplication component
        else if (effectComponent == effects::EFFECT_COMPONENT_DUPLICATE_INSECT)
        {
            mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::DUPLICATE_NEXT_INSECT;
            mCardBoardEffectMask = effects::board_modifier_masks::DUPLICATE_NEXT_INSECT;
        }
        
        // Dig no Fail component
        else if (effectComponent == effects::EFFECT_COMPONENT_DIG_NO_FAIL)
        {
            mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::DIG_NO_FAIL;
            mCardBoardEffectMask = effects::board_modifier_masks::DIG_NO_FAIL;
        }
        
        // Doubling Dino Damage component
        else if (effectComponent == effects::EFFECT_COMPONENT_DOUBLE_NEXT_DINO_DAMAGE)
        {
            mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::DOUBLE_NEXT_DINO_DAMAGE;
            mCardBoardEffectMask = effects::board_modifier_masks::DOUBLE_NEXT_DINO_DAMAGE;
        }
        
        // Heal on next Dino's Damage
        else if (effectComponent == effects::EFFECT_COMPONENT_HEAL_NEXT_DINO_DAMAGE)
        {
            mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::HEAL_NEXT_DINO_DAMAGE;
            mCardBoardEffectMask = effects::board_modifier_masks::HEAL_NEXT_DINO_DAMAGE;
        }
        
        // Meteor
        else if (effectComponent == effects::EFFECT_COMPONENT_METEOR)
        {
            mGameActionEngine->AddGameAction(METEOR_CARD_SACRIFICE_GAME_ACTION_NAME);
        }
        
        // Dino Damage Reversal
        else if (effectComponent == effects::EFFECT_COMPONENT_SWAP_MIN_MAX_DAMAGE)
        {
            mGameActionEngine->AddGameAction(DINO_DAMAGE_REVERSAL_GAME_ACTION_NAME);
        }
        
        // Rodents Lifesteal
        else if (effectComponent == effects::EFFECT_COMPONENT_RODENT_LIFESTEAL_ON_ATTACKS)
        {
            mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::RODENT_LIFESTEAL;
            mCardBoardEffectMask = effects::board_modifier_masks::RODENT_LIFESTEAL;
        }
        
        // Doubling Poison Attacks component
        else if (effectComponent == effects::EFFECT_COMPONENT_DOUBLE_POISON_ATTACKS)
        {
            mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::DOUBLE_POISON_ATTACKS;
            mCardBoardEffectMask = effects::board_modifier_masks::DOUBLE_POISON_ATTACKS;
        }
        
        // Gain 1 Weight Component
        else if (effectComponent == effects::EFFECT_COMPONENT_GAIN_1_WEIGHT)
        {
            mBoardState->GetActivePlayerState().mPlayerCurrentWeightAmmo++;
            events::EventSystem::GetInstance().DispatchEvent<events::WeightChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
        }
        
        // Gain 2 Weight Component
        else if (effectComponent == effects::EFFECT_COMPONENT_GAIN_2_WEIGHT)
        {
            mBoardState->GetActivePlayerState().mPlayerCurrentWeightAmmo += 2;
            events::EventSystem::GetInstance().DispatchEvent<events::WeightChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
        }
        
        // Card Token
        else if (effectComponent == effects::EFFECT_COMPONENT_CARD_TOKEN)
        {
            mCardTokenCase = true;
        }
        
        // Insect Megaswarm
        else if (effectComponent == effects::EFFECT_COMPONENT_INSECT_MEGASWARM)
        {
            mGameActionEngine->AddGameAction(INSECT_MEGASWARM_GAME_ACTION_NAME);
        }
        
        // Insect Megaswarm
        else if (effectComponent == effects::EFFECT_COMPONENT_INSECT_VIRUS)
        {
            mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::INSECT_VIRUS;
            mCardBoardEffectMask = effects::board_modifier_masks::INSECT_VIRUS;
        }
        
        // Toxic Bomb
        else if (effectComponent == effects::EFFECT_COMPONENT_TOXIC_BOMB)
        {
            if (mBoardState->GetActivePlayerState().mPlayerCurrentWeightAmmo > 0)
            {
                int bombStack = mBoardState->GetActivePlayerState().mPlayerCurrentWeightAmmo;
                if ((mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::DOUBLE_POISON_ATTACKS) != 0)
                {
                    bombStack *= 2;
                }
                
                mBoardState->GetInactivePlayerState().mPlayerPoisonStack += bombStack;
                
                mBoardState->GetActivePlayerState().mPlayerCurrentWeightAmmo = 0;
                events::EventSystem::GetInstance().DispatchEvent<events::WeightChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
                
                events::EventSystem::GetInstance().DispatchEvent<events::PoisonStackChangeChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, mBoardState->GetInactivePlayerState().mPlayerPoisonStack);
            }
        }
        
        // Demon Punch
        else if (effectComponent == effects::EFFECT_COMPONENT_DEMON_PUNCH)
        {
            if (mBoardState->GetActivePlayerState().mPlayerCurrentWeightAmmo > 0)
            {
                int damage = mBoardState->GetActivePlayerState().mPlayerCurrentWeightAmmo;
                mBoardState->GetActivePlayerState().mPlayerCurrentWeightAmmo = 0;
                events::EventSystem::GetInstance().DispatchEvent<events::WeightChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
                
                mGameActionEngine->AddGameAction(DEMON_PUNCH_GAME_ACTION_NAME,
                {
                    { DemonPunchGameAction::DEMON_PUNCH_DAMAGE_PARAM, std::to_string(damage)}
                });
            }
        }
        
        // Modifier/Offset value component
        else if (!effects::STATIC_EFFECT_COMPONENT_NAMES.count(effectComponent))
        {
            mEffectValue = std::stoi(effectComponent);
        }
    }
    
    // Board effect
    if (std::find(mEffectComponents.cbegin(), mEffectComponents.cend(), effects::EFFECT_COMPONENT_BOARD) != mEffectComponents.cend())
    {
        for (int i = 0; i < static_cast<int>(boardCards.size()) - 1; ++i)
        {
            auto cardData = CardDataRepository::GetInstance().GetCardData(boardCards[i], mBoardState->GetActivePlayerIndex());
            
            if (affectingFamilyOnly)
            {
                if (effectCardFamily == game_constants::DEMONS_GENERIC_FAMILY_NAME)
                {
                    if
                    (
                        cardData.mCardFamily != game_constants::DEMONS_GENERIC_FAMILY_NAME &&
                        cardData.mCardFamily != game_constants::DEMONS_NORMAL_FAMILY_NAME &&
                        cardData.mCardFamily != game_constants::DEMONS_MEDIUM_FAMILY_NAME &&
                        cardData.mCardFamily != game_constants::DEMONS_HARD_FAMILY_NAME &&
                        cardData.mCardFamily != game_constants::DEMONS_BOSS_FAMILY_NAME
                    )
                    {
                        continue;
                    }
                }
                else if (cardData.mCardFamily != effectCardFamily)
                {
                    continue;
                }
            }
            
            if (!cardData.IsSpell())
            {
                affectedBoardCardIndices.emplace_back(i);
            }
        }
    }
    
    // Held Cards effect
    if (std::find(mEffectComponents.cbegin(), mEffectComponents.cend(), effects::EFFECT_COMPONENT_HELD) != mEffectComponents.cend())
    {
        for (int i = 0; i < static_cast<int>(heldCards.size()); ++i)
        {
            auto cardData = CardDataRepository::GetInstance().GetCardData(heldCards[i], mBoardState->GetActivePlayerIndex());
            
            if (affectingFamilyOnly)
            {
                if (effectCardFamily == game_constants::DEMONS_GENERIC_FAMILY_NAME)
                {
                    if
                    (
                        cardData.mCardFamily != game_constants::DEMONS_GENERIC_FAMILY_NAME &&
                        cardData.mCardFamily != game_constants::DEMONS_NORMAL_FAMILY_NAME &&
                        cardData.mCardFamily != game_constants::DEMONS_MEDIUM_FAMILY_NAME &&
                        cardData.mCardFamily != game_constants::DEMONS_HARD_FAMILY_NAME &&
                        cardData.mCardFamily != game_constants::DEMONS_BOSS_FAMILY_NAME
                    )
                    {
                        continue;
                    }
                }
                else if (cardData.mCardFamily != effectCardFamily)
                {
                    continue;
                }
            }
            
            if (!cardData.IsSpell())
            {
                affectedHeldCardIndices.emplace_back(i);
            }
        }
    }
    
    // Draw spell effect
    if (std::find(mEffectComponents.cbegin(), mEffectComponents.cend(), effects::EFFECT_COMPONENT_DRAW_RANDOM_SPELL) != mEffectComponents.cend())
    {
        mGameActionEngine->AddGameAction(DRAW_CARD_GAME_ACTION_NAME,
        {
            { DrawCardGameAction::DRAW_SPELL_ONLY_PARAM, "true"}
        });
    }
    
    // Hound Summoning
    if (std::find(mEffectComponents.cbegin(), mEffectComponents.cend(), effects::EFFECT_COMPONENT_HOUND_SUMMONING) != mEffectComponents.cend())
    {
        mGameActionEngine->AddGameAction(HOUND_SUMMONING_GAME_ACTION_NAME, { { HoundSummoningGameAction::NUMBER_OF_HOUNDS_PARAM, std::to_string(mEffectValue) } });
    }
    
    // Armor effect
    if (std::find(mEffectComponents.cbegin(), mEffectComponents.cend(), effects::EFFECT_COMPONENT_ARMOR) != mEffectComponents.cend())
    {
        mBoardState->GetActivePlayerState().mPlayerArmorRecharge += mEffectValue;
        mBoardState->GetActivePlayerState().mPlayerCurrentArmor += mEffectValue;
        events::EventSystem::GetInstance().DispatchEvent<events::ArmorChangeChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, mBoardState->GetActivePlayerState().mPlayerCurrentArmor);
    }
    
    // Armor effect
    if (std::find(mEffectComponents.cbegin(), mEffectComponents.cend(), effects::EFFECT_COMPONENT_ADD_POISON_STACKS) != mEffectComponents.cend())
    {
        int poisonStack = mEffectValue;
        if ((mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::DOUBLE_POISON_ATTACKS) != 0)
        {
            poisonStack *= 2;
        }
        
        mBoardState->GetInactivePlayerState().mPlayerPoisonStack += poisonStack;
        
        events::EventSystem::GetInstance().DispatchEvent<events::PoisonStackChangeChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, mBoardState->GetInactivePlayerState().mPlayerPoisonStack);
    }
    
    // Next turn effect
    if (std::find(mEffectComponents.cbegin(), mEffectComponents.cend(), effects::EFFECT_COMPONENT_ENEMY_BOARD_DEBUFF) != mEffectComponents.cend())
    {
        // For Hero Cards
        if (mBoardState->GetInactivePlayerState().mHasHeroCard)
        {
            mGameActionEngine->AddGameAction(CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME,
            {
                { CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM, "0"},
                { CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM, std::to_string(game_constants::REMOTE_PLAYER_INDEX)},
                { CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM, "true" },
                { CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM, std::to_string(CARD_SCALE_DOWN_FACTOR) }
            });
        }
        
        mBoardState->GetInactivePlayerState().mBoardModifiers.mGlobalCardStatModifiers[sAffectedStatTypeToCardStatType.at(mAffectedBoardCardsStatType)] += mEffectValue;
        mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::BOARD_SIDE_DEBUFF;
        mCardBoardEffectMask = effects::board_modifier_masks::BOARD_SIDE_DEBUFF;
    }
    
    // Continual weight reduction component
    else if (std::find(mEffectComponents.cbegin(), mEffectComponents.cend(), effects::EFFECT_COMPONENT_PERMANENT_CONTINUAL_WEIGHT_REDUCTION) != mEffectComponents.cend())
    {
        if (mBoardState->GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers.count(sAffectedStatTypeToCardStatType.at(mAffectedBoardCardsStatType)) == 0)
        {
            mBoardState->GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers[sAffectedStatTypeToCardStatType.at(mAffectedBoardCardsStatType)] = 0;
        }
        
        mBoardState->GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers[sAffectedStatTypeToCardStatType.at(mAffectedBoardCardsStatType)]--;
        mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION;
        mCardBoardEffectMask = effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION;
    }
    
    // Every third card played has zero cost component
    else if (std::find(mEffectComponents.cbegin(), mEffectComponents.cend(), effects::EFFECT_COMPONENT_EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST) != mEffectComponents.cend())
    {
        mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask |= effects::board_modifier_masks::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST;
        mBoardState->GetActivePlayerState().mPlayedCardComboThisTurn = 0;
        mCardBoardEffectMask = effects::board_modifier_masks::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST;
    }
    
    // Create/Modify board card stat overrides
    int particleEmitterIndex = 0;
    for (auto affectedBoardCardIter = affectedBoardCardIndices.begin(); affectedBoardCardIter != affectedBoardCardIndices.end();)
    {
        const auto affectedStat = sAffectedStatTypeToCardStatType.at(mAffectedBoardCardsStatType);
        auto cardData = CardDataRepository::GetInstance().GetCardData(mBoardState->GetActivePlayerState().mPlayerBoardCards.at(*affectedBoardCardIter), mBoardState->GetActivePlayerIndex());
        auto currentValue = mAffectedBoardCardsStatType == AffectedStatType::DAMAGE ? cardData.mCardDamage : cardData.mCardWeight;
        
        if (static_cast<int>(mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides.size()) <= *affectedBoardCardIter)
        {
            mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides.resize(*affectedBoardCardIter + 1);
        }
        
        if (mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides[*affectedBoardCardIter].count(affectedStat) == 0)
        {
            mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides[*affectedBoardCardIter][affectedStat] = 0;
        }
        else
        {
            currentValue = mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides[*affectedBoardCardIter][affectedStat];
        }
        
        mGameActionEngine->AddGameAction(CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME,
        {
            { CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM, std::to_string(*affectedBoardCardIter)},
            { CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())},
            { CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM, "true" },
            { CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM, std::to_string(CARD_SCALE_UP_FACTOR) },
            { CardBuffedDebuffedAnimationGameAction::CARD_BUFFED_REPEAT_INDEX, std::to_string(particleEmitterIndex) },
            { CardBuffedDebuffedAnimationGameAction::PARTICLE_EMITTER_NAME_TO_REMOVE_PARAM, BUFFED_CARD_PARTICLE_EMITTER_NAME_PREFIX + std::to_string(particleEmitterIndex++) }
            
        });
        
        mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides[*affectedBoardCardIter][affectedStat] = currentValue + mEffectValue;
        affectedBoardCardIter++;
    }
    
    // Create/Modify held card stat overrides
    for (auto affectedHeldCardIter = affectedHeldCardIndices.begin(); affectedHeldCardIter != affectedHeldCardIndices.end();)
    {
        if (mCardBoardEffectMask != effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION)
        {
            const auto affectedStat = sAffectedStatTypeToCardStatType.at(mAffectedBoardCardsStatType);
            auto cardData = CardDataRepository::GetInstance().GetCardData(mBoardState->GetActivePlayerState().mPlayerHeldCards.at(*affectedHeldCardIter), mBoardState->GetActivePlayerIndex());
            auto currentValue = mAffectedBoardCardsStatType == AffectedStatType::DAMAGE ? cardData.mCardDamage : cardData.mCardWeight;
            
            if (static_cast<int>(mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.size()) <= *affectedHeldCardIter)
            {
                mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.resize(*affectedHeldCardIter + 1);
            }
            
            if (mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[*affectedHeldCardIter].count(affectedStat) == 0)
            {
                mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[*affectedHeldCardIter][affectedStat] = 0;
            }
            else
            {
                currentValue = mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[*affectedHeldCardIter][affectedStat];
            }
            
            mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[*affectedHeldCardIter][affectedStat] = currentValue + mEffectValue;
        }
        
        // Skip animation for held cards for opponent
        if (mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX)
        {
            affectedHeldCardIter = affectedHeldCardIndices.erase(affectedHeldCardIter);
            continue;
        }
        
        mGameActionEngine->AddGameAction(CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME,
        {
            { CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM, std::to_string(*affectedHeldCardIter)},
            { CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())},
            { CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM, "false" },
            { CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM, std::to_string(CARD_SCALE_UP_FACTOR) },
            { CardBuffedDebuffedAnimationGameAction::CARD_BUFFED_REPEAT_INDEX, std::to_string(particleEmitterIndex) },
            { CardBuffedDebuffedAnimationGameAction::PARTICLE_EMITTER_NAME_TO_REMOVE_PARAM, BUFFED_CARD_PARTICLE_EMITTER_NAME_PREFIX + std::to_string(particleEmitterIndex++) }
        });
        
        affectedHeldCardIter++;
    }
    
    // Draw effect
    if (std::find(mEffectComponents.cbegin(), mEffectComponents.cend(), effects::EFFECT_COMPONENT_DRAW) != mEffectComponents.cend())
    {
        for (auto i = 0; i < mEffectValue; ++i)
        {
            mGameActionEngine->AddGameAction(DRAW_CARD_GAME_ACTION_NAME);
        }
    }
    
    // For non-headless behavior
    if (mBattleSceneLogicManager)
    {
        for (auto i = 0; i < static_cast<int>(affectedBoardCardIndices.size()); ++i)
        {
            mAffectedCards.push_back({mBattleSceneLogicManager->GetBoardCardSoWrappers()[mBoardState->GetActivePlayerIndex()].at(affectedBoardCardIndices.at(i)), affectedBoardCardIndices.at(i), true});
        }
        
        for (auto i = 0; i < static_cast<int>(affectedHeldCardIndices.size()); ++i)
        {
            mAffectedCards.push_back({mBattleSceneLogicManager->GetHeldCardSoWrappers()[mBoardState->GetActivePlayerIndex()].at(affectedHeldCardIndices.at(i)), affectedHeldCardIndices.at(i), false});
        }
    }
}

///------------------------------------------------------------------------------------------------
