///------------------------------------------------------------------------------------------------
///  CardTooltipController.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 16/01/2024                                                       
///------------------------------------------------------------------------------------------------

#include <game/CardTooltipController.h>
#include <game/GameConstants.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId CARD_TOOLTIP_SCENE_OBJECT_NAME = strutils::StringId("card_tooltip");
static const strutils::StringId CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME = strutils::StringId("reveal_threshold");
static const strutils::StringId CARD_TOOLTIP_REVEAL_RGB_EXPONENT_UNIFORM_NAME = strutils::StringId("reveal_rgb_exponent");
static const strutils::StringId CARD_TOOLTIP_TEXT_SCENE_OBJECT_NAMES [game_constants::CARD_TOOLTIP_TEXT_ROWS_COUNT] =
{
    strutils::StringId("card_tooltip_text_0"),
    strutils::StringId("card_tooltip_text_1"),
    strutils::StringId("card_tooltip_text_2"),
    strutils::StringId("card_tooltip_text_3")
};

static const std::string CARD_TOOLTIP_TEXTURE_FILE_NAME = "tooltip.png";
static const std::string CARD_TOOLTIP_VERTICAL_TEXTURE_FILE_NAME = "tooltip_vertical.png";
static const std::string CARD_TOOLTIP_SHADER_FILE_NAME = "diagonal_reveal.vs";

static const glm::vec3 CARD_TOOLTIP_BASE_OFFSET = {0.06f, 0.033f, 0.2f};
static const glm::vec3 CARD_TOOLTIP_TEXT_OFFSETS[game_constants::CARD_TOOLTIP_TEXT_ROWS_COUNT] =
{
    { -0.055f, 0.032f, 0.1f },
    { -0.055f, 0.015f, 0.1f },
    { -0.055f, -0.002f, 0.1f },
    { -0.055f, -0.019f, 0.1f }
};

static const float CARD_TOOLTIP_MAX_REVEAL_THRESHOLD = 2.5f;
static const float CARD_TOOLTIP_REVEAL_SPEED = 1.0f/200.0f;
static const float CARD_TOOLTIP_TEXT_REVEAL_SPEED = 1.0f/500.0f;
static const float CARD_TOOLTIP_FLIPPED_X_OFFSET = -0.17f;
static const float CARD_TOOLTIP_FLIPPED_Y_OFFSET = -0.25f;
static const float CARD_TOOLTIP_TEXT_FLIPPED_X_OFFSET = -0.002f;
static const float CARD_TOOLTIP_TEXT_FLIPPED_Y_OFFSET = -0.008f;

///------------------------------------------------------------------------------------------------

CardTooltipController::CardTooltipController
(
    const glm::vec3& position,
    const glm::vec3& scale,
    const std::string& tooltipText,
    const bool startHidden,
    const bool horFlipped,
    const bool verFlipped,
    scene::Scene& scene
)
{
    auto tooltipSceneObject = scene.CreateSceneObject(CARD_TOOLTIP_SCENE_OBJECT_NAME);
    
    tooltipSceneObject->mPosition = position + CARD_TOOLTIP_BASE_OFFSET;
    tooltipSceneObject->mPosition.x += horFlipped ? CARD_TOOLTIP_FLIPPED_X_OFFSET : 0.046f;
    tooltipSceneObject->mPosition.y += verFlipped ? CARD_TOOLTIP_FLIPPED_Y_OFFSET : 0.0f;
    tooltipSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + (verFlipped ? CARD_TOOLTIP_VERTICAL_TEXTURE_FILE_NAME : CARD_TOOLTIP_TEXTURE_FILE_NAME));
    tooltipSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_TOOLTIP_SHADER_FILE_NAME);
    tooltipSceneObject->mInvisible = startHidden;
    tooltipSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    tooltipSceneObject->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME] = 0.0f;
    tooltipSceneObject->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_RGB_EXPONENT_UNIFORM_NAME] =  1.127f;
    tooltipSceneObject->mScale.x = horFlipped ? -scale.x : scale.x;
    tooltipSceneObject->mScale.y = verFlipped ? -scale.y : scale.y;
    mSceneObjects.push_back(tooltipSceneObject);
    
    auto tooltipTextRows = strutils::StringSplit(tooltipText, '$');
    
    if (tooltipTextRows.size() == 1)
    {
        auto tooltipTextSceneObject = scene.CreateSceneObject(CARD_TOOLTIP_TEXT_SCENE_OBJECT_NAMES[1]);
        tooltipTextSceneObject->mScale = {0.00032f, 0.00032f, 0.00032f};
        tooltipTextSceneObject->mPosition = tooltipSceneObject->mPosition;
        tooltipTextSceneObject->mPosition += 2.0f * CARD_TOOLTIP_TEXT_OFFSETS[1];
        tooltipTextSceneObject->mPosition.x += horFlipped ? (2.0f * CARD_TOOLTIP_TEXT_FLIPPED_X_OFFSET) : 0.0f;
        tooltipTextSceneObject->mPosition.y += verFlipped ? (2.0f * CARD_TOOLTIP_TEXT_FLIPPED_Y_OFFSET) : 0.0f;
        tooltipTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        
        scene::TextSceneObjectData textData;
        textData.mFontName = game_constants::DEFAULT_FONT_BLACK_NAME;
        textData.mText = tooltipTextRows[0];
        tooltipTextSceneObject->mSceneObjectTypeData = std::move(textData);
        
        tooltipTextSceneObject->mInvisible = startHidden;
        mSceneObjects.push_back(tooltipTextSceneObject);
    }
    else
    {
        for (auto i = 0U; i < tooltipTextRows.size(); ++i)
        {
            assert(i < game_constants::CARD_TOOLTIP_TEXT_ROWS_COUNT);
            auto tooltipTextSceneObject = scene.CreateSceneObject(CARD_TOOLTIP_TEXT_SCENE_OBJECT_NAMES[i]);
            tooltipTextSceneObject->mScale = {0.00032f, 0.00032f, 0.00032f};
            tooltipTextSceneObject->mPosition = tooltipSceneObject->mPosition;
            tooltipTextSceneObject->mPosition += 2.0f * CARD_TOOLTIP_TEXT_OFFSETS[i];
            tooltipTextSceneObject->mPosition.x += horFlipped ? (2.0f * CARD_TOOLTIP_TEXT_FLIPPED_X_OFFSET) : 0.0f;
            tooltipTextSceneObject->mPosition.y += verFlipped ? (2.0f * CARD_TOOLTIP_TEXT_FLIPPED_Y_OFFSET) : 0.0f;
            tooltipTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            
            scene::TextSceneObjectData textData;
            textData.mFontName = game_constants::DEFAULT_FONT_BLACK_NAME;
            textData.mText = tooltipTextRows[i];
            tooltipTextSceneObject->mSceneObjectTypeData = std::move(textData);
            
            tooltipTextSceneObject->mInvisible = startHidden;
            mSceneObjects.push_back(tooltipTextSceneObject);
        }
    }
}

///------------------------------------------------------------------------------------------------

CardTooltipController::~CardTooltipController()
{
    
}

///------------------------------------------------------------------------------------------------

void CardTooltipController::Update(const float dtMillis)
{
    mSceneObjects[0]->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME] += dtMillis * CARD_TOOLTIP_REVEAL_SPEED;
    if (mSceneObjects[0]->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME] >= CARD_TOOLTIP_MAX_REVEAL_THRESHOLD)
    {
        mSceneObjects[0]->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME] = CARD_TOOLTIP_MAX_REVEAL_THRESHOLD;
        
        for (auto i = 1U; i < mSceneObjects.size(); ++i)
        {
            auto tooltipTextSceneObject = mSceneObjects[i];
            tooltipTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = math::Min(1.0f, tooltipTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] +  dtMillis * CARD_TOOLTIP_TEXT_REVEAL_SPEED);
        }
    }
}

///------------------------------------------------------------------------------------------------

std::vector<std::shared_ptr<scene::SceneObject>>& CardTooltipController::GetSceneObjects()
{
    return mSceneObjects;
}

///------------------------------------------------------------------------------------------------
