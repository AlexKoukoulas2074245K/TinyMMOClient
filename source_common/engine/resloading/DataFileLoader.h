///------------------------------------------------------------------------------------------------
///  DataFileLoader.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///-----------------------------------------------------------------------------------------------

#ifndef DataFileLoader_h
#define DataFileLoader_h

///-----------------------------------------------------------------------------------------------

#include <engine/resloading/IResourceLoader.h>

///-----------------------------------------------------------------------------------------------

namespace resources
{

///-----------------------------------------------------------------------------------------------

class DataFileLoader final: public IResourceLoader
{
    friend class ResourceLoadingService;

public:
    void VInitialize() override;
    bool VCanLoadAsync() const override;
    std::shared_ptr<IResource> VCreateAndLoadResource(const std::string& path) const override;
    
private:
    DataFileLoader() = default;

};

///-----------------------------------------------------------------------------------------------

}

///-----------------------------------------------------------------------------------------------

#endif /* DataFileLoader_h */
