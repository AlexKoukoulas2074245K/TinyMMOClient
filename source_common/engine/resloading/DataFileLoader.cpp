///------------------------------------------------------------------------------------------------
///  DataFileLoader.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///-----------------------------------------------------------------------------------------------

#include <engine/resloading/DataFileLoader.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/utils/OSMessageBox.h>
#include <engine/utils/StringUtils.h>
#include <fstream>
#include <streambuf>

///-----------------------------------------------------------------------------------------------

namespace resources
{

///-----------------------------------------------------------------------------------------------
void DataFileLoader::VInitialize()
{ 
}

///-----------------------------------------------------------------------------------------------

bool DataFileLoader::VCanLoadAsync() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<IResource> DataFileLoader::VCreateAndLoadResource(const std::string& resourcePath) const
{
    std::ifstream file(resourcePath);
    
    if (!file.good())
    {
        ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "File could not be found", resourcePath.c_str());
        return nullptr;
    }
    
    std::string str;
    
    file.seekg(0, std::ios::end);
    str.reserve(static_cast<size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);
    
    str.assign((std::istreambuf_iterator<char>(file)),
               std::istreambuf_iterator<char>());
    
    return std::shared_ptr<IResource>(new DataFileResource(str));
}

///-----------------------------------------------------------------------------------------------

}

