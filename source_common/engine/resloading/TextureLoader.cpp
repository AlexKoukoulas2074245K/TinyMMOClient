///------------------------------------------------------------------------------------------------
///  TextureLoader.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#include <algorithm>
#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/OpenGL.h>
#include <engine/resloading/ImageSurfaceResource.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/resloading/TextureLoader.h>
#include <engine/resloading/TextureResource.h>
#include <engine/utils/FileUtils.h>
#include <engine/utils/Logging.h>
#include <engine/utils/OSMessageBox.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/utils/StringUtils.h>
#include <fstream>
#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <unordered_map>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

void TextureLoader::VInitialize()
{
}

///------------------------------------------------------------------------------------------------

bool TextureLoader::VCanLoadAsync() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<IResource> TextureLoader::VCreateAndLoadResource(const std::string& resourcePath) const
{
    auto& surfaceResource = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<ImageSurfaceResource>(resourcePath);
    auto* sdlSurface = surfaceResource.GetSurface();
    
    GLuint glTextureId;
    GL_CALL(glGenTextures(1, &glTextureId));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, glTextureId));
    
    int mode;
    switch (sdlSurface->format->BytesPerPixel)
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
        sdlSurface->w,
        sdlSurface->h,
        0,
        mode,
        GL_UNSIGNED_BYTE,
        sdlSurface->pixels
     ));
    
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    
    GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
    
    const auto surfaceWidth = sdlSurface->w;
    const auto surfaceHeight = sdlSurface->h;
    
    CoreSystemsEngine::GetInstance().GetResourceLoadingService().UnloadResource(resourcePath);
    
    return std::shared_ptr<IResource>(new TextureResource(surfaceWidth, surfaceHeight, mode, mode, glTextureId));
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
