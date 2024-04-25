///------------------------------------------------------------------------------------------------
///  Fonts.h
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 22/09/2023
///------------------------------------------------------------------------------------------------

#ifndef Fonts_h
#define Fonts_h

///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/MathUtils.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/StringUtils.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

struct Glyph
{
    float minU = 0.0f;
    float minV = 0.0f;
    float maxU = 0.0f;
    float maxV = 0.0f;
    float mXOffsetPixels = 0.0f;
    float mYOffsetPixels = 0.0f;
    float mWidthPixels = 0.0f;
    float mHeightPixels = 0.0f;
    float mAdvancePixels = 0.0f;
    float mXOffsetOverride = 0.0f;
};

///------------------------------------------------------------------------------------------------

struct Font
{
    const Glyph& FindGlyph(char c) const
    {
        auto glyphIter = mGlyphs.find(c);
        return glyphIter == mGlyphs.cend() ? mGlyphs.at(' ') : glyphIter->second;
    }
    
    strutils::StringId mFontName = strutils::StringId();
    resources::ResourceId mFontTextureResourceId;
    std::unordered_map<char, Glyph> mGlyphs;
    glm::vec2 mFontTextureDimensions = glm::vec2(0.0f, 0.0f);
};

///------------------------------------------------------------------------------------------------

class FontRepository final
{
    friend struct CoreSystemsEngine::SystemsImpl;
public:
    ~FontRepository() = default;
    FontRepository(const FontRepository&) = delete;
    FontRepository(FontRepository&&) = delete;
    const FontRepository& operator = (const FontRepository&) = delete;
    FontRepository& operator = (FontRepository&&) = delete;
    
    std::optional<std::reference_wrapper<const Font>> GetFont(const strutils::StringId& fontName) const;
    void ReloadMarkedFontsFromDisk();
    void LoadFont(const std::string& fontName, const resources::ResourceReloadMode resourceReloadMode = resources::ResourceReloadMode::DONT_RELOAD);
    
private:
    FontRepository() = default;
    
private:
    std::unordered_map<strutils::StringId, Font, strutils::StringIdHasher> mFontMap;
    std::unordered_set<strutils::StringId, strutils::StringIdHasher> mFontsToAutoReload;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* Fonts_h */
