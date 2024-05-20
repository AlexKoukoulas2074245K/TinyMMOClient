///------------------------------------------------------------------------------------------------
///  RenderingUtils.cpp                                                                                        
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 31/10/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/RenderingUtils.h>
#include <engine/rendering/OpenGL.h>
#include <engine/rendering/IRenderer.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <engine/rendering/stb_image_write.h>
#include <engine/resloading/TextureResource.h>
#include <engine/scene/Scene.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <SDL_surface.h>

///------------------------------------------------------------------------------------------------

//static constexpr int NEW_TEXTURE_SIZE = 4096;
//static constexpr int DOWNSCALED_NAVMAP_IMAGE_SIZE = 4096;

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

void CreateGLTextureFromSurface(SDL_Surface* surface, GLuint& glTextureId, int& mode, const bool nnFiltering)
{
    GL_CALL(glGenTextures(1, &glTextureId));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, glTextureId));
    
    switch (surface->format->BytesPerPixel)
    {
        case 4:
            mode = GL_RGBA;
            break;
        case 3:
            mode = GL_RGB;
            break;
        default:
            throw std::runtime_error("Image with unknown channel profile");
            break;
    }

    GL_CALL(glTexImage2D
    (
        GL_TEXTURE_2D,
        0,
        mode,
        surface->w,
        surface->h,
        0,
        mode,
        GL_UNSIGNED_BYTE,
        surface->pixels
     ));
    
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nnFiltering ? GL_NEAREST : GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nnFiltering ? GL_NEAREST : GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
}

///------------------------------------------------------------------------------------------------

void ExportPixelsToPNG(const std::string& exportFilePath, unsigned char* pixels, const int imageSize)
{
    stbi_write_png(exportFilePath.c_str(), imageSize, imageSize, 4, pixels, imageSize * 4);
}

///------------------------------------------------------------------------------------------------

int GetDisplayRefreshRate()
{
    SDL_DisplayMode mode;
    int displayIndex = SDL_GetWindowDisplayIndex(&CoreSystemsEngine::GetInstance().GetContextWindow());
    
    // If we can't find the refresh rate, we'll return this:
    constexpr int DEFAULT_REFRESH_RATE = 60;
    if (SDL_GetDesktopDisplayMode(displayIndex, &mode) != 0)
    {
        return DEFAULT_REFRESH_RATE;
    }
    if (mode.refresh_rate == 0)
    {
        return DEFAULT_REFRESH_RATE;
    }
    return mode.refresh_rate;
}

///------------------------------------------------------------------------------------------------

}
