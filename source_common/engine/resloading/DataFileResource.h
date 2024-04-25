///------------------------------------------------------------------------------------------------
///  DataFileResource.h
///  Predators
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#ifndef DataFileResource_h
#define DataFileResource_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/IResource.h>
#include <string>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

class DataFileResource final: public IResource
{
    friend class DataFileLoader;

public:
    const std::string& GetContents() const;
    
private:
    DataFileResource(const std::string& contents);
    
private:
    const std::string mContents;

};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* DataFileResource_h */
