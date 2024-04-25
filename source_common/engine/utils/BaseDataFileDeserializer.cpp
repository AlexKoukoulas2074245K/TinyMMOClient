///------------------------------------------------------------------------------------------------
///  BaseDataFileDeserializer.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 27/10/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <engine/utils/OSMessageBox.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/utils/StringUtils.h>
#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace serial
{

///------------------------------------------------------------------------------------------------

template<class T>
bool ValidateChecksum(T& contentsContainer)
{
    std::string checkSumString;
    
    if (!contentsContainer.empty() && contentsContainer.back() == '\n')
    {
        contentsContainer.pop_back();
    }
    
    while (!contentsContainer.empty())
    {
        if (contentsContainer.back() == '&')
        {
            break;
        }
        checkSumString = char(contentsContainer.back()) + checkSumString;
        contentsContainer.pop_back();
    }
    
    if (contentsContainer.empty())
    {
        return false;
    }
    
    contentsContainer.pop_back();
    
    if (contentsContainer.empty())
    {
        return false;
    }
    
    if (checkSumString == std::to_string(strutils::StringId(nlohmann::json::parse(contentsContainer).dump(4)).GetStringId()))
    {
        return true;
    }
    
    return false;
}

///------------------------------------------------------------------------------------------------

BaseDataFileDeserializer::BaseDataFileDeserializer(const std::string& fileNameWithoutExtension, const DataFileType& dataFileType, const WarnOnFileNotFoundBehavior warnOnFnFBehavior, const CheckSumValidationBehavior checkSumValidationBehavior)
{
    std::string dataFileExtension = ".json";
    
#if defined(MACOS) || defined(MOBILE_FLOW)
    auto filePath = (dataFileType == DataFileType::PERSISTENCE_FILE_TYPE ? apple_utils::GetPersistentDataDirectoryPath() : resources::ResourceLoadingService::RES_DATA_ROOT) + fileNameWithoutExtension + dataFileExtension;
#elif defined(WINDOWS)
    auto filePath = (dataFileType == DataFileType::PERSISTENCE_FILE_TYPE ? windows_utils::GetPersistentDataDirectoryPath() : resources::ResourceLoadingService::RES_DATA_ROOT) + fileNameWithoutExtension + dataFileExtension;
#endif
    
    std::ifstream dataFile(filePath);
    if (dataFile.is_open())
    {
        std::stringstream buffer;
        buffer << dataFile.rdbuf();
        auto contents = buffer.str();
        
        if (checkSumValidationBehavior == CheckSumValidationBehavior::VALIDATE_CHECKSUM && !ValidateChecksum(contents))
        {
            if (warnOnFnFBehavior == WarnOnFileNotFoundBehavior::WARN)
            {
                ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "Corrupted file", ("Data File " + filePath + " is corrupted.").c_str());
            }
            
            return;
        }
        
        if (contents.size() > 1)
        {
            mState = nlohmann::json::parse(contents);
        }
    }
    else if (warnOnFnFBehavior == WarnOnFileNotFoundBehavior::WARN)
    {
        ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "File not found", ("Data File " + filePath + " not found.").c_str());
    }
            
    dataFile.close();
}

///------------------------------------------------------------------------------------------------

const nlohmann::json& BaseDataFileDeserializer::GetState() const
{
    return mState;
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
