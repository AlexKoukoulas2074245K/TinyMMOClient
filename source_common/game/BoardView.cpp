

///------------------------------------------------------------------------------------------------
///  BoardView.cpp
///  Predators
///
///  Created by Alex Koukoulas on 28/02/2025
///------------------------------------------------------------------------------------------------

#include <game/BoardView.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/Animations.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/CommonUniforms.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <net_common/Board.h>
#include <net_common/Symbols.h>
#include <net_common/SymbolDataRepository.h>
#include <map>

///------------------------------------------------------------------------------------------------

static const strutils::StringId BOARD_NAME = strutils::StringId("board");
static const strutils::StringId INTERACTIVE_COLOR_THRESHOLD_UNIFORM_NAME = strutils::StringId("interactive_color_threshold");
static const strutils::StringId INTERACTIVE_COLOR_TIME_MULTIPLIER_UNIFORM_NAME = strutils::StringId("interactive_color_time_multiplier");
static const strutils::StringId SCATTER_EFFECT_MULTIPLIER_COEFF_UNIFORM_NAME = strutils::StringId("scatter_effect_stretch_multiplier");
static const strutils::StringId FRICTION_PARTICLE_DEFINITION_NAME = strutils::StringId("friction_particle");
static const strutils::StringId COMBO_SMOKE_PARTICLE_DEFINITION_NAME = strutils::StringId("combo_smoke");

static const std::string SYMBOL_SHADER_PATH = "symbol.vs";
static const std::string SYMBOL_FRAME_TEXTURE_PATH = "game/basket_frame.png";
static const std::string SHELVES_TEXTURE_PATH = "game/shelves.png";
static const std::string SCATTER_SYMBOL_EFFECT_TEXTURE_PATH = "game/food_slot_images/scatter_effect.png";
static const std::string SCATTER_BACKGROUND_MASK_TEXTURE_PATH = "game/food_slot_images/scatter_background_mask.png";
static const std::string FRICTION_EMITTER_NAME_PREFIX = "friction_emitter_";
static const std::string TUMBLE_TEMP_PREFIX = "tumbl_temp_";

static constexpr int FRICTION_EMITTER_COUNT = 6;

static const glm::vec2 TUMBLE_INGREDIENT_BEZIER_MIDPOINT_Y_POSITIONS = glm::vec2(-0.2f, 0.15f);
static const glm::vec3 BOARD_SCALE = glm::vec3(0.5f * 1.28f, 0.5f, 1.0f);
static const glm::vec3 SYMBOL_SCALE = glm::vec3(0.092f, 0.06624f, 1.0f);
static const glm::vec3 SYMBOL_FRAME_SCALE = glm::vec3(0.08f * 1.4f, 0.08f, 1.0f);
static const glm::vec3 SHELVES_POSITION = glm::vec3(0.0f, 0.0f, -0.2f);
static const glm::vec3 TOP_LEFT_SYMBOL_POSITION = glm::vec3(-0.2467f, 0.464f, 0.1f);
static const glm::vec3 FRICTION_PARTICLE_EMITTER_POSITIONS[FRICTION_EMITTER_COUNT] =
{
    glm::vec3(-0.06f, -0.140f, 1.5f), // Unused
    glm::vec3(-0.06f, -0.140f, 1.5f), // Unused
    glm::vec3(-0.074f, -0.198f, 1.5f),
    glm::vec3( 0.050f, -0.198f, 1.5f),
    glm::vec3( 0.174f, -0.198f, 1.5f),
    glm::vec3( 0.298f, -0.198f, 1.5f)
};

static const float HOR_SYMBOL_DISTANCE = 0.123f;
static const float VER_SYMBOL_DISTANCE = 0.116f;
static const float PRE_SPIN_Y_OFFSET = 0.04f;
static const float PRE_SPIN_ANIMATION_TIME = 0.15f;
static const float MAX_REEL_SPIN_SPEED = 0.001f;
static const float TIME_TO_REACH_MAX_REEL_SPIN_SPEED = 0.5f;
static const float SYMBOL_FRAME_Z_OFFSET = 0.01f;
static const float TIME_TILL_REEL_PENDING_SYMBOLS_UNLOCK = 1.0f;
static const float TIME_PER_REEL_SYMBOL_UNLOCK = 0.3f;
static const float TIME_TO_FINALIZE_SYMBOL_POSITION = 0.8f;
static const float TIME_DELAY_TO_BEGIN_WINNING_SYMBOLS_ANIMATION = 0.1f;
static const float INTERACTIVE_COLOR_THRESHOLD = 0.224f;
static const float INTERACTIVE_COLOR_TIME_MULTIPLIER = -0.7f;
static const float WINNING_SYMBOL_PULSE_SCALE_FACTOR = 1.2f;
static const float WINNING_SYMBOL_PULSE_ANIMATION_DURATION = 0.3f;
static const float WINNING_SYMBOL_PULSE_ANIMATION_DELAY = 0.3f;
static const float SCATTER_EFFECT_MULTIPLIER_COEFF = 0.02f;
static const float SCATTER_SUSPENSE_SLOWDOWN_MULTIPIER = 0.4f;
static const float SCATTER_SUSPENSE_EXTRA_SPIN_TIME = 2.0f;
static const float SCATTER_SLOWDOWN_KICKOFF_MULTIPLIER = 0.6666f;
static const float TUMBLE_COMBO_SYMBOL_Z = 2.0f;
static const float TUMBLE_SMOKE_PARTICLE_Z = 3.0f;
static const float TUMBLE_ANIMATION_DELAY_PER_COMBO_EVENT = 1.5f;
static const float TUMBLE_ANIMATION_DELAY_PER_REEL = 0.5f;
static const float TUMBLE_INGREDIENT_ANIMATION_TIME = 0.75f;
static const float TUMBLE_INGREDIENT_ANIMATION_DELAY = 0.2f;

static const std::unordered_map<slots::SymbolType, std::string> SYMBOL_TEXTURE_PATHS =
{
    { slots::SymbolType::BUTTER, "game/food_slot_images/butter.png" },
    { slots::SymbolType::CAMP_FIRE, "game/food_slot_images/camp_fire.png" },
    { slots::SymbolType::CHICKEN, "game/food_slot_images/chicken.png" },
    { slots::SymbolType::CHOCOLATE, "game/food_slot_images/chocolate.png" },
    { slots::SymbolType::COOKING_OIL, "game/food_slot_images/cooking_oil.png" },
    { slots::SymbolType::EGGS, "game/food_slot_images/eggs.png" },
    { slots::SymbolType::FLOUR, "game/food_slot_images/flour.png" },
    { slots::SymbolType::GARLICS, "game/food_slot_images/garlics.png" },
    { slots::SymbolType::LEMONS, "game/food_slot_images/lemons.png" },
    { slots::SymbolType::STRAWBERRIES, "game/food_slot_images/strawberries.png" },
    { slots::SymbolType::SUGAR, "game/food_slot_images/sugar.png" },
    { slots::SymbolType::WATER, "game/food_slot_images/water.png" },
    { slots::SymbolType::CHOCOLATE_CAKE, "game/food_slot_images/chocolate_cake.png" },
    { slots::SymbolType::STRAWBERRY_CAKE, "game/food_slot_images/strawberry_cake.png" },
    { slots::SymbolType::ROAST_CHICKEN, "game/food_slot_images/roast_chicken.png" },
    { slots::SymbolType::CHICKEN_SOUP, "game/food_slot_images/chicken_soup.png" },
    { slots::SymbolType::WILD, "game/food_slot_images/wild.png" },
    { slots::SymbolType::SCATTER, "game/food_slot_images/scatter.png" }
};

static const std::unordered_map<slots::SymbolType, std::string> SPECIAL_SYMBOL_SHADERS =
{
    { slots::SymbolType::WILD, "wild_symbol.vs" },
    { slots::SymbolType::SCATTER, "scatter_symbol.vs" }
};

static const std::unordered_map<BoardView::SpinAnimationState, std::string> SPIN_ANIMATION_STATE_NAMES =
{
    { BoardView::SpinAnimationState::IDLE, "IDLE" },
    { BoardView::SpinAnimationState::PRE_SPIN_LOADING, "PRE_SPIN_LOADING" },
    { BoardView::SpinAnimationState::SPINNING, "SPINNING" },
    { BoardView::SpinAnimationState::COMBO_PRE_TUMBLING, "COMBO_PRE_TUMBLING" },
    { BoardView::SpinAnimationState::TUMBLING, "TUMBLING" },
    { BoardView::SpinAnimationState::POST_SPINNING, "POST_SPINNING" },
    { BoardView::SpinAnimationState::WAITING_FOR_PAYLINES, "WAITING_FOR_PAYLINES" }
};

static const std::unordered_map<BoardView::PendingSymbolData::PendingSymbolDataState, std::string> PENDING_SYMBOL_DATA_STATE_NAMES =
{
    { BoardView::PendingSymbolData::PendingSymbolDataState::LOCKED, "LOCKED" },
    { BoardView::PendingSymbolData::PendingSymbolDataState::LOCKED_SUSPENSE, "LOCKED_SUSPENSE" },
    { BoardView::PendingSymbolData::PendingSymbolDataState::UNLOCKED, "UNLOCKED" },
    { BoardView::PendingSymbolData::PendingSymbolDataState::FINISHED, "FINISHED" }
};

///------------------------------------------------------------------------------------------------

static inline strutils::StringId GetSymbolSoName(const int row, const int col)
{
    return strutils::StringId(std::to_string(row) + "," + std::to_string(col) + "_symbol");
}

static inline strutils::StringId GetSymbolFrameSoName(const int row, const int col)
{
    return strutils::StringId(std::to_string(row) + "," + std::to_string(col) + "_symbol_frame");
}

static inline bool IsSceneObjectNameSymbolFrame(const strutils::StringId& sceneObjectName)
{
    return strutils::StringEndsWith(sceneObjectName.GetString(), "frame");
}

static inline std::vector<std::shared_ptr<scene::SceneObject>> FindAllSceneObjectsForSymbolCoordinates(const scene::Scene& scene, const int row, const int col)
{
    return scene.FindSceneObjectsWhoseNameStartsWith(std::to_string(row) + "," + std::to_string(col));
}

static inline slots::SymbolType LookupSceneObjectSymbolType(const resources::ResourceId textureResourceId)
{
    for (auto& symbolToTextureEntry: SYMBOL_TEXTURE_PATHS)
    {
        auto genResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResourceIdFromPath(resources::ResourceLoadingService::RES_TEXTURES_ROOT + symbolToTextureEntry.second, false);
        if (genResourceId == textureResourceId)
        {
            return symbolToTextureEntry.first;
        }
    }
    return slots::SymbolType::COUNT;
};

static inline std::vector<std::shared_ptr<scene::SceneObject>> FindAllSceneObjectsForSymbolCoordinatesWithSymbolType(const scene::Scene& scene, const int row, const int col, const slots::SymbolType symbolType)
{
    std::vector<std::shared_ptr<scene::SceneObject>> result;
    auto allSceneObjectsInCoord = scene.FindSceneObjectsWhoseNameStartsWith(std::to_string(row) + "," + std::to_string(col));
    
    for (auto sceneObject: allSceneObjectsInCoord)
    {
        if (IsSceneObjectNameSymbolFrame(sceneObject->mName))
        {
            result.push_back(sceneObject);
            break;
        }
    }
    
    for (auto sceneObject: allSceneObjectsInCoord)
    {
        if (LookupSceneObjectSymbolType(sceneObject->mTextureResourceId) == symbolType)
        {
            result.push_back(sceneObject);
            break;
        }
    }
    
    return result;
}

///------------------------------------------------------------------------------------------------

struct SymbolEntryNextPositionComparator
{
    bool operator()(const slots::SymbolEntryData& lhs, const slots::SymbolEntryData& rhs) const
    {
        if (lhs.mCol != rhs.mCol)
        {
            return lhs.mCol < rhs.mCol;
        }
        else
        {
            if (lhs.mRow != rhs.mRow)
            {
                return lhs.mRow > rhs.mRow;
            }
            else
            {
                return lhs.mSymbolType < rhs.mSymbolType;
            }
        }
    }
};

struct NextPositionEntry
{
    slots::SymbolEntryData mSymbolEntryData;
    glm::vec3 mSymbolPosition;
};

///------------------------------------------------------------------------------------------------

const std::string& BoardView::GetSymbolTexturePath(slots::SymbolType symbol)
{
    return SYMBOL_TEXTURE_PATHS.at(symbol);
}

///------------------------------------------------------------------------------------------------

BoardView::BoardView(scene::Scene& scene, const slots::Board& boardModel)
    : mScene(scene)
    , mBoardModel(boardModel)
    , mSpinAnimationState(SpinAnimationState::IDLE)
{
    auto board = scene.CreateSceneObject(BOARD_NAME);
    board->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + SHELVES_TEXTURE_PATH);
    board->mPosition = SHELVES_POSITION;
    board->mScale = BOARD_SCALE;
    board->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    
    for (auto i = 0; i < static_cast<int>(slots::PaylineType::PAYLINE_COUNT); ++i)
    {
        mPaylines.push_back(PaylineView(scene, static_cast<slots::PaylineType>(i)));
    }
    
    auto& particleManager = CoreSystemsEngine::GetInstance().GetParticleManager();
    for (auto i = 0; i < FRICTION_EMITTER_COUNT; ++i)
    {
        auto emitterSo = particleManager.CreateParticleEmitterAtPosition(FRICTION_PARTICLE_DEFINITION_NAME, FRICTION_PARTICLE_EMITTER_POSITIONS[i], mScene, strutils::StringId(FRICTION_EMITTER_NAME_PREFIX + std::to_string(i)));
        particleManager.RemoveParticleEmitterFlag(particle_flags::CONTINUOUS_PARTICLE_GENERATION, emitterSo->mName, mScene);
    }

    ResetBoardSymbols();
}

///------------------------------------------------------------------------------------------------

void BoardView::Update(const float dtMillis)
{
    static float time = 0.0f;
    time += dtMillis / 1000.0f;
    
    // Update time uniform for all symbol scene objects
    for (int row = 0; row < slots::REEL_LENGTH; ++row)
    {
        for (int col = 0; col < slots::BOARD_COLS; ++col)
        {
            auto symbolSoName = GetSymbolSoName(row, col);
            auto symbolFrameSoName = GetSymbolFrameSoName(row, col);
            
            auto symbolSceneObjects = FindAllSceneObjectsForSymbolCoordinates(mScene, row, col);
            
            for (auto& sceneObject: symbolSceneObjects)
            {
                sceneObject->mShaderFloatUniformValues[TIME_UNIFORM_NAME] = time;
            }
        }
    }

    switch (mSpinAnimationState)
    {
        case SpinAnimationState::IDLE: break;
        
        case SpinAnimationState::PRE_SPIN_LOADING:
        {
            // Set up reel speeds and configure reel animations
            mSpinAnimationState = SpinAnimationState::SPINNING;
                        
            for (int i = 0; i < slots::BOARD_COLS; ++i)
            {
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mPendingSymbolData[i].mReelSpeed, MAX_REEL_SPIN_SPEED, TIME_TO_REACH_MAX_REEL_SPIN_SPEED), [](){});
                
                if (mPendingSymbolData[i].mState == PendingSymbolData::PendingSymbolDataState::LOCKED_SUSPENSE)
                {
                    continue;
                }

                float reelSpinTillUnlockDuration = TIME_TILL_REEL_PENDING_SYMBOLS_UNLOCK + TIME_PER_REEL_SYMBOL_UNLOCK * i;
                
                // Scatter suspense flow
                int scatterCountInPreviousReels = 0;
                for (int prevReelIndex = 0; prevReelIndex < i; ++prevReelIndex)
                {
                    scatterCountInPreviousReels += mBoardModel.GetSymbolCountInPlayableReelArea(prevReelIndex, slots::SymbolType::SCATTER);
                }
                
                if (scatterCountInPreviousReels >= 2)
                {
                    reelSpinTillUnlockDuration += SCATTER_SUSPENSE_EXTRA_SPIN_TIME;
                    
                    for (int nextReelIndex = i + 1; nextReelIndex < slots::BOARD_COLS; ++nextReelIndex)
                    {
                        mPendingSymbolData[nextReelIndex].mState = PendingSymbolData::PendingSymbolDataState::LOCKED_SUSPENSE;
                    }
                    
                    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(reelSpinTillUnlockDuration * SCATTER_SLOWDOWN_KICKOFF_MULTIPLIER), [i, this]()
                    {
                        SetFrictionEmitterState(i, true);
                        SetFrictionEmitterState(i + 1, true);
                        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mPendingSymbolData[i].mReelSpeed, MAX_REEL_SPIN_SPEED * SCATTER_SUSPENSE_SLOWDOWN_MULTIPIER, SCATTER_SUSPENSE_EXTRA_SPIN_TIME), [](){});
                    });
                }
                
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(reelSpinTillUnlockDuration), [i, this]()
                {
                    mPendingSymbolData[i].mState = PendingSymbolData::PendingSymbolDataState::UNLOCKED;
                });
            }
        } break;
        
        case SpinAnimationState::SPINNING:
        {
            for (int row = 0; row < slots::REEL_LENGTH; ++row)
            {
                for (int col = 0; col < slots::BOARD_COLS; ++col)
                {
                    if (mPendingSymbolData[col].mState == PendingSymbolData::PendingSymbolDataState::FINISHED)
                    {
                        continue;
                    }

                    auto symbolSceneObjects = FindAllSceneObjectsForSymbolCoordinates(mScene, row, col);
                    
                    for (auto& sceneObject: symbolSceneObjects)
                    {
                        UpdateSceneObjectDuringReelAnimation(sceneObject, dtMillis, col);
                    }
                    
                    // If we've exhausted the final symbols for a reel
                    // animate everything currently in the reel to it's final position
                    if (mPendingSymbolData[col].mSymbols.empty())
                    {
                        AnimateReelSymbolsToFinalPosition(col);
                        mPendingSymbolData[col].mState = PendingSymbolData::PendingSymbolDataState::FINISHED;
                    }
                }
            }
        } break;
            
        case SpinAnimationState::COMBO_PRE_TUMBLING:
        {
            if (mScene.FindSceneObjectsWhoseNameStartsWith(TUMBLE_TEMP_PREFIX).empty())
            {
                for (int row = 0; row < slots::REEL_LENGTH; ++row)
                {
                    for (int col = 0; col < slots::BOARD_COLS; ++col)
                    {
                        auto symbolSceneObjects = FindAllSceneObjectsForSymbolCoordinates(mScene, row, col);
                        
                        for (auto& sceneObject: symbolSceneObjects)
                        {
                            // De-grayscale everything
                            sceneObject->mShaderBoolUniformValues[GRAYSCALE_UNIFORM_NAME] = false;
                            
                            // And reset z-s (manipulated during combo creation)
                            sceneObject->mPosition.z = TOP_LEFT_SYMBOL_POSITION.z;
                            if (IsSceneObjectNameSymbolFrame(sceneObject->mName))
                            {
                                sceneObject->mPosition.z += SYMBOL_FRAME_Z_OFFSET;
                            }
                        }
                    }
                }
                
                // Newly created symbols (at the invisible top of the reel) should equal the number of destroyed symbols
                assert(mTumbleResolutionData.mNewlyCreatedSymbolData.size() == mTumbleResolutionData.mDestroyedCoordsTopToBotom.size());
                
                // Keeps track and updates final tumble positions for all affected symbols
                std::map<slots::SymbolEntryData, NextPositionEntry, SymbolEntryNextPositionComparator> symbolNextPositionsMap;
                
                // Keeps track of symbol destructions per reel
                int numDestroyedSymbolsPerReel[slots::BOARD_COLS] = {0};
                
                auto destroyedCoordCounter = 0;
                auto iter = mTumbleResolutionData.mDestroyedCoordsTopToBotom.begin();
                while (iter != mTumbleResolutionData.mDestroyedCoordsTopToBotom.end())
                {
                    auto symbolEntryDataToDestroy = *iter;
                    const auto col = symbolEntryDataToDestroy.mCol;
                    
                    for (int row = symbolEntryDataToDestroy.mRow - 1; row >= 0; --row)
                    {
                        auto symbolSceneObjects = FindAllSceneObjectsForSymbolCoordinates(mScene, row, col);
                        
                        // Lookup symbol type via Scene Object's texture
                        auto symbolType = slots::SymbolType::COUNT;
                        for (auto sceneObject: symbolSceneObjects)
                        {
                            if (!IsSceneObjectNameSymbolFrame(sceneObject->mName))
                            {
                                symbolType = LookupSceneObjectSymbolType(sceneObject->mTextureResourceId);
                                break;
                            }
                        }
                        
                        if (!symbolSceneObjects.empty())
                        {
                            assert(symbolType != slots::SymbolType::COUNT);
                            slots::SymbolEntryData key = {symbolType, col, row};
                            
                            auto foundIter = symbolNextPositionsMap.find(key);
                            
                            // Update existing next position entry
                            if (foundIter != symbolNextPositionsMap.cend())
                            {
                                symbolNextPositionsMap[key].mSymbolEntryData.mRow++;
                                symbolNextPositionsMap[key].mSymbolPosition.y -= VER_SYMBOL_DISTANCE;
                            }
                            // Create new next position entry
                            else
                            {
                                auto nextSymbolEntryData = key;
                                nextSymbolEntryData.mRow++;
                                
                                auto targetSymbolPosition = TOP_LEFT_SYMBOL_POSITION;
                                targetSymbolPosition.x += col * HOR_SYMBOL_DISTANCE;
                                targetSymbolPosition.y -= (row + 1) * VER_SYMBOL_DISTANCE;
                                
                                symbolNextPositionsMap[key] = NextPositionEntry{nextSymbolEntryData, targetSymbolPosition};
                            }
                        }
                    }
                    
                    // Update counters
                    numDestroyedSymbolsPerReel[col]++;
                    iter = mTumbleResolutionData.mDestroyedCoordsTopToBotom.erase(iter);
                    destroyedCoordCounter++;
                }
                
                // Animate all affected symbols to their target positions
                for (int reelIndex = 0; reelIndex < slots::BOARD_COLS; ++reelIndex)
                {
                    for (const auto& nextPositionEntry: symbolNextPositionsMap)
                    {
                        if (nextPositionEntry.first.mCol != reelIndex)
                        {
                            continue;
                        }
                        
                        auto symbolSceneObjects = FindAllSceneObjectsForSymbolCoordinatesWithSymbolType(mScene, nextPositionEntry.first.mRow, nextPositionEntry.first.mCol, nextPositionEntry.first.mSymbolType);
                        
                        for (auto sceneObject: symbolSceneObjects)
                        {
                            // Adjust z for symbol frames
                            auto finalTargetPosition = nextPositionEntry.second.mSymbolPosition;
                            if (IsSceneObjectNameSymbolFrame(sceneObject->mName))
                            {
                                finalTargetPosition.z += SYMBOL_FRAME_Z_OFFSET;
                            }
                            
                            auto nextRow = nextPositionEntry.second.mSymbolEntryData.mRow;
                            auto nextCol = nextPositionEntry.second.mSymbolEntryData.mCol;
                            
                            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(sceneObject, finalTargetPosition, sceneObject->mScale, TIME_TO_FINALIZE_SYMBOL_POSITION, animation_flags::NONE, reelIndex * TUMBLE_ANIMATION_DELAY_PER_REEL, math::ElasticFunction, math::TweeningMode::EASE_IN), [this, nextRow, nextCol, sceneObject]()
                            {
                                sceneObject->mName = IsSceneObjectNameSymbolFrame(sceneObject->mName) ? GetSymbolFrameSoName(nextRow, nextCol) : GetSymbolSoName(nextRow, nextCol);
                            });
                        }
                    }
                }
                
                // Create new symbols
                for (int reelIndex = 0; reelIndex < slots::BOARD_COLS; ++reelIndex)
                {
                    auto addedInReelCount = 0;
                    for (const auto& newlyCreatedSymbolData: mTumbleResolutionData.mNewlyCreatedSymbolData)
                    {
                        if (newlyCreatedSymbolData.mCol == reelIndex)
                        {
                            CreateSymbolSceneObjects(newlyCreatedSymbolData.mSymbolType, newlyCreatedSymbolData.mRow + numDestroyedSymbolsPerReel[reelIndex] - addedInReelCount - 1, newlyCreatedSymbolData.mCol);
                            addedInReelCount++;
                        }
                    }
                }
                
                mSpinAnimationState = SpinAnimationState::TUMBLING;
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(slots::BOARD_COLS * TUMBLE_ANIMATION_DELAY_PER_REEL + TIME_TO_FINALIZE_SYMBOL_POSITION), [this](){ mSpinAnimationState = SpinAnimationState::POST_SPINNING; });
            }
        } break;
            
        default: break;
    }
}

///------------------------------------------------------------------------------------------------

const std::string& BoardView::GetSpinAnimationStateName() const
{
    return SPIN_ANIMATION_STATE_NAMES.at(mSpinAnimationState);
}

///------------------------------------------------------------------------------------------------

const std::string& BoardView::GetPendingSymbolDataStateName(const int reelIndex) const
{
    return PENDING_SYMBOL_DATA_STATE_NAMES.at(mPendingSymbolData[reelIndex].mState);
}

///------------------------------------------------------------------------------------------------

BoardView::SpinAnimationState BoardView::GetSpinAnimationState() const
{
    return mSpinAnimationState;
}

///------------------------------------------------------------------------------------------------

void BoardView::BeginSpin()
{
    if (mSpinAnimationState == SpinAnimationState::IDLE)
    {
        mSpinAnimationState = SpinAnimationState::PRE_SPIN_LOADING;
        
        for (int col = 0; col < slots::BOARD_COLS; ++col)
        {
            mPendingSymbolData[col].mState = PendingSymbolData::PendingSymbolDataState::LOCKED;
            mPendingSymbolData[col].mReelSpeed = 0.0f;
            mPendingSymbolData[col].mSymbols.clear();
        }
        
        // Pull all symbols up a tiny bit before proceeding with the main reel animation
        for (int row = 0; row < slots::REEL_LENGTH; ++row)
        {
            for (int col = 0; col < slots::BOARD_COLS; ++col)
            {
                mPendingSymbolData[col].mSymbols.push_back(mBoardModel.GetBoardSymbol(row, col));

                auto symbolSoName = GetSymbolSoName(row, col);
                auto symbolFrameSoName = GetSymbolFrameSoName(row, col);
                
                auto symbol = mScene.FindSceneObject(symbolSoName);
                auto symbolFrame = mScene.FindSceneObject(symbolFrameSoName);
                
                symbol->mPosition.y = TOP_LEFT_SYMBOL_POSITION.y - row * VER_SYMBOL_DISTANCE;
                symbolFrame->mPosition.y = TOP_LEFT_SYMBOL_POSITION.y - row * VER_SYMBOL_DISTANCE;
                
                auto targetPosition = symbol->mPosition;
                targetPosition.y += PRE_SPIN_Y_OFFSET;
                
                CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(symbolSoName);
                CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(symbolFrameSoName);
                
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(std::vector<std::shared_ptr<scene::SceneObject>>{ symbol, symbolFrame }, targetPosition, symbol->mScale, PRE_SPIN_ANIMATION_TIME), [](){});
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void BoardView::BeginTumble(const slots::TumbleResolutionData& tumbleResolutionData)
{
    mTumbleResolutionData = tumbleResolutionData;
    mSpinAnimationState = SpinAnimationState::COMBO_PRE_TUMBLING;
    
    //float totalEventAnimationDelay = 0.0f;
    
    // Instantly destroy ingredient symbols (they will be overshadowed by new temporary ones soon)
    int placedComboCounter = 0;
    
    std::set<slots::SymbolEntryData, slots::SymbolEntryDataPlacementComparator> alreadyAnimatingIngredientsSymbolData;
    for (const auto placedComboData: tumbleResolutionData.mPlacedCombosCoords)
    {
        auto comboSymbolPosition = mScene.FindSceneObject(GetSymbolSoName(placedComboData.mRow, placedComboData.mCol))->mPosition;
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(placedComboCounter * TUMBLE_ANIMATION_DELAY_PER_COMBO_EVENT), [placedComboData, this]()
        {
            auto symbol = mScene.FindSceneObject(GetSymbolSoName(placedComboData.mRow, placedComboData.mCol));
            auto symbolFrame = mScene.FindSceneObject(GetSymbolFrameSoName(placedComboData.mRow, placedComboData.mCol));
            
            symbol->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SYMBOL_TEXTURE_PATHS.at(placedComboData.mSymbolType));
            symbol->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + SYMBOL_SHADER_PATH);
            symbol->mPosition.z = TUMBLE_COMBO_SYMBOL_Z;
            symbolFrame->mPosition.z = symbol->mPosition.z + SYMBOL_FRAME_Z_OFFSET;

            auto& particleManager = CoreSystemsEngine::GetInstance().GetParticleManager();
            auto particleEmitterPosition = symbol->mPosition;
            particleEmitterPosition.z = TUMBLE_SMOKE_PARTICLE_Z;
            particleManager.CreateParticleEmitterAtPosition(COMBO_SMOKE_PARTICLE_DEFINITION_NAME, particleEmitterPosition, mScene);
        });
        
        // For just the 4 combo ingredients, other than the place the combo symbol was placed
        int secondIngredientIndex = (placedComboCounter * slots::BOARD_COLS) + 1;
        for (int i = secondIngredientIndex; i < secondIngredientIndex + 4; ++i)
        {
            const auto& ingredientSymbolData = tumbleResolutionData.mComboIngredientsSymbolData[i];
                        
            auto symbolSoName = GetSymbolSoName(ingredientSymbolData.mRow, ingredientSymbolData.mCol);
            auto symbolFrameSoName = GetSymbolFrameSoName(ingredientSymbolData.mRow, ingredientSymbolData.mCol);
            
            auto symbol = mScene.FindSceneObject(symbolSoName);
            auto symbolFrame = mScene.FindSceneObject(symbolFrameSoName);
            
            auto newNamePrefix = TUMBLE_TEMP_PREFIX + std::to_string(placedComboCounter) + "_" + std::to_string(i - secondIngredientIndex);
    
            if (alreadyAnimatingIngredientsSymbolData.contains(ingredientSymbolData) || tumbleResolutionData.mPlacedCombosCoords.contains(ingredientSymbolData))
            {
                auto newSymbolSceneObjects = CreateSymbolSceneObjects(ingredientSymbolData.mSymbolType, ingredientSymbolData.mRow, ingredientSymbolData.mCol, newNamePrefix);
                symbol = newSymbolSceneObjects.first;
                symbolFrame = newSymbolSceneObjects.second;
            }
            else
            {
                symbol->mName = strutils::StringId(newNamePrefix + "_symbol");
                symbolFrame->mName = strutils::StringId(newNamePrefix + "_symbol_frame");
            }
            // Bezier-curve animate ingredients to final position
            symbol->mPosition.z = 1.01f - 0.2f * placedComboCounter + (i - secondIngredientIndex) * 0.05f;
            symbolFrame->mPosition.z = symbol->mPosition.z + SYMBOL_FRAME_Z_OFFSET;
            
            auto animateSceneObjectLambda = [comboSymbolPosition, i, secondIngredientIndex, placedComboCounter, this](std::shared_ptr<scene::SceneObject> sceneObject)
            {
                auto targetPosition = comboSymbolPosition;
                targetPosition.z = sceneObject->mPosition.z;
                
                auto midPosition = (targetPosition + sceneObject->mPosition)/2.0f;
                midPosition.y += (targetPosition.y < sceneObject->mPosition.y) ? TUMBLE_INGREDIENT_BEZIER_MIDPOINT_Y_POSITIONS.r : TUMBLE_INGREDIENT_BEZIER_MIDPOINT_Y_POSITIONS.g;
                math::BezierCurve bezier(std::vector<glm::vec3>{sceneObject->mPosition, midPosition, targetPosition});
                
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::BezierCurveAnimation>(sceneObject, bezier, TUMBLE_INGREDIENT_ANIMATION_TIME, animation_flags::NONE, placedComboCounter * TUMBLE_ANIMATION_DELAY_PER_COMBO_EVENT + (i - secondIngredientIndex) * TUMBLE_INGREDIENT_ANIMATION_DELAY), [sceneObject, this](){ mScene.RemoveSceneObject(sceneObject->mName); });
            };
            
            animateSceneObjectLambda(symbol);
            animateSceneObjectLambda(symbolFrame);
            
            alreadyAnimatingIngredientsSymbolData.insert(ingredientSymbolData);
        }
        
        placedComboCounter++;
    }
}

///------------------------------------------------------------------------------------------------

void BoardView::WaitForPaylines(const slots::BoardStateResolutionData& boardResolutionData)
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto winningPaylines = boardResolutionData.mWinningPaylines;

    animationManager.StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(TIME_DELAY_TO_BEGIN_WINNING_SYMBOLS_ANIMATION), [this, winningPaylines]()
    {
        for (int row = 0; row < slots::REEL_LENGTH; ++row)
        {
            for (int col = 0; col < slots::BOARD_COLS; ++col)
            {
                auto symbolSceneObjects = FindAllSceneObjectsForSymbolCoordinates(mScene, row, col);
                
                for (auto& sceneObject: symbolSceneObjects)
                {
                    sceneObject->mShaderBoolUniformValues[GRAYSCALE_UNIFORM_NAME] = true;
                }
            }
        }
        
        for (const auto& paylineData: winningPaylines)
        {
            for (const auto& symbolData: paylineData.mSymbolData)
            {
                auto symbolSceneObjects = FindAllSceneObjectsForSymbolCoordinates(mScene, symbolData.mRow, symbolData.mCol);
                
                for (auto& sceneObject: symbolSceneObjects)
                {
                    sceneObject->mShaderBoolUniformValues[GRAYSCALE_UNIFORM_NAME] = false;
                }
            }
        }
    });
                                
    mSpinAnimationState = SpinAnimationState::WAITING_FOR_PAYLINES;
}

///------------------------------------------------------------------------------------------------

void BoardView::CompleteSpin()
{
    mSpinAnimationState = SpinAnimationState::IDLE;
}

///------------------------------------------------------------------------------------------------

void BoardView::ResetBoardSymbols()
{
    for (int row = 0; row < slots::REEL_LENGTH; ++row)
    {
        for (int col = 0; col < slots::BOARD_COLS; ++col)
        {
            auto symbolSoName = GetSymbolSoName(row, col);
            auto symbolFrameSoName = GetSymbolFrameSoName(row, col);
            
            CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(symbolSoName);
            CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(symbolFrameSoName);
            
            mScene.RemoveSceneObject(symbolSoName);
            mScene.RemoveSceneObject(symbolFrameSoName);
        }
    }

    for (int row = 0; row < slots::REEL_LENGTH; ++row)
    {
        for (int col = 0; col < slots::BOARD_COLS; ++col)
        {
            CreateSymbolSceneObjects(mBoardModel.GetBoardSymbol(row, col), row, col);
        }
    }
    
    mSpinAnimationState = SpinAnimationState::IDLE;
}

///------------------------------------------------------------------------------------------------

void BoardView::AnimatePaylineReveal(const slots::PaylineResolutionData& paylineResolutionData, const float revealAnimationDurationSecs, const float hidingAnimationDurationSecs, const float delaySecs /* = 0.0f */)
{
    if (!paylineResolutionData.mScatter)
    {
        mPaylines[static_cast<int>(paylineResolutionData.mPayline)].AnimatePaylineReveal(revealAnimationDurationSecs, hidingAnimationDurationSecs, delaySecs);
    }
    
    auto winningSymbolData = paylineResolutionData.mSymbolData;
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    for (const auto& symbolData: winningSymbolData)
    {
        auto symbolName = GetSymbolSoName(symbolData.mRow, symbolData.mCol);
        auto symbolFrameName = GetSymbolFrameSoName(symbolData.mRow, symbolData.mCol);
        
        animationManager.StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(delaySecs + WINNING_SYMBOL_PULSE_ANIMATION_DELAY), [=, this]()
        {
            auto symbol = mScene.FindSceneObject(symbolName);
            auto symbolFrame = mScene.FindSceneObject(symbolFrameName);

            auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
            
            animationManager.StartAnimation(std::make_unique<rendering::PulseAnimation>(symbol, WINNING_SYMBOL_PULSE_SCALE_FACTOR, WINNING_SYMBOL_PULSE_ANIMATION_DURATION, animation_flags::NONE), [](){});
            animationManager.StartAnimation(std::make_unique<rendering::PulseAnimation>(symbolFrame, WINNING_SYMBOL_PULSE_SCALE_FACTOR, WINNING_SYMBOL_PULSE_ANIMATION_DURATION, animation_flags::NONE), [](){});
        });
    }
}

///------------------------------------------------------------------------------------------------

void BoardView::UpdateSceneObjectDuringReelAnimation(std::shared_ptr<scene::SceneObject> sceneObject, const float dtMillis, const int reelIndex)
{
    sceneObject->mPosition.y -= mPendingSymbolData[reelIndex].mReelSpeed * dtMillis;
    auto newRow = static_cast<int>(std::round((TOP_LEFT_SYMBOL_POSITION.y - sceneObject->mPosition.y)/VER_SYMBOL_DISTANCE));
    
    auto newSceneObjectName = sceneObject->mName.GetString();
    newSceneObjectName[0] = '0' + newRow;
    
    // Check whether we need to recycle the symbol
    // and feed it at the top
    if (newRow == slots::REEL_LENGTH)
    {
        newSceneObjectName[0] = '0';
        
        // The position of the "new" symbol should be proportional
        // to the second symbol in a reel
        auto firstRowSymbol = mScene.FindSceneObject(GetSymbolSoName(1, reelIndex));
        if (firstRowSymbol)
        {
            sceneObject->mPosition.y = firstRowSymbol->mPosition.y + VER_SYMBOL_DISTANCE;
        }
        else
        {
            sceneObject->mPosition.y += VER_SYMBOL_DISTANCE * slots::REEL_LENGTH;
        }
        
        if (!IsSceneObjectNameSymbolFrame(strutils::StringId(newSceneObjectName)))
        {
            auto newSymbolType = static_cast<slots::SymbolType>(math::RandomInt() % static_cast<int>(slots::SymbolType::COUNT));
            
            bool existingWildInReel = false;
            bool existingScatterInReel = false;
            for (int row = 0; row < slots::REEL_LENGTH - 1; row++)
            {
                auto sceneObjects = mScene.FindSceneObjectsWhoseNameStartsWith(GetSymbolSoName(row, reelIndex).GetString());
                for (auto sceneObject: sceneObjects)
                {
                    if (sceneObject->mTextureResourceId == CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResourceIdFromPath(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + SYMBOL_TEXTURE_PATHS.at(slots::SymbolType::WILD), false))
                    {
                        existingWildInReel = true;
                    }
                    
                    if (sceneObject->mTextureResourceId == CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResourceIdFromPath(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + SYMBOL_TEXTURE_PATHS.at(slots::SymbolType::SCATTER), false))
                    {
                        existingScatterInReel = true;
                    }
                }
            }
            while (slots::SymbolDataRepository::GetInstance().GetAllRecipesAndIngredientsMap().count(newSymbolType) ||
                   (newSymbolType == slots::SymbolType::WILD && existingWildInReel) ||
                   (newSymbolType == slots::SymbolType::SCATTER && existingScatterInReel))
            {
                newSymbolType = static_cast<slots::SymbolType>(math::RandomInt() % static_cast<int>(slots::SymbolType::COUNT));
            }
            
            // If we have started feeding the final symbols in the reel adjust
            // the new symbol type with the one from the final symbols
            if (mPendingSymbolData[reelIndex].mState == PendingSymbolData::PendingSymbolDataState::UNLOCKED)
            {
                newSymbolType = mPendingSymbolData[reelIndex].mSymbols.back();
                mPendingSymbolData[reelIndex].mSymbols.pop_back();
            }
            
            // Update assets for "new" symbol
            sceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + SYMBOL_TEXTURE_PATHS.at(newSymbolType));
            sceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT  + (SPECIAL_SYMBOL_SHADERS.count(newSymbolType) ? SPECIAL_SYMBOL_SHADERS.at(newSymbolType) : SYMBOL_SHADER_PATH));
        }
    }
    
    sceneObject->mName = strutils::StringId(newSceneObjectName);
}

///------------------------------------------------------------------------------------------------

void BoardView::AnimateReelSymbolsToFinalPosition(const int reelIndex)
{
    std::vector<std::shared_ptr<scene::SceneObject>> reelSceneObjects;
    for (int row = 0; row < slots::REEL_LENGTH; ++row)
    {
        auto symbolSceneObjects = FindAllSceneObjectsForSymbolCoordinates(mScene, row, reelIndex);
        reelSceneObjects.insert(reelSceneObjects.end(), symbolSceneObjects.begin(), symbolSceneObjects.end());
    }
    
    for (int i = 0; i < reelSceneObjects.size(); ++i)
    {
        auto reelSceneObject = reelSceneObjects[i];
        auto finalPosition = reelSceneObject->mPosition;
        finalPosition.y = TOP_LEFT_SYMBOL_POSITION.y - (i/2) * VER_SYMBOL_DISTANCE;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(reelSceneObject, finalPosition, reelSceneObject->mScale, TIME_TO_FINALIZE_SYMBOL_POSITION, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [this, reelIndex](){});
    }
    
    // For scatter suspense, time the unlock of the next reel
    if (reelIndex < slots::BOARD_COLS - 1 && mPendingSymbolData[reelIndex + 1].mState == PendingSymbolData::PendingSymbolDataState::LOCKED_SUSPENSE)
    {
        auto nextReelIndex = reelIndex + 1;
        float reelSpinTillUnlockDuration = TIME_TILL_REEL_PENDING_SYMBOLS_UNLOCK + TIME_PER_REEL_SYMBOL_UNLOCK + SCATTER_SUSPENSE_EXTRA_SPIN_TIME;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(reelSpinTillUnlockDuration * SCATTER_SLOWDOWN_KICKOFF_MULTIPLIER), [nextReelIndex, this]()
        {
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mPendingSymbolData[nextReelIndex].mReelSpeed, MAX_REEL_SPIN_SPEED * SCATTER_SUSPENSE_SLOWDOWN_MULTIPIER, SCATTER_SUSPENSE_EXTRA_SPIN_TIME), [](){});
        });
    
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(reelSpinTillUnlockDuration), [nextReelIndex, this]()
        {
            mPendingSymbolData[nextReelIndex].mState = PendingSymbolData::PendingSymbolDataState::UNLOCKED;
        });
    }
    
    if (reelIndex == slots::BOARD_COLS - 1)
    {
        // Disable any running friction emitters
        SetFrictionEmitterState(reelIndex, false);
        SetFrictionEmitterState(reelIndex + 1, false);
        mSpinAnimationState = SpinAnimationState::POST_SPINNING;
    }
    else
    {
        if (IsFrictionEmitterEnabled(reelIndex))
        {
            // Disable current emitter and enable next reel's emitters
            SetFrictionEmitterState(reelIndex, false);
            SetFrictionEmitterState(reelIndex + 1, true);
            SetFrictionEmitterState(reelIndex + 2, true);
        }
    }
}

///------------------------------------------------------------------------------------------------

bool BoardView::IsFrictionEmitterEnabled(const int emitterIndex) const
{
    auto& particleManager = CoreSystemsEngine::GetInstance().GetParticleManager();
    auto emitterSo = mScene.FindSceneObject(strutils::StringId(FRICTION_EMITTER_NAME_PREFIX + std::to_string(emitterIndex)));
    return particleManager.IsParticleEmitterFlagEnabled(particle_flags::CONTINUOUS_PARTICLE_GENERATION, emitterSo->mName, mScene);
}

///------------------------------------------------------------------------------------------------

void BoardView::SetFrictionEmitterState(const int emitterIndex, const bool enabled)
{
    auto& particleManager = CoreSystemsEngine::GetInstance().GetParticleManager();
    auto emitterSo = mScene.FindSceneObject(strutils::StringId(FRICTION_EMITTER_NAME_PREFIX + std::to_string(emitterIndex)));
    
    if (enabled)
    {
        particleManager.AddParticleEmitterFlag(particle_flags::CONTINUOUS_PARTICLE_GENERATION, emitterSo->mName, mScene);
    }
    else
    {
        particleManager.RemoveParticleEmitterFlag(particle_flags::CONTINUOUS_PARTICLE_GENERATION, emitterSo->mName, mScene);
    }
}

///------------------------------------------------------------------------------------------------

std::pair<std::shared_ptr<scene::SceneObject>, std::shared_ptr<scene::SceneObject>> BoardView::CreateSymbolSceneObjects(const slots::SymbolType symbolType, const int row, const int col, const std::string customNamePrefix /* = "" */)
{
    auto targetSymbolPosition = TOP_LEFT_SYMBOL_POSITION;
    targetSymbolPosition.x += col * HOR_SYMBOL_DISTANCE;
    targetSymbolPosition.y -= row * VER_SYMBOL_DISTANCE;
    
    auto symbol = mScene.CreateSceneObject(customNamePrefix.empty() ? GetSymbolSoName(row, col) : strutils::StringId(customNamePrefix + "_symbol"));
    symbol->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SYMBOL_TEXTURE_PATHS.at(symbolType));
    symbol->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + (SPECIAL_SYMBOL_SHADERS.count(symbolType) ? SPECIAL_SYMBOL_SHADERS.at(symbolType) : SYMBOL_SHADER_PATH));
    symbol->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SCATTER_SYMBOL_EFFECT_TEXTURE_PATH);
    symbol->mEffectTextureResourceIds[1] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SCATTER_BACKGROUND_MASK_TEXTURE_PATH);
    symbol->mShaderFloatUniformValues[INTERACTIVE_COLOR_THRESHOLD_UNIFORM_NAME] = INTERACTIVE_COLOR_THRESHOLD;
    symbol->mShaderFloatUniformValues[INTERACTIVE_COLOR_TIME_MULTIPLIER_UNIFORM_NAME] = INTERACTIVE_COLOR_TIME_MULTIPLIER;
    symbol->mShaderFloatUniformValues[SCATTER_EFFECT_MULTIPLIER_COEFF_UNIFORM_NAME] = SCATTER_EFFECT_MULTIPLIER_COEFF;
    symbol->mShaderBoolUniformValues[GRAYSCALE_UNIFORM_NAME] = false;
    
    symbol->mPosition = targetSymbolPosition;
    symbol->mScale = SYMBOL_SCALE;
    symbol->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;

    auto symbolFrame = mScene.CreateSceneObject(customNamePrefix.empty() ? GetSymbolFrameSoName(row, col) : strutils::StringId(customNamePrefix + "_symbol_frame"));
    symbolFrame->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + SYMBOL_FRAME_TEXTURE_PATH);
    symbolFrame->mPosition = targetSymbolPosition;
    symbolFrame->mPosition.z += SYMBOL_FRAME_Z_OFFSET;
    symbolFrame->mScale = SYMBOL_FRAME_SCALE;
    symbolFrame->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    
    return std::make_pair(symbol, symbolFrame);
}

///------------------------------------------------------------------------------------------------
