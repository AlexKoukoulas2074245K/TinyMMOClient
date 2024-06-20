///------------------------------------------------------------------------------------------------
///  Fonts.cpp
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 22/09/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/Fonts.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/resloading/TextureResource.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <engine/utils/Logging.h>
#include <engine/utils/OSMessageBox.h>
#include <nlohmann/json.hpp>

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

static const std::string FONT_PLACEHOLDER_STRING = "_placeholder";

///------------------------------------------------------------------------------------------------

std::optional<std::reference_wrapper<const Font>> FontRepository::GetFont(const strutils::StringId& fontName) const
{
    auto findIter = mFontMap.find(fontName);
    if (findIter != mFontMap.end())
    {
        return std::optional<std::reference_wrapper<const Font>>{findIter->second};
    }
    
    ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "Cannot find font", fontName.GetString().c_str());
    return std::nullopt;
}

///------------------------------------------------------------------------------------------------

void FontRepository::ReloadMarkedFontsFromDisk()
{
    for (const auto& fontName: mFontsToAutoReload)
    {
        LoadFont(fontName.GetString());
    }
}

///------------------------------------------------------------------------------------------------

void FontRepository::LoadFont(const std::string& fontName, const resources::ResourceReloadMode resourceReloadMode /* = resources::ResourceReloadMode::DONT_RELOAD */)
{
    auto fontTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + fontName + ".png", resourceReloadMode);
    auto& fontTexture = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::TextureResource>(fontTextureResourceId);
    
    auto fontDefinitionName = fontName;
    if (strutils::StringContains(fontDefinitionName, FONT_PLACEHOLDER_STRING))
    {
        fontDefinitionName = fontDefinitionName.substr(0, fontDefinitionName.find(FONT_PLACEHOLDER_STRING));
    }
    
    auto fontDefinitionJsonResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_DATA_ROOT + fontDefinitionName + ".fnt", resourceReloadMode);
    const auto& fontData = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::DataFileResource>(fontDefinitionJsonResourceId).GetContents();
    
    Font font;
    font.mFontName = strutils::StringId(fontName);
    font.mFontTextureResourceId = fontTextureResourceId;
    font.mFontTextureDimensions = fontTexture.GetDimensions();
    
    std::stringstream fontLineStream(fontData);
    std::string fontLine;
    while (std::getline(fontLineStream, fontLine))
    {
        if (strutils::StringStartsWith(fontLine, "char id="))
        {
            const auto& lineComponents = strutils::StringSplit(fontLine, ' ');
            
            Glyph glyph;
            glyph.mWidthPixels = std::stof(strutils::StringSplit(lineComponents[4], '=')[1]);
            glyph.mHeightPixels = std::stof(strutils::StringSplit(lineComponents[5], '=')[1]);
            
            auto normalizedU = std::stof(strutils::StringSplit(lineComponents[2], '=')[1]) / font.mFontTextureDimensions.x;
            glyph.minU = normalizedU;
            glyph.maxU = normalizedU + glyph.mWidthPixels / font.mFontTextureDimensions.x;
            
            auto normalizedV = (font.mFontTextureDimensions.y - std::stof(strutils::StringSplit(lineComponents[3], '=')[1])) / font.mFontTextureDimensions.y;
            glyph.minV = normalizedV - glyph.mHeightPixels / font.mFontTextureDimensions.y;
            glyph.maxV = normalizedV;
            
            glyph.mXOffsetPixels = std::stof(strutils::StringSplit(lineComponents[6], '=')[1]);
            glyph.mYOffsetPixels = std::stof(strutils::StringSplit(lineComponents[7], '=')[1]);
            glyph.mAdvancePixels = std::stof(strutils::StringSplit(lineComponents[8], '=')[1]);
            
            font.mGlyphs[std::stof(strutils::StringSplit(lineComponents[1], '=')[1])] = glyph;
        }
    }
    
    mFontMap[font.mFontName] = font;
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
