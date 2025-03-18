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
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObjectUtils.h>
#include <net_common/Board.h>
#include <net_common/Symbols.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId BOARD_NAME = strutils::StringId("board");

static const glm::vec3 BOARD_SCALE = glm::vec3(0.5f * 1.28f, 0.5f, 1.0f);
static const glm::vec3 SYMBOL_SCALE = glm::vec3(0.08f * 1.4f, 0.08f, 1.0f);
static const float HOR_SYMBOL_DISTANCE = 0.123f;
static const float VER_SYMBOL_DISTANCE = 0.116f;
static const glm::vec3 TOP_LEFT_SYMBOL_POSITION = glm::vec3(-0.2467f, 0.464f, 0.1f);

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
    { slots::SymbolType::ROAST_CHICKEN, "game/food_slot_images/Roast_chicken.png" },
    { slots::SymbolType::WILD, "game/food_slot_images/grandma.png" }
};

///------------------------------------------------------------------------------------------------

BoardView::BoardView(scene::Scene& scene, const slots::Board& boardModel)
    : mScene(scene)
    , mBoardModel(boardModel)
{
    auto board = scene.CreateSceneObject(BOARD_NAME);
    board->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/shelves.png");
    board->mPosition.z = -0.2f;
    board->mScale = BOARD_SCALE;
    board->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 0.0f;
    mSceneObjects.push_back(board);
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(board, 1.0f, 0.5f), [](){});
    
    DebugFillBoard();
}

///------------------------------------------------------------------------------------------------

void BoardView::Update(const float)
{
}

///------------------------------------------------------------------------------------------------

std::vector<std::shared_ptr<scene::SceneObject>> BoardView::GetSceneObjects() { return mSceneObjects; }

///------------------------------------------------------------------------------------------------

const std::string& BoardView::GetSymbolTexturePath(slots::SymbolType symbol) const
{
    return SYMBOL_TEXTURE_PATHS.at(symbol);
}

///------------------------------------------------------------------------------------------------

void BoardView::DebugFillBoard()
{
    for (int row = 0; row < slots::REEL_LENGTH; ++row)
    {
        for (int col = 0; col < slots::BOARD_COLS; ++col)
        {
            auto symbolSoName = strutils::StringId(std::to_string(row) + "," + std::to_string(col) + "_symbol");
            auto symbolFrameSoName = strutils::StringId(std::to_string(row) + "," + std::to_string(col) + "_symbol_frame");
            
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
            
            auto initSymbolPosition = targetSymbolPosition;
            initSymbolPosition.y += 0.2f;

            auto symbol = mScene.CreateSceneObject(strutils::StringId(std::to_string(row) + "," + std::to_string(col) + "_symbol"));
            symbol->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + SYMBOL_TEXTURE_PATHS.at(mBoardModel.GetBoardSymbol(row, col)));
            symbol->mPosition = initSymbolPosition;
            symbol->mScale = SYMBOL_SCALE * 0.9f;
            symbol->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 1.0f;
            mSceneObjects.push_back(symbol);
            
            auto symbolFrame = mScene.CreateSceneObject(strutils::StringId(std::to_string(row) + "," + std::to_string(col) + "_symbol_frame"));
            symbolFrame->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/basket_frame.png");
            symbolFrame->mPosition = initSymbolPosition;
            symbolFrame->mPosition.z += 0.1f;
            symbolFrame->mScale = SYMBOL_SCALE;
            symbolFrame->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 1.0f;
            mSceneObjects.push_back(symbolFrame);
            
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(std::vector<std::shared_ptr<scene::SceneObject>>{ symbol, symbolFrame }, targetSymbolPosition, symbol->mScale, 1.0f, animation_flags::NONE, col * 0.3f, math::ElasticFunction), [](){});
        }
    }
}

///------------------------------------------------------------------------------------------------
