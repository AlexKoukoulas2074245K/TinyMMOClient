///------------------------------------------------------------------------------------------------
///  CardUtils.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 10/10/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/rendering/RenderingUtils.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/sound/SoundManager.h>
#include <engine/utils/OSMessageBox.h>
#include <game/CardUtils.h>
#include <game/DataRepository.h>
#include <fstream>
#include <engine/utils/FileUtils.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/utils/StringUtils.h>
#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif
#include <filesystem>
#include <SDL.h>

///------------------------------------------------------------------------------------------------

namespace card_utils
{

///------------------------------------------------------------------------------------------------

static const std::string CARD_BACK_TEXTURE_FILE_NAME = "card_back.png";
static const std::string DORMANT_CARD_MASK_TEXTURE_FILE_NAME = "card_dormant_mask.png";
static const std::string POISON_CRYSTAL_TEXTURE_FILE_NAME = "poison_crystal.png";
static const std::string SINGLE_USE_CARD_TEXTURE_FILE_NAME = "single_use_stamp.png";
static const std::string DIG_ICON_TEXTURE_FILE_NAME = "dig_icon.png";
static const std::string CARD_SHADER_FILE_NAME = "card.vs";
static const std::string CARD_DAMAGE_ICON_TEXTURE_FILE_NAME = "damage_icon.png";
static const std::string CARD_WEIGHT_ICON_TEXTURE_FILE_NAME = "feather_icon.png";
static const std::string GENERATED_R2T_NAME_PREFIX = "generated_card_texture_player_";
static const std::string CARD_PLAY_SFX = "sfx_card_play";
static const std::string CARD_LIGHT_ATTACK_SFX = "sfx_light_attack";
static const std::string CARD_MEDIUM_ATTACK_SFX = "sfx_medium_attack";
static const std::string CARD_HEAVY_ATTACK_SFX = "sfx_heavy_attack";
static const std::string CARD_SHIELD_ATTACK_SFX = "sfx_shield";

static const glm::vec3 RENDER_TO_TEXTURE_UPSCALE_FACTOR = {-1.365f, 1.256f, 1.0f};

static const float CARD_NAME_AREA_LENGTH = 0.042f;
static const float CARD_NAME_TEST_DEDUCT_INCREMENTS = 0.00001f;
static const float CARD_INDEX_Z_OFFSET = 1.0f;
static const float BARD_CARD_POSITION_Z_OFFSET = 0.01f;
static const float DOUBLE_DIGIT_STAT_X_OFFSET = 0.003f;

///------------------------------------------------------------------------------------------------

static float GetZoomVariableHeldCardY(const float zoomFactor)
{
    return 0.0000070f * (zoomFactor * zoomFactor) - 0.0004989f * zoomFactor - 0.1645f;
}

///------------------------------------------------------------------------------------------------

int CalculateNonDeadCardsCount(const std::vector<int>& cards, const std::unordered_set<int>& deadIndices)
{
    return static_cast<int>(cards.size()) - static_cast<int>(deadIndices.size());
}

///------------------------------------------------------------------------------------------------

glm::vec3 CalculateHeldCardPosition(const int cardIndex, const int playerCardCount, bool forRemotePlayer, const rendering::Camera& camera)
{
    float cardBlockWidth = game_constants::IN_GAME_CARD_WIDTH * playerCardCount;
    float cardStartX = cardBlockWidth/2.0f;
    
    auto targetX = cardStartX - cardIndex * game_constants::IN_GAME_CARD_WIDTH - game_constants::IN_GAME_CARD_WIDTH/2;
    if (playerCardCount > game_constants::IN_GAME_CARD_PUSH_THRESHOLD)
    {
        float pushX = (playerCardCount - game_constants::IN_GAME_CARD_PUSH_THRESHOLD) * game_constants::IN_GAME_CARD_PUSH_VALUE * (math::Abs(cardIndex - playerCardCount/2));
        bool oddCardCount = playerCardCount % 2 != 0;
        if ((oddCardCount && cardIndex != playerCardCount/2) || !oddCardCount)
        {
            targetX -= (cardIndex < playerCardCount/2) ? pushX : -pushX;
        }
    }
    
    auto zoomVariableY = GetZoomVariableHeldCardY(camera.GetZoomFactor());
    return glm::vec3(targetX, forRemotePlayer ? -zoomVariableY : zoomVariableY, game_constants::IN_GAME_HELD_CARD_Z + cardIndex * CARD_INDEX_Z_OFFSET);
}

///------------------------------------------------------------------------------------------------

glm::vec3 CalculateBoardCardPosition(const int cardIndex, const int playerCardCount, bool forRemotePlayer)
{
    float cardBlockWidth = game_constants::IN_GAME_CARD_ON_BOARD_WIDTH * playerCardCount;
    float cardStartX = -cardBlockWidth/2.0f;
    
    auto targetX = cardStartX + cardIndex * game_constants::IN_GAME_CARD_ON_BOARD_WIDTH + game_constants::IN_GAME_CARD_ON_BOARD_WIDTH/2;
    if (playerCardCount > game_constants::IN_GAME_CARD_PUSH_THRESHOLD)
    {
        float pushX = (playerCardCount - game_constants::IN_GAME_CARD_PUSH_THRESHOLD) * game_constants::IN_GAME_CARD_PUSH_VALUE * (math::Abs(cardIndex - playerCardCount/2));
        bool oddCardCount = playerCardCount % 2 != 0;
        if ((oddCardCount && cardIndex != playerCardCount/2) || !oddCardCount)
        {
            targetX += (cardIndex < playerCardCount/2) ? pushX : -pushX;
        }
    }
    
    return glm::vec3(targetX, forRemotePlayer ? game_constants::IN_GAME_TOP_PLAYER_BOARD_CARD_Y : game_constants::IN_GAME_BOT_PLAYER_BOARD_CARD_Y, game_constants::IN_GAME_PLAYED_CARD_Z + cardIndex * BARD_CARD_POSITION_Z_OFFSET);
}

///------------------------------------------------------------------------------------------------

CardRarity GetCardRarity(const int cardId, const size_t forPlayerIndex, const BoardState& boardState)
{
    return std::find
    (
     boardState.GetPlayerStates()[forPlayerIndex].mGoldenCardIds.cbegin(),
     boardState.GetPlayerStates()[forPlayerIndex].mGoldenCardIds.cend(),
     cardId
     ) != boardState.GetPlayerStates()[forPlayerIndex].mGoldenCardIds.cend() ? CardRarity::GOLDEN : CardRarity::NORMAL;
}

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
    const std::string& exportToFilePath /* = std::string() */
)
{
    auto cardSoWrapper = std::make_shared<CardSoWrapper>();
    
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& resService = systemsEngine.GetResourceLoadingService();
    
    const auto& sceneObjectName = strutils::StringId(cardNamePrefix);
    
    if (cardOrientation == CardOrientation::BACK_FACE)
    {
        // Create card back
        cardSoWrapper->mSceneObject = scene.CreateSceneObject(sceneObjectName);
        cardSoWrapper->mSceneObject->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CARD_BACK_TEXTURE_FILE_NAME);
        cardSoWrapper->mSceneObject->mScale.x = cardSoWrapper->mSceneObject->mScale.y = game_constants::IN_GAME_CARD_BASE_SCALE;
        cardSoWrapper->mSceneObject->mBoundingRectMultiplier.x = game_constants::CARD_BOUNDING_RECT_X_MULTIPLIER;
        cardSoWrapper->mSceneObject->mPosition = position;
        cardSoWrapper->mSceneObject->mShaderBoolUniformValues[game_constants::IS_GOLDEN_CARD_UNIFORM_NAME] = false;
        cardSoWrapper->mSceneObject->mShaderBoolUniformValues[game_constants::IS_HELD_CARD_UNIFORM_NAME] = true;
    }
    else
    {
        assert(cardData);
        
        // Create card base
        std::vector<std::shared_ptr<scene::SceneObject>> cardComponents;
        cardComponents.push_back(scene.CreateSceneObject(sceneObjectName));
        if (cardRarity == CardRarity::GOLDEN)
        {
            cardComponents.back()->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + game_constants::GOLDEN_CARD_TEXTURE_FILE_NAME);
        }
        else
        {
            cardComponents.back()->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + (cardData->IsSpell() ? game_constants::CARD_FRAME_SPELL_TEXTURE_FILE_NAME : game_constants::CARD_FRAME_NORMAL_TEXTURE_FILE_NAME));
        }
        
        cardComponents.back()->mScale.x = cardComponents.back()->mScale.y = game_constants::IN_GAME_CARD_BASE_SCALE;
        cardComponents.back()->mBoundingRectMultiplier.x = game_constants::CARD_BOUNDING_RECT_X_MULTIPLIER;
        cardComponents.back()->mPosition = position;
        cardComponents.back()->mRotation.z = math::PI;
        
        // Create portrait
        cardComponents.push_back(std::make_shared<scene::SceneObject>());
        cardComponents.back()->mTextureResourceId = cardData->mCardTextureResourceId;
        cardComponents.back()->mShaderResourceId = cardData->mCardShaderResourceId;
        cardComponents.back()->mScale.x = cardComponents.back()->mScale.y = game_constants::IN_GAME_CARD_PORTRAIT_SCALE;
        cardComponents.back()->mBoundingRectMultiplier.x = game_constants::CARD_BOUNDING_RECT_X_MULTIPLIER;
        cardComponents.back()->mPosition = position;
        cardComponents.back()->mPosition.y += game_constants::IN_GAME_CARD_PORTRAIT_Y_OFFSET;
        cardComponents.back()->mPosition.z += game_constants::CARD_COMPONENT_Z_OFFSET;
        
        int weight = 0;
        int damage = 0;
        
        if (cardData->IsSpell())
        {
            // Create weight icon
            cardComponents.push_back(std::make_shared<scene::SceneObject>());
            cardComponents.back()->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CARD_WEIGHT_ICON_TEXTURE_FILE_NAME);
            cardComponents.back()->mScale.x = cardComponents.back()->mScale.y = game_constants::IN_GAME_CARD_PROPERTY_ICON_SCALE;
            cardComponents.back()->mBoundingRectMultiplier.x = game_constants::CARD_BOUNDING_RECT_X_MULTIPLIER;
            cardComponents.back()->mPosition = position;
            cardComponents.back()->mPosition.x += game_constants::IN_GAME_CARD_SPELL_PROPERTY_ICON_X_OFFSET;
            cardComponents.back()->mPosition.y += game_constants::IN_GAME_CARD_SPELL_PROPERTY_ICON_Y_OFFSET;
            cardComponents.back()->mPosition.z += 2 * game_constants::CARD_COMPONENT_Z_OFFSET;
            
            // Create weight
            cardComponents.push_back(std::make_shared<scene::SceneObject>());
            scene::TextSceneObjectData weightTextData;
            weightTextData.mFontName = game_constants::FONT_PLACEHOLDER_WEIGHT_NAME;
            
            weight = math::Max(0, cardStatOverrides.count(CardStatType::WEIGHT) ? cardStatOverrides.at(CardStatType::WEIGHT) : cardData->mCardWeight);
//            if (globalStatModifiers.count(CardStatType::WEIGHT))
//            {
//                weight = math::Max(0, weight + globalStatModifiers.at(CardStatType::WEIGHT));
//            }
            weightTextData.mText = std::to_string(weight);
            
            cardComponents.back()->mSceneObjectTypeData = std::move(weightTextData);
            cardComponents.back()->mScale = glm::vec3(game_constants::IN_GAME_CARD_PROPERTY_SCALE);
            cardComponents.back()->mPosition = position;
            cardComponents.back()->mPosition.x -= game_constants::IN_GAME_CARD_SPELL_PROPERTY_ICON_X_OFFSET - (cardData->mCardWeight >= 10 ? DOUBLE_DIGIT_STAT_X_OFFSET : 0.0f);
            cardComponents.back()->mPosition.y += game_constants::IN_GAME_CARD_SPELL_PROPERTY_ICON_Y_OFFSET;
            cardComponents.back()->mPosition.z += 3 * game_constants::CARD_COMPONENT_Z_OFFSET;
        }
        else
        {
            // Create damage icon
            cardComponents.push_back(std::make_shared<scene::SceneObject>());
            cardComponents.back()->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CARD_DAMAGE_ICON_TEXTURE_FILE_NAME);
            cardComponents.back()->mScale.x = cardComponents.back()->mScale.y = game_constants::IN_GAME_CARD_PROPERTY_ICON_SCALE;
            cardComponents.back()->mBoundingRectMultiplier.x = game_constants::CARD_BOUNDING_RECT_X_MULTIPLIER;
            cardComponents.back()->mPosition = position;
            cardComponents.back()->mPosition.x += game_constants::IN_GAME_CARD_PROPERTY_ICON_X_OFFSET;
            cardComponents.back()->mPosition.y += game_constants::IN_GAME_CARD_PROPERTY_ICON_Y_OFFSET;
            cardComponents.back()->mPosition.z += 2 * game_constants::CARD_COMPONENT_Z_OFFSET;
            
            // Create damage
            cardComponents.push_back(std::make_shared<scene::SceneObject>());
            scene::TextSceneObjectData damageTextData;
            damageTextData.mFontName = game_constants::FONT_PLACEHOLDER_DAMAGE_NAME;
            
            damage = math::Max(0, cardStatOverrides.count(CardStatType::DAMAGE) ? cardStatOverrides.at(CardStatType::DAMAGE) : cardData->mCardDamage);
            if (isOnBoard && globalStatModifiers.count(CardStatType::DAMAGE))
            {
                damage = math::Max(0, damage + globalStatModifiers.at(CardStatType::DAMAGE));
            }
            damageTextData.mText = std::to_string(damage);
            cardComponents.back()->mSceneObjectTypeData = std::move(damageTextData);
            cardComponents.back()->mScale = glm::vec3(game_constants::IN_GAME_CARD_PROPERTY_SCALE);
            cardComponents.back()->mPosition = position;
            cardComponents.back()->mPosition.x += game_constants::IN_GAME_CARD_PROPERTY_X_OFFSET - (damage >= 10 ? DOUBLE_DIGIT_STAT_X_OFFSET : 0.0f);
            cardComponents.back()->mPosition.y += game_constants::IN_GAME_CARD_PROPERTY_Y_OFFSET;
            cardComponents.back()->mPosition.z += 3 * game_constants::CARD_COMPONENT_Z_OFFSET;
            
            // Create poison indicator
            if (cardData->mCardFamily == game_constants::INSECTS_FAMILY_NAME)
            {
                cardComponents.push_back(std::make_shared<scene::SceneObject>());
                cardComponents.back()->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + POISON_CRYSTAL_TEXTURE_FILE_NAME);
                cardComponents.back()->mScale.x = cardComponents.back()->mScale.y = game_constants::IN_GAME_CARD_PROPERTY_ICON_SCALE/2;
                cardComponents.back()->mBoundingRectMultiplier.x = game_constants::CARD_BOUNDING_RECT_X_MULTIPLIER;
                cardComponents.back()->mPosition = position;
                cardComponents.back()->mPosition.y += game_constants::IN_GAME_CARD_PROPERTY_Y_OFFSET;
                cardComponents.back()->mPosition.z += 2 * game_constants::CARD_COMPONENT_Z_OFFSET;
            }
            // Create dig indicator
            else if (cardData->mCardFamily == game_constants::RODENTS_FAMILY_NAME)
            {
                cardComponents.push_back(std::make_shared<scene::SceneObject>());
                cardComponents.back()->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + DIG_ICON_TEXTURE_FILE_NAME);
                cardComponents.back()->mScale.x = cardComponents.back()->mScale.y = game_constants::IN_GAME_CARD_PROPERTY_ICON_SCALE/4;
                cardComponents.back()->mBoundingRectMultiplier.x = game_constants::CARD_BOUNDING_RECT_X_MULTIPLIER;
                cardComponents.back()->mPosition = position;
                cardComponents.back()->mPosition.y += game_constants::IN_GAME_CARD_PROPERTY_Y_OFFSET;
                cardComponents.back()->mPosition.z += 2 * game_constants::CARD_COMPONENT_Z_OFFSET;
            }
            
            // Create weight icon
            cardComponents.push_back(std::make_shared<scene::SceneObject>());
            cardComponents.back()->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CARD_WEIGHT_ICON_TEXTURE_FILE_NAME);
            cardComponents.back()->mScale.x = cardComponents.back()->mScale.y = game_constants::IN_GAME_CARD_PROPERTY_ICON_SCALE;
            cardComponents.back()->mBoundingRectMultiplier.x = game_constants::CARD_BOUNDING_RECT_X_MULTIPLIER;
            cardComponents.back()->mPosition = position;
            cardComponents.back()->mPosition.x -= game_constants::IN_GAME_CARD_PROPERTY_ICON_X_OFFSET;
            cardComponents.back()->mPosition.y += game_constants::IN_GAME_CARD_PROPERTY_ICON_Y_OFFSET;
            cardComponents.back()->mPosition.z += 2 * game_constants::CARD_COMPONENT_Z_OFFSET;
            
            // Create weight
            cardComponents.push_back(std::make_shared<scene::SceneObject>());
            scene::TextSceneObjectData weightTextData;
            weightTextData.mFontName = game_constants::FONT_PLACEHOLDER_WEIGHT_NAME;
            
            weight = math::Max(0, cardStatOverrides.count(CardStatType::WEIGHT) ? cardStatOverrides.at(CardStatType::WEIGHT) : cardData->mCardWeight);
            if (globalStatModifiers.count(CardStatType::WEIGHT))
            {
                weight = math::Max(0, weight + globalStatModifiers.at(CardStatType::WEIGHT));
            }
            weightTextData.mText = std::to_string(weight);
            
            cardComponents.back()->mSceneObjectTypeData = std::move(weightTextData);
            cardComponents.back()->mScale = glm::vec3(game_constants::IN_GAME_CARD_PROPERTY_SCALE);
            cardComponents.back()->mPosition = position;
            cardComponents.back()->mPosition.x -= game_constants::IN_GAME_CARD_PROPERTY_X_OFFSET + (weight >= 10 ? DOUBLE_DIGIT_STAT_X_OFFSET : 0.0f);
            cardComponents.back()->mPosition.y += game_constants::IN_GAME_CARD_PROPERTY_Y_OFFSET;
            cardComponents.back()->mPosition.z += 3 * game_constants::CARD_COMPONENT_Z_OFFSET;
        }
        
        // Create card name
        cardComponents.push_back(std::make_shared<scene::SceneObject>());
        scene::TextSceneObjectData cardNameTextData;
        cardNameTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
        cardNameTextData.mText = cardData->mCardName.GetString();
        cardComponents.back()->mSceneObjectTypeData = std::move(cardNameTextData);
        
        float scaleDeduct = CARD_NAME_TEST_DEDUCT_INCREMENTS;
        float textLength = 0.0f;
        do
        {
            scaleDeduct += CARD_NAME_TEST_DEDUCT_INCREMENTS;
            cardComponents.back()->mScale = glm::vec3(game_constants::IN_GAME_CARD_NAME_SCALE - scaleDeduct);
            cardComponents.back()->mPosition = position + game_constants::IN_GAME_CARD_NAME_X_OFFSET;
            auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*cardComponents.back());
            textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
            cardComponents.back()->mPosition.x -= textLength/2.0f;
        } while (textLength > CARD_NAME_AREA_LENGTH);
        
        cardComponents.back()->mPosition.y += game_constants::IN_GAME_CARD_NAME_Y_OFFSET;
        cardComponents.back()->mPosition.z += game_constants::CARD_COMPONENT_Z_OFFSET;
        
        // Single use card
        if (cardData->mIsSingleUse)
        {
            cardComponents.push_back(std::make_shared<scene::SceneObject>());
            cardComponents.back()->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SINGLE_USE_CARD_TEXTURE_FILE_NAME);
            cardComponents.back()->mScale.x = cardComponents.back()->mScale.y = game_constants::IN_GAME_CARD_PROPERTY_ICON_SCALE/2;
            cardComponents.back()->mBoundingRectMultiplier.x = game_constants::CARD_BOUNDING_RECT_X_MULTIPLIER;
            cardComponents.back()->mPosition = position;
            cardComponents.back()->mPosition.x -= game_constants::IN_GAME_CARD_PROPERTY_X_OFFSET;
            cardComponents.back()->mPosition.z += 3 * game_constants::CARD_COMPONENT_Z_OFFSET;
        }
        
        
        std::stringstream generatedTextureOverridePostfixSS;
        if (!cardStatOverrides.empty())
        {
            generatedTextureOverridePostfixSS << "_overrides_";
            bool seenFirstEntry = false;
            for (const auto& entry: cardStatOverrides)
            {
                if (seenFirstEntry)
                {
                    generatedTextureOverridePostfixSS << ", ";
                }
                else
                {
                    seenFirstEntry = true;
                }
                
                generatedTextureOverridePostfixSS << static_cast<int>(entry.first);
                generatedTextureOverridePostfixSS << "=";
                generatedTextureOverridePostfixSS << entry.second;
            }
        }
        
        if (isOnBoard && globalStatModifiers.count(CardStatType::DAMAGE))
        {
            generatedTextureOverridePostfixSS << "_global_damage_" << globalStatModifiers.at(CardStatType::DAMAGE);
        }
        
        if (globalStatModifiers.count(CardStatType::WEIGHT))
        {
            generatedTextureOverridePostfixSS << "_global_" << (isOnBoard ? "on_board_" : "held_") << "weight_" << globalStatModifiers.at(CardStatType::WEIGHT);
        }
        
        if (DataRepository::GetInstance().IsCurrentlyPlayingStoryMode())
        {
            const auto& storyPlayerCardStatModifiers = DataRepository::GetInstance().GetStoryPlayerCardStatModifiers();
            if (storyPlayerCardStatModifiers.count(CardStatType::DAMAGE))
            {
                generatedTextureOverridePostfixSS << "_story_modifier_damage_" << storyPlayerCardStatModifiers.at(CardStatType::DAMAGE);
            }
            if (storyPlayerCardStatModifiers.count(CardStatType::WEIGHT))
            {
                generatedTextureOverridePostfixSS << "_story_modifier_weight_" << storyPlayerCardStatModifiers.at(CardStatType::WEIGHT);
            }
        }
        
        if (cardRarity == CardRarity::GOLDEN)
        {
            generatedTextureOverridePostfixSS << "_golden";
        }
        
        generatedTextureOverridePostfixSS << "_damage_" << damage << "_weight_" << weight;
        
        rendering::CollateSceneObjectsIntoOne(GENERATED_R2T_NAME_PREFIX + (forRemotePlayer ? "0_id_" : "1_id_") + std::to_string(cardData->mCardId) + generatedTextureOverridePostfixSS.str(), position, cardComponents, "", scene);
        cardComponents.front()->mShaderResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_SHADER_FILE_NAME);
        
        weight = math::Max(0, cardStatOverrides.count(CardStatType::WEIGHT) ? cardStatOverrides.at(CardStatType::WEIGHT) : cardData->mCardWeight);
        if (!cardData->IsSpell())
        {
            if (globalStatModifiers.count(CardStatType::WEIGHT))
            {
                weight = math::Max(0, weight + globalStatModifiers.at(CardStatType::WEIGHT));
            }
        }
        
        cardComponents.front()->mShaderIntUniformValues[game_constants::CARD_WEIGHT_INTERACTIVE_MODE_UNIFORM_NAME] = canCardBePlayed ? (weight < cardData->mCardWeight ? game_constants::CARD_INTERACTIVE_MODE_INTERACTIVE : game_constants::CARD_INTERACTIVE_MODE_DEFAULT) : game_constants::CARD_INTERACTIVE_MODE_NONINTERACTIVE;
        
        damage = math::Max(0, cardStatOverrides.count(CardStatType::DAMAGE) ? cardStatOverrides.at(CardStatType::DAMAGE) : cardData->mCardDamage);
        if (isOnBoard && globalStatModifiers.count(CardStatType::DAMAGE))
        {
            damage = math::Max(0, damage + globalStatModifiers.at(CardStatType::DAMAGE));
        }
        
        if (damage > cardData->mCardDamage)
        {
            cardComponents.front()->mShaderIntUniformValues[game_constants::CARD_DAMAGE_INTERACTIVE_MODE_UNIFORM_NAME] = game_constants::CARD_INTERACTIVE_MODE_INTERACTIVE;
        }
        else if (damage == cardData->mCardDamage)
        {
            cardComponents.front()->mShaderIntUniformValues[game_constants::CARD_DAMAGE_INTERACTIVE_MODE_UNIFORM_NAME] = game_constants::CARD_INTERACTIVE_MODE_DEFAULT;
        }
        else
        {
            cardComponents.front()->mShaderIntUniformValues[game_constants::CARD_DAMAGE_INTERACTIVE_MODE_UNIFORM_NAME] = game_constants::CARD_INTERACTIVE_MODE_NONINTERACTIVE;
        }
        
        cardComponents.front()->mPosition += position;
        cardComponents.front()->mScale *= RENDER_TO_TEXTURE_UPSCALE_FACTOR;
        
        cardComponents.front()->mShaderBoolUniformValues[game_constants::IS_GOLDEN_CARD_UNIFORM_NAME] = cardRarity == CardRarity::GOLDEN;
        cardComponents.front()->mShaderBoolUniformValues[game_constants::IS_HELD_CARD_UNIFORM_NAME] = !isOnBoard;
        cardComponents.front()->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME] = -1.0f;
        cardComponents.front()->mShaderFloatUniformValues[game_constants::DORMANT_CARD_VALUE_UNIFORM_NAME] = 0.0f;
        cardComponents.front()->mEffectTextureResourceIds[0] = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + (cardData->IsSpell() ? game_constants::GOLDEN_SPELL_CARD_FLAKES_MASK_TEXTURE_FILE_NAME : game_constants::GOLDEN_CARD_FLAKES_MASK_TEXTURE_FILE_NAME));
        cardComponents.front()->mEffectTextureResourceIds[1] = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + DORMANT_CARD_MASK_TEXTURE_FILE_NAME);
        cardSoWrapper->mSceneObject = cardComponents.front();
        
        if (!exportToFilePath.empty())
        {
            cardComponents.front()->mScale.x *= 2.0f;
            cardComponents.front()->mScale.y *= -1.0f;
            rendering::CollateSceneObjectsIntoOne(GENERATED_R2T_NAME_PREFIX + (forRemotePlayer ? "0_id_" : "1_id_") + std::to_string(cardData->mCardId) + generatedTextureOverridePostfixSS.str(), position, cardComponents, exportToFilePath, scene);
        }
    }
    
    cardSoWrapper->mCardData = *cardData;
    
    return cardSoWrapper;
}

///------------------------------------------------------------------------------------------------

void PlayCardPlaySfx(const CardData* cardData)
{
    CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(CARD_PLAY_SFX);
}

///------------------------------------------------------------------------------------------------

void PlayCardAttackSfx(const int pendingDamage, const int amountOfArmorDamaged)
{
    if (amountOfArmorDamaged > 0)
    {
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(CARD_SHIELD_ATTACK_SFX);
        return;
    }
    
    if (pendingDamage < 5)
    {
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(CARD_PLAY_SFX);
    }
    else if (pendingDamage < 10)
    {
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(CARD_PLAY_SFX, false, 1.8f);
    }
    else
    {
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(CARD_PLAY_SFX, false, 2.6f);
    }
}

///------------------------------------------------------------------------------------------------

void ExportCardData(const strutils::StringId& expansionId, std::shared_ptr<scene::Scene> scene)
{
    CardDataRepository::GetInstance().LoadCardData(true);
    
    // Collect all card IDs
    auto cardIdsToExport = CardDataRepository::GetInstance().GetCardIdsByFamily(game_constants::RODENTS_FAMILY_NAME);
    auto dinosaurCardIds = CardDataRepository::GetInstance().GetCardIdsByFamily(game_constants::DINOSAURS_FAMILY_NAME);
    auto insectCardIds = CardDataRepository::GetInstance().GetCardIdsByFamily(game_constants::INSECTS_FAMILY_NAME);
    
    cardIdsToExport.insert(cardIdsToExport.end(), dinosaurCardIds.begin(), dinosaurCardIds.end());
    cardIdsToExport.insert(cardIdsToExport.end(), insectCardIds.begin(), insectCardIds.end());
    
    // Sort them
    std::sort(cardIdsToExport.begin(), cardIdsToExport.end(), [](const int& lhs, const int& rhs)
    {
        const auto& lhsCardData = CardDataRepository::GetInstance().GetCardData(lhs, game_constants::LOCAL_PLAYER_INDEX);
        const auto& rhsCardData = CardDataRepository::GetInstance().GetCardData(rhs, game_constants::LOCAL_PLAYER_INDEX);
        
        if ((lhsCardData.IsSpell() && rhsCardData.IsSpell()) || (!lhsCardData.IsSpell() && !rhsCardData.IsSpell()))
        {
            return lhsCardData.mCardWeight < rhsCardData.mCardWeight;
        }
        else
        {
            return !lhsCardData.IsSpell();
        }
    });
    
    // Filter out cards not in selected expansion
    cardIdsToExport.erase(std::remove_if(cardIdsToExport.begin(), cardIdsToExport.end(), [=](const int cardId)
    {
        return CardDataRepository::GetInstance().GetCardData(cardId, game_constants::REMOTE_PLAYER_INDEX).mExpansion != expansionId;
    }), cardIdsToExport.end());
    
    // Delete existing exported cards
#if defined(MACOS) || defined(MOBILE_FLOW)
    const auto existingExportedFiles = fileutils::GetAllFilenamesInDirectory(apple_utils::GetPersistentDataDirectoryPath() + "card_exports/");
    for (const auto& file: existingExportedFiles)
    {
        std::remove((apple_utils::GetPersistentDataDirectoryPath() + "card_exports/" + file).c_str());
    }
#endif
    
    for (auto i = 0; i < cardIdsToExport.size(); ++i)
    {
        const auto& cardData = CardDataRepository::GetInstance().GetCardData(cardIdsToExport[i], false);
        auto cardEffectTooltip = cardData.mCardEffectTooltip;
        if (cardData.IsSpell())
        {
            if (cardEffectTooltip[0] == ' ')
            {
                cardEffectTooltip = cardEffectTooltip.substr(1);
            }
            
            for (auto& c: cardEffectTooltip)
            {
                if (c == '$')
                {
                    c = ' ';
                }
            }
        }
        std::string fileName = "entry=" + std::to_string(i) + " name=" + cardData.mCardName.GetString() + " effect=" + cardEffectTooltip + ".png";
        
#if defined(MACOS) || defined(MOBILE_FLOW)
        CreateCardSoWrapper(&cardData, glm::vec3(0.0f), "test", CardOrientation::FRONT_FACE, CardRarity::NORMAL, false, true, true, {}, {}, *scene, apple_utils::GetPersistentDataDirectoryPath() + "card_exports/" + fileName);
#endif
    }
    
    ospopups::ShowMessageBox(ospopups::MessageBoxType::INFO, "Export Data Success", "Successfully export data for " + std::to_string(cardIdsToExport.size()) + " cards.");
}

///------------------------------------------------------------------------------------------------

}
