///------------------------------------------------------------------------------------------------
///  BaseDataFileSerializer.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 16/12/2023
///------------------------------------------------------------------------------------------------

#include <chrono>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/BaseDataFileSerializer.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/utils/StringUtils.h>
#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif
#include <filesystem>

///------------------------------------------------------------------------------------------------

namespace serial
{

///------------------------------------------------------------------------------------------------

BaseDataFileSerializer::BaseDataFileSerializer(const std::string& fileNameWithoutExtension, const DataFileType& dataFileType, const DataFileOpeningBehavior fileOpeningBehavior)
    : mDataFileType(dataFileType)
{    
    std::string dataFileExtension = ".json";
    mFilename = fileNameWithoutExtension + dataFileExtension;
    
    if (fileOpeningBehavior == DataFileOpeningBehavior::OPEN_DATA_FILE_ON_CONSTRUCTION)
    {
        OpenDataFile();
    }
}

///------------------------------------------------------------------------------------------------

void BaseDataFileSerializer::FlushStateToFile()
{
    OpenDataFile();
    
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    auto secsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    
    mState["timestamp"] = secsSinceEpoch;
#if defined(MACOS) || defined(MOBILE_FLOW)
    mState["device_id"] = apple_utils::GetDeviceId();
    mState["device_name"] = apple_utils::GetDeviceName();
    mState["app_version"] = apple_utils::GetAppVersion();
#elif defined(WINDOWS)
#endif
    
    if (mFile.is_open())
    {
        auto checksumString = "&" + std::to_string(strutils::StringId(mState.dump(4)).GetStringId());
        
        mFile << mState.dump(4);
        mFile << checksumString;
        
        mFile.close();
    }
}

///------------------------------------------------------------------------------------------------

nlohmann::json& BaseDataFileSerializer::GetState()
{
    return mState;
}

///------------------------------------------------------------------------------------------------

void BaseDataFileSerializer::OpenDataFile()
{
    if (!mFile.is_open())
    {
        if (mDataFileType == DataFileType::PERSISTENCE_FILE_TYPE)
        {
    #if defined(MACOS) || defined(MOBILE_FLOW)
            auto directoryPath = apple_utils::GetPersistentDataDirectoryPath();
    #elif defined(WINDOWS)
            auto directoryPath = windows_utils::GetPersistentDataDirectoryPath();
    #endif
            
    #if defined(DESKTOP_FLOW)
            std::filesystem::create_directory(directoryPath);
    #endif
            mFile.open(directoryPath + mFilename);
        }
        else if (mDataFileType == DataFileType::ASSET_FILE_TYPE)
        {
            mFile.open(resources::ResourceLoadingService::RES_DATA_ROOT + mFilename);
        }
    }
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
