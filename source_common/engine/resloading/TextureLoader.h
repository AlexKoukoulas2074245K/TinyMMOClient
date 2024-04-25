///------------------------------------------------------------------------------------------------
///  TextureLoader.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#ifndef TextureLoader_h
#define TextureLoader_h

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

class TextureLoader final: public IResourceLoader
{
    friend class ResourceLoadingService;

public:
    void VInitialize() override;
    bool VCanLoadAsync() const override;

    std::shared_ptr<IResource> VCreateAndLoadResource(const std::string& path) const override;

private:
    TextureLoader() = default;
    
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* TextureLoader_h */
