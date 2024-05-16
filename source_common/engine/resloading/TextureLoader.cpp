///------------------------------------------------------------------------------------------------
///  TextureLoader.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#include <algorithm>
#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/OpenGL.h>
#include <engine/rendering/RenderingUtils.h>
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
    
    GLuint glTextureId; int mode;
    rendering::CreateGLTextureFromSurface(sdlSurface, glTextureId, mode);
    
    const auto surfaceWidth = sdlSurface->w;
    const auto surfaceHeight = sdlSurface->h;
    
    CoreSystemsEngine::GetInstance().GetResourceLoadingService().UnloadResource(resourcePath);
    
    return std::shared_ptr<IResource>(new TextureResource(surfaceWidth, surfaceHeight, mode, mode, glTextureId));
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
