///------------------------------------------------------------------------------------------------
///  ImageSurfaceResource.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 08/12/2023.
///------------------------------------------------------------------------------------------------

#ifndef ImageSurfaceResource_h
#define ImageSurfaceResource_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/IResource.h>
#include <engine/utils/MathUtils.h>
#include <memory>
#include <SDL_surface.h>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

class ImageSurfaceResource final: public IResource
{
    friend class ImageSurfaceLoader;
    friend class ResourceLoadingService;
    
public:
    ~ImageSurfaceResource();
 
    SDL_Surface* GetSurface();
    
private:
    ImageSurfaceResource(SDL_Surface* surface);
    
private:
    SDL_Surface* mSurface;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* ImageSurfaceResource_h */
