///------------------------------------------------------------------------------------------------
///  WindowsUtils.cpp
///  Predators
///
///  Created by Alex Koukoulas on 20/01/2024.
///-----------------------------------------------------------------------------------------------

#include <platform_utilities/WindowsUtils.h>
#include <Windows.h>
#include <wininet.h>

///-----------------------------------------------------------------------------------------------

namespace windows_utils
{

///-----------------------------------------------------------------------------------------------

bool IsConnectedToTheInternet()
{
    return InternetCheckConnection("http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0);
}

///-----------------------------------------------------------------------------------------------

std::string GetPersistentDataDirectoryPath()
{
#define _CRT_SECURE_NO_WARNINGS
    auto appDataLocation = getenv("APPDATA");
    return std::string(appDataLocation) + "/RealmofBeasts/";
}

///-----------------------------------------------------------------------------------------------

}
