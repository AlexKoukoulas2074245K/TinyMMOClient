///------------------------------------------------------------------------------------------------
///  CardUtils.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 10/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef CardUtils_h
#define CardUtils_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <game/BoardState.h>
#include <game/Cards.h>
#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene { class Scene; }
namespace scene { struct SceneObject; }
namespace rendering { class Camera; }
struct CardData;

///------------------------------------------------------------------------------------------------

namespace card_utils
{

///------------------------------------------------------------------------------------------------

int CalculateNonDeadCardsCount(const std::vector<int>& cards, const std::unordered_set<int>& deadIndices);

///------------------------------------------------------------------------------------------------

glm::vec3 CalculateHeldCardPosition(const int cardIndex, const int playerCardCount, bool forRemotePlayer, const rendering::Camera& camera);

///------------------------------------------------------------------------------------------------

glm::vec3 CalculateBoardCardPosition(const int cardIndex, const int playerCardCount, bool forRemotePlayer);

///------------------------------------------------------------------------------------------------

CardRarity GetCardRarity(const int cardId, const size_t forPlayerIndex, const BoardState& boardState);

///------------------------------------------------------------------------------------------------

std::shared_ptr<CardSoWrapper> CreateCardSoWrapper
(
    const CardData* cardData,
    const glm::vec3& position,
    const std::string& cardNamePrefix,
    const CardOrientation cardOrientation,
    const CardRarity cardRarity,
    const bool isOnBoard,
    const bool forRemotePlayer,
    const bool canCardBePlayed,
    const CardStatOverrides& cardStatOverrides,
    const CardStatOverrides& globalStatModifiers,
    scene::Scene& scene,
    const std::string& exportToFilePath = std::string()
);

///------------------------------------------------------------------------------------------------

void PlayCardPlaySfx(const CardData* cardData);

///------------------------------------------------------------------------------------------------

void PlayCardAttackSfx(const int pendingDamage, const int amountOfArmorDamaged);

///------------------------------------------------------------------------------------------------

void ExportCardData(const strutils::StringId& expansionId, std::shared_ptr<scene::Scene> scene);

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* CardUtils_h */
