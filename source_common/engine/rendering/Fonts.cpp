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
    
    auto fontDefinitionJsonResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_DATA_ROOT + fontDefinitionName + ".json", resourceReloadMode);
    const auto fontJson =  nlohmann::json::parse(CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::DataFileResource>(fontDefinitionJsonResourceId).GetContents());
    
    Font font;
    font.mFontName = strutils::StringId(fontName);
    font.mFontTextureResourceId = fontTextureResourceId;
    font.mFontTextureDimensions = fontTexture.GetDimensions();
    
    for (const auto& charObject: fontJson["font"]["chars"]["char"])
    {
        Glyph glyph;
        glyph.mWidthPixels = std::stof(charObject["width"].get<std::string>());
        glyph.mHeightPixels = std::stof(charObject["height"].get<std::string>());
        
        auto normalizedU = std::stof(charObject["x"].get<std::string>()) / font.mFontTextureDimensions.x;
        glyph.minU = normalizedU;
        glyph.maxU = normalizedU + glyph.mWidthPixels / font.mFontTextureDimensions.x;
        
        auto normalizedV = (font.mFontTextureDimensions.y - std::stof(charObject["y"].get<std::string>())) / font.mFontTextureDimensions.y;
        glyph.minV = normalizedV - glyph.mHeightPixels / font.mFontTextureDimensions.y;
        glyph.maxV = normalizedV;
        
        glyph.mXOffsetPixels = std::stof(charObject["xoffset"].get<std::string>());
        glyph.mYOffsetPixels = std::stof(charObject["yoffset"].get<std::string>());
        glyph.mAdvancePixels = std::stof(charObject["xadvance"].get<std::string>());
        
        if (charObject.count("xoffsetoverride"))
        {
            glyph.mXOffsetOverride = std::stof(charObject["xoffsetoverride"].get<std::string>());
        }
        
        font.mGlyphs[static_cast<char>(std::stoi(charObject["id"].get<std::string>()))] = glyph;
    }
    
    mFontMap[font.mFontName] = font;
    
    if (resourceReloadMode == resources::ResourceReloadMode::RELOAD_EVERY_SECOND)
    {
        mFontsToAutoReload.insert(font.mFontName);
    }
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
