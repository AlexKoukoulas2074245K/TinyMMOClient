///------------------------------------------------------------------------------------------------
///  ImageSurfaceLoader.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 08/12/2023.
///------------------------------------------------------------------------------------------------

#ifndef ImageSurfaceLoader_h
#define ImageSurfaceLoader_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/IResourceLoader.h>
#include <memory>
#include <SDL_stdinc.h>
#include <set>

///------------------------------------------------------------------------------------------------

struct SDL_Surface;

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

class ImageSurfaceLoader final: public IResourceLoader
{
    friend class ResourceLoadingService;

public:
    void VInitialize() override;
    bool VCanLoadAsync() const override;
    std::shared_ptr<IResource> VCreateAndLoadResource(const std::string& path) const override;

private:
    ImageSurfaceLoader() = default;
    
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* TextureLoader_h */
