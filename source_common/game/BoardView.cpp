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
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObjectUtils.h>
#include <net_common/Board.h>
#include <net_common/Symbols.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId BOARD_NAME = strutils::StringId("board");
static const strutils::StringId INTERACTIVE_COLOR_THRESHOLD_UNIFORM_NAME = strutils::StringId("interactive_color_threshold");
static const strutils::StringId INTERACTIVE_COLOR_TIME_MULTIPLIER_UNIFORM_NAME = strutils::StringId("interactive_color_time_multiplier");

static const std::string SYMBOL_FRAME_TEXTURE_PATH = "game/basket_frame.png";

static const glm::vec3 BOARD_SCALE = glm::vec3(0.5f * 1.28f, 0.5f, 1.0f);
static const glm::vec3 SYMBOL_SCALE = glm::vec3(0.08f * 1.4f, 0.08f, 1.0f);
static const glm::vec3 TOP_LEFT_SYMBOL_POSITION = glm::vec3(-0.2467f, 0.464f, 0.1f);

static const float HOR_SYMBOL_DISTANCE = 0.123f;
static const float VER_SYMBOL_DISTANCE = 0.116f;
static const float PRE_SPIN_Y_OFFSET = 0.04f;
static const float PRE_SPIN_ANIMATION_TIME = 0.15f;
static const float MAX_REEL_SPIN_SPEED = 0.001f;
static const float TIME_TO_REACH_MAX_REEL_SPIN_SPEED = 0.5f;
static const float TIME_TILL_REEL_PENDING_SYMBOLS_UNLOCK = 1.0f;
static const float TIME_PER_REEL_SYMBOL_UNLOCK = 0.3f;
static const float TIME_TO_FINALIZE_SYMBOL_POSITION = 0.6f;
static const float INTERACTIVE_COLOR_THRESHOLD = 0.07f;
static const float INTERACTIVE_COLOR_TIME_MULTIPLIER = -0.7f;

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
    { slots::SymbolType::CHOCOLATE_CAKE, "game/food_slot_images/chocolate_cake.png" },
    { slots::SymbolType::STRAWBERRY_CAKE, "game/food_slot_images/strawberry_cake.png" },
    { slots::SymbolType::ROAST_CHICKEN, "game/food_slot_images/roast_chicken.png" },
    { slots::SymbolType::WILD, "game/food_slot_images/wild.png" },
    { slots::SymbolType::SCATTER, "game/food_slot_images/grandma.png" }
};

static const std::unordered_map<slots::SymbolType, std::string> SPECIAL_SYMBOL_SHADERS =
{
    { slots::SymbolType::WILD, "wild_symbol.vs" }
};

static const std::unordered_map<BoardView::SpinAnimationState, std::string> SPIN_ANIMATION_STATE_NAMES =
{
    { BoardView::SpinAnimationState::IDLE, "IDLE" },
    { BoardView::SpinAnimationState::PRE_SPIN_LOADING, "PRE_SPIN_LOADING" },
    { BoardView::SpinAnimationState::SPINNING, "SPINNING" },
    { BoardView::SpinAnimationState::POST_SPINNING, "POST_SPINNING" },
    { BoardView::SpinAnimationState::WAITING_FOR_PAYLINES, "WAITING_FOR_PAYLINES" }
};

static const std::unordered_map<BoardView::PendingSymbolData::PendingSymbolDataState, std::string> PENDING_SYMBOL_DATA_STATE_NAMES =
{
    { BoardView::PendingSymbolData::PendingSymbolDataState::LOCKED, "LOCKED" },
    { BoardView::PendingSymbolData::PendingSymbolDataState::UNLOCKED, "UNLOCKED" },
    { BoardView::PendingSymbolData::PendingSymbolDataState::FINISHED, "FINISHED" }
};

///------------------------------------------------------------------------------------------------

static inline strutils::StringId GetSymbolSoName(const int row, const int col) { return strutils::StringId(std::to_string(row) + "," + std::to_string(col) + "_symbol"); }
static inline strutils::StringId GetSymbolFrameSoName(const int row, const int col) { return strutils::StringId(std::to_string(row) + "," + std::to_string(col) + "_symbol_frame"); }

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
    , mSymbolSpinSpeed(0.0f)
{
    auto board = scene.CreateSceneObject(BOARD_NAME);
    board->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/shelves.png");
    board->mPosition.z = -0.2f;
    board->mScale = BOARD_SCALE;
    board->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    mSceneObjects.push_back(board);
    
    for (auto i = 0; i < static_cast<int>(slots::PaylineType::PAYLINE_COUNT); ++i)
    {
        mPaylines.push_back(PaylineView(scene, static_cast<slots::PaylineType>(i)));
    }
    
    ResetBoardSymbols();
}

///------------------------------------------------------------------------------------------------

void BoardView::Update(const float dtMillis)
{
    static float time = 0.0f;
    time += dtMillis / 1000.0f;

    for (int row = 0; row < slots::REEL_LENGTH; ++row)
    {
        for (int col = 0; col < slots::BOARD_COLS; ++col)
        {
            auto symbolSoName = GetSymbolSoName(row, col);
            auto symbolFrameSoName = GetSymbolFrameSoName(row, col);
            
            auto symbolSceneObjects = mScene.FindSceneObjectsWhoseNameStartsWith(std::to_string(row) + "," + std::to_string(col));
            
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
            bool foundAnimatedSymbols = false;
            for (int row = 0; row < slots::REEL_LENGTH; ++row)
            {
                for (int col = 0; col < slots::BOARD_COLS; ++col)
                {
                    auto symbolSoName = GetSymbolSoName(row, col);
                    auto symbolFrameSoName = GetSymbolFrameSoName(row, col);
                    
                    foundAnimatedSymbols |= CoreSystemsEngine::GetInstance().GetAnimationManager().GetAnimationCountPlayingForSceneObject(symbolSoName);
                    foundAnimatedSymbols |= CoreSystemsEngine::GetInstance().GetAnimationManager().GetAnimationCountPlayingForSceneObject(symbolFrameSoName);
                }
            }
            
            if (!foundAnimatedSymbols)
            {
                mSpinAnimationState = SpinAnimationState::SPINNING;
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mSymbolSpinSpeed, MAX_REEL_SPIN_SPEED, TIME_TO_REACH_MAX_REEL_SPIN_SPEED), [](){});
                
                for (int i = 0; i < slots::BOARD_COLS; ++i)
                {
                    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(TIME_TILL_REEL_PENDING_SYMBOLS_UNLOCK + TIME_PER_REEL_SYMBOL_UNLOCK * i), [i, this](){ mPendingSymbolData[i].mState = PendingSymbolData::PendingSymbolDataState::UNLOCKED; });
                }
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

                    auto symbolSoName = GetSymbolSoName(row, col);
                    auto symbolFrameSoName = GetSymbolFrameSoName(row, col);
                    
                    auto symbolSceneObjects = mScene.FindSceneObjectsWhoseNameStartsWith(std::to_string(row) + "," + std::to_string(col));
                    
                    for (auto& sceneObject: symbolSceneObjects)
                    {
                        sceneObject->mPosition.y -= mSymbolSpinSpeed * dtMillis;
                        auto newRow = static_cast<int>(std::round((TOP_LEFT_SYMBOL_POSITION.y - sceneObject->mPosition.y)/VER_SYMBOL_DISTANCE));
                        
                        auto newSceneObjectName = sceneObject->mName.GetString();
                        newSceneObjectName[0] = '0' + newRow;
                        
                        if (newRow == slots::REEL_LENGTH)
                        {
                            newSceneObjectName[0] = '0';
                            
                            auto firstRowSymbol = mScene.FindSceneObject(GetSymbolSoName(1,col));
                            if (firstRowSymbol)
                            {
                                sceneObject->mPosition.y = firstRowSymbol->mPosition.y + VER_SYMBOL_DISTANCE;
                            }
                            else
                            {
                                sceneObject->mPosition.y += VER_SYMBOL_DISTANCE * slots::REEL_LENGTH;
                            }
                            
                            if (!strutils::StringEndsWith(newSceneObjectName, "frame"))
                            {
                                auto newSymbolType = static_cast<slots::SymbolType>(math::RandomInt() % static_cast<int>(slots::SymbolType::COUNT));
                                while (newSymbolType == slots::SymbolType::CHOCOLATE_CAKE ||
                                       newSymbolType == slots::SymbolType::STRAWBERRY_CAKE ||
                                       newSymbolType == slots::SymbolType::ROAST_CHICKEN)
                                {
                                    newSymbolType = static_cast<slots::SymbolType>(math::RandomInt() % static_cast<int>(slots::SymbolType::COUNT));
                                }
                                
                                if (mPendingSymbolData[col].mState == PendingSymbolData::PendingSymbolDataState::UNLOCKED)
                                {
                                    newSymbolType = mPendingSymbolData[col].mSymbols.back();
                                    mPendingSymbolData[col].mSymbols.pop_back();
                                }

                                sceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + SYMBOL_TEXTURE_PATHS.at(newSymbolType));
                                sceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT  + (SPECIAL_SYMBOL_SHADERS.count(newSymbolType) ? SPECIAL_SYMBOL_SHADERS.at(newSymbolType) : "symbol.vs"));
                            }
                        }
                        
                        sceneObject->mName = strutils::StringId(newSceneObjectName);
                    }
                    
                    if (mPendingSymbolData[col].mSymbols.empty())
                    {
                        AnimateReelSymbolsToFinalPosition(col);
                        mPendingSymbolData[col].mState = PendingSymbolData::PendingSymbolDataState::FINISHED;
                    }
                }
            }
        } break;
            
        default: break;
    }
}

///------------------------------------------------------------------------------------------------

std::vector<std::shared_ptr<scene::SceneObject>> BoardView::GetSceneObjects() { return mSceneObjects; }

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
            mPendingSymbolData[col].mSymbols.clear();
        }
        
        for (int row = 0; row < slots::REEL_LENGTH; ++row)
        {
            for (int col = 0; col < slots::BOARD_COLS; ++col)
            {
                auto symbolSceneObjects = mScene.FindSceneObjectsWhoseNameStartsWith(std::to_string(row) + "," + std::to_string(col));
                assert(symbolSceneObjects.size() == 2);
                
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

void BoardView::WaitForPaylines()
{
    for (int row = 0; row < slots::REEL_LENGTH; ++row)
    {
        for (int col = 0; col < slots::BOARD_COLS; ++col)
        {
            auto symbolSoName = GetSymbolSoName(row, col);
            auto symbolFrameSoName = GetSymbolFrameSoName(row, col);
            
            auto symbolSceneObjects = mScene.FindSceneObjectsWhoseNameStartsWith(std::to_string(row) + "," + std::to_string(col));
            
            for (auto& sceneObject: symbolSceneObjects)
            {
                sceneObject->mShaderBoolUniformValues[GRAYSCALE_UNIFORM_NAME] = true;
            }
        }
    }
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
    
    if (mSceneObjects.size() > 1)
    {
        mSceneObjects.erase(mSceneObjects.begin() + 1, mSceneObjects.end());
    }
    
    for (int row = 0; row < slots::REEL_LENGTH; ++row)
    {
        for (int col = 0; col < slots::BOARD_COLS; ++col)
        {
            auto targetSymbolPosition = TOP_LEFT_SYMBOL_POSITION;
            targetSymbolPosition.x += col * HOR_SYMBOL_DISTANCE;
            targetSymbolPosition.y -= row * VER_SYMBOL_DISTANCE;
            
            auto symbol = mScene.CreateSceneObject(GetSymbolSoName(row, col));
            symbol->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + SYMBOL_TEXTURE_PATHS.at(mBoardModel.GetBoardSymbol(row, col)));
            symbol->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT  + (SPECIAL_SYMBOL_SHADERS.count(mBoardModel.GetBoardSymbol(row, col)) ? SPECIAL_SYMBOL_SHADERS.at(mBoardModel.GetBoardSymbol(row, col)) : "basic.vs"));
            symbol->mShaderFloatUniformValues[INTERACTIVE_COLOR_THRESHOLD_UNIFORM_NAME] = INTERACTIVE_COLOR_THRESHOLD;
            symbol->mShaderFloatUniformValues[INTERACTIVE_COLOR_TIME_MULTIPLIER_UNIFORM_NAME] = INTERACTIVE_COLOR_TIME_MULTIPLIER;
            symbol->mShaderBoolUniformValues[GRAYSCALE_UNIFORM_NAME] = false;

            symbol->mPosition = targetSymbolPosition;
            symbol->mScale = SYMBOL_SCALE * 0.9f;
            symbol->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
            mSceneObjects.push_back(symbol);

            auto symbolFrame = mScene.CreateSceneObject(GetSymbolFrameSoName(row, col));
            symbolFrame->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + SYMBOL_FRAME_TEXTURE_PATH);
            symbolFrame->mPosition = targetSymbolPosition;
            symbolFrame->mPosition.z += 0.1f;
            symbolFrame->mScale = SYMBOL_SCALE;
            symbolFrame->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
            mSceneObjects.push_back(symbolFrame);
        }
    }
    
    mSpinAnimationState = SpinAnimationState::IDLE;
    mSymbolSpinSpeed = 0.0f;
}

///------------------------------------------------------------------------------------------------

void BoardView::AnimatePaylineReveal(const slots::PaylineType paylineType, const float revealAnimationDurationSecs, const float hidingAnimationDurationSecs, const float delaySecs /* = 0.0f */)
{
    mPaylines[static_cast<int>(paylineType)].AnimatePaylineReveal(revealAnimationDurationSecs, hidingAnimationDurationSecs, delaySecs);
}

///------------------------------------------------------------------------------------------------

void BoardView::AnimateReelSymbolsToFinalPosition(const int reelIndex)
{
    std::vector<std::shared_ptr<scene::SceneObject>> reelSceneObjects;
    for (int row = 0; row < slots::REEL_LENGTH; ++row)
    {
        auto symbolSceneObjects = mScene.FindSceneObjectsWhoseNameStartsWith(std::to_string(row) + "," + std::to_string(reelIndex));
        reelSceneObjects.insert(reelSceneObjects.end(), symbolSceneObjects.begin(), symbolSceneObjects.end());
    }
    
    for (int i = 0; i < reelSceneObjects.size(); ++i)
    {
        auto reelSceneObject = reelSceneObjects[i];
        auto finalPosition = reelSceneObject->mPosition;
        finalPosition.y = TOP_LEFT_SYMBOL_POSITION.y - (i/2) * VER_SYMBOL_DISTANCE;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(reelSceneObject, finalPosition, reelSceneObject->mScale, TIME_TO_FINALIZE_SYMBOL_POSITION, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [this, reelIndex](){});
    }
    
    if (reelIndex == slots::BOARD_COLS - 1)
    {
        mSpinAnimationState = SpinAnimationState::POST_SPINNING;
    }
}

///------------------------------------------------------------------------------------------------
