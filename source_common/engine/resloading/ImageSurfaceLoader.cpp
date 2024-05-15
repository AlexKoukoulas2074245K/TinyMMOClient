///------------------------------------------------------------------------------------------------
///  ImageSurfaceLoader.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 08/12/2023.
///------------------------------------------------------------------------------------------------

#include <algorithm>
#include <engine/resloading/ImageSurfaceLoader.h>
#include <engine/resloading/ImageSurfaceResource.h>
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

void ImageSurfaceLoader::VInitialize()
{
    SDL_version imgCompiledVersion;
    SDL_IMAGE_VERSION(&imgCompiledVersion);
    
    const auto* imgLinkedVersion = IMG_Linked_Version();
    
    const auto imgMajorVersionConsistency = imgCompiledVersion.major == imgLinkedVersion->major;
    const auto imgMinorVersionConsistency = imgCompiledVersion.minor == imgLinkedVersion->minor;
    const auto imgPatchConsistency = imgCompiledVersion.patch == imgLinkedVersion->patch;
    const auto imgVersionConsistency = imgMajorVersionConsistency && imgMinorVersionConsistency && imgPatchConsistency;
    
    const auto sdlImageInitFlags = IMG_INIT_PNG;
    if (!imgVersionConsistency || IMG_Init(sdlImageInitFlags) != sdlImageInitFlags)
    {
    }
    
    logging::Log(logging::LogType::INFO, "Successfully initialized SDL_image version %d.%d.%d", imgCompiledVersion.major, imgCompiledVersion.minor, imgCompiledVersion.patch);
}

///------------------------------------------------------------------------------------------------

bool ImageSurfaceLoader::VCanLoadAsync() const
{
    return true;
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<IResource> ImageSurfaceLoader::VCreateAndLoadResource(const std::string& resourcePath) const
{
    std::ifstream file(resourcePath);
        
    if (!file.good())
    {
        ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "File could not be found", resourcePath.c_str());
        return nullptr;
    }
    
    auto* sdlSurface = IMG_Load(resourcePath.c_str());
    
    if (!sdlSurface)
    {
        ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "SDL_image could not load texture", IMG_GetError());
        return nullptr;
    }
    
#if defined(MACOS) || defined(MOBILE_FLOW)
    // OpenGL ES 3.0 format shenanigans
    SDL_LockSurface(sdlSurface);
    for (int y = 0; y < sdlSurface->h; ++y)
    {
        for (int x = 0; x < sdlSurface->w; ++x)
        {
            Uint32 * const pixel = (Uint32 *) ((Uint8 *) sdlSurface->pixels + y * sdlSurface->pitch + x * sdlSurface->format->BytesPerPixel);
            *pixel = (*pixel & 0xFF000000) + ((*pixel & 0x000000FF) << 16) + (*pixel & 0x0000FF00) + ((*pixel & 0x00FF0000) >> 16);
        }
    }
    SDL_UnlockSurface(sdlSurface);
#endif
    
    return std::shared_ptr<IResource>(new ImageSurfaceResource(sdlSurface));
}



///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
