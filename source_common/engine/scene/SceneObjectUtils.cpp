///------------------------------------------------------------------------------------------------
///  SceneObjectUtils.cpp                                                                                        
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 11/10/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/Fonts.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/scene/SceneObject.h>

///------------------------------------------------------------------------------------------------

namespace scene_object_utils
{

///------------------------------------------------------------------------------------------------

math::Rectangle GetSceneObjectBoundingRect(const scene::SceneObject& sceneObject)
{
    math::Rectangle boundingRect;
    boundingRect.bottomLeft = glm::vec2(0.0f);
    boundingRect.topRight = glm::vec2(0.0f);
    
    if (std::holds_alternative<scene::TextSceneObjectData>(sceneObject.mSceneObjectTypeData))
    {
        const auto& textData = std::get<scene::TextSceneObjectData>(sceneObject.mSceneObjectTypeData);
        auto fontOpt = CoreSystemsEngine::GetInstance().GetFontRepository().GetFont(textData.mFontName);
        if (!fontOpt) return boundingRect;
        
        const auto& font = fontOpt->get();
        
        float xCursor = sceneObject.mPosition.x;
        float yCursor = sceneObject.mPosition.y;
        
        float minX = xCursor;
        float minY = yCursor;
        float maxX = xCursor;
        float maxY = yCursor;
        
        const auto& stringFontGlyphs = font.FindGlyphs(textData.mText);
        for (size_t i = 0; i < stringFontGlyphs.size(); ++i)
        {
            const auto& glyph = stringFontGlyphs[i];
            
            yCursor = sceneObject.mPosition.y - glyph.mHeightPixels/2.0f * sceneObject.mScale.y;
            
            float targetX = xCursor + glyph.mXOffsetPixels * sceneObject.mScale.x;
            float targetY = yCursor - glyph.mYOffsetPixels * sceneObject.mScale.y;
            
            if (targetX + glyph.mWidthPixels * sceneObject.mScale.x/2 > maxX) maxX = targetX + glyph.mWidthPixels * sceneObject.mScale.x/2;
            if (targetX - glyph.mWidthPixels * sceneObject.mScale.x/2 < minX) minX = targetX - glyph.mWidthPixels * sceneObject.mScale.x/2;
            if (targetY + glyph.mHeightPixels * sceneObject.mScale.y/2 > maxY) maxY = targetY + glyph.mHeightPixels * sceneObject.mScale.y/2;
            if (targetY - glyph.mHeightPixels * sceneObject.mScale.y/2 < minY) minY = targetY - glyph.mHeightPixels * sceneObject.mScale.y/2;
            
            if (i != stringFontGlyphs.size() - 1)
            {
                xCursor += (glyph.mAdvancePixels * sceneObject.mScale.x)/2.0f + (stringFontGlyphs[i + 1].mAdvancePixels * sceneObject.mScale.y)/2.0f;
            }
        }
        
        boundingRect.bottomLeft = glm::vec2(minX, minY);
        boundingRect.topRight = glm::vec2(maxX, maxY);
    }
    else if (std::holds_alternative<scene::DefaultSceneObjectData>(sceneObject.mSceneObjectTypeData))
    {                                                                                                
        boundingRect.bottomLeft = glm::vec2(sceneObject.mPosition.x - math::Abs((sceneObject.mScale.x * sceneObject.mBoundingRectMultiplier.x)/2), sceneObject.mPosition.y - math::Abs((sceneObject.mScale.y * sceneObject.mBoundingRectMultiplier.y)/2));
        boundingRect.topRight = glm::vec2(sceneObject.mPosition.x + math::Abs((sceneObject.mScale.x * sceneObject.mBoundingRectMultiplier.x)/2), sceneObject.mPosition.y + math::Abs((sceneObject.mScale.y * sceneObject.mBoundingRectMultiplier.y)/2));
    }
    
    return boundingRect;
}

///------------------------------------------------------------------------------------------------

}
