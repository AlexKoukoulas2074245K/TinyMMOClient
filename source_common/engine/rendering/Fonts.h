///------------------------------------------------------------------------------------------------
///  Fonts.h
///  TinyMMOClient
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
    const std::vector<Glyph> FindGlyphs(const std::string& str) const
    {
        std::vector<Glyph> glyphs;
        size_t i = 0;
        while (i < str.size())
        {
            uint32_t codePoint = 0;
            unsigned char c = str[i];
            if (c < 0x80) 
            {
                // 1-byte character (ASCII)
                codePoint = c;
                i += 1;
            } 
            else if ((c & 0xE0) == 0xC0)
            {
                // 2-byte character
                codePoint = ((c & 0x1F) << 6) |
                            (str[i + 1] & 0x3F);
                i += 2;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                // 3-byte character
                codePoint = ((c & 0x0F) << 12) |
                            ((str[i + 1] & 0x3F) << 6) |
                            (str[i + 2] & 0x3F);
                i += 3;
            }
            else if ((c & 0xF8) == 0xF0) 
            {
                // 4-byte character
                codePoint = ((c & 0x07) << 18) |
                            ((str[i + 1] & 0x3F) << 12) |
                            ((str[i + 2] & 0x3F) << 6) |
                            (str[i + 3] & 0x3F);
                i += 4;
            }
            
            auto glyphIter = mGlyphs.find(codePoint);
            glyphs.push_back(glyphIter == mGlyphs.end() ? mGlyphs.at('?') : glyphIter->second);
        }
        
        return glyphs;
    }
    
    strutils::StringId mFontName = strutils::StringId();
    resources::ResourceId mFontTextureResourceId;
    std::unordered_map<uint32_t, Glyph> mGlyphs;
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
