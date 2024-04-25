///------------------------------------------------------------------------------------------------
///  OBJMeshLoader.h
///  Predators
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#ifndef OBJMeshLoader_h
#define OBJMeshLoader_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/IResourceLoader.h>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

class OBJMeshLoader final: public IResourceLoader
{
    friend class ResourceLoadingService;
    
public:
    void VInitialize() override;
    bool VCanLoadAsync() const override;
    std::shared_ptr<IResource> VCreateAndLoadResource(const std::string& path) const override;
    
private:
    OBJMeshLoader() = default;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* OBJMeshLoader_h */
