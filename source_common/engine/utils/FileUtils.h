///------------------------------------------------------------------------------------------------
///  FileUtils.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///-----------------------------------------------------------------------------------------------

#ifndef FileUtils_h
#define FileUtils_h

///-----------------------------------------------------------------------------------------------

#include <algorithm> 
#include <string>
#include <vector>

#ifndef _WIN32
#include <dirent.h>
#else
#include <filesystem>
#endif

///-----------------------------------------------------------------------------------------------

namespace fileutils
{

///-----------------------------------------------------------------------------------------------
/// Extracts the file extension from the given file path.
/// @param[in] filePath the input file path.
/// @returns the extension (the string after the dot) of the file path given.
inline std::string GetFileExtension(const std::string& filePath)
{
    std::string pathExt;
    
    auto reverseIter = filePath.rbegin();
    
    while (reverseIter != filePath.rend() && (*reverseIter != '.'))
    {
        pathExt = *reverseIter + pathExt;
        reverseIter++;
    }
    
    return pathExt;
}

///-----------------------------------------------------------------------------------------------
/// Extracts, and returns the file name from the given file path.
/// @param[in] filePath the input file path.
/// @returns the file name (with the extension) from the file path given.
inline std::string GetFileName(const std::string& filePath)
{    
    std::string fileName;
    
    auto reverseIter = filePath.rbegin();
    while (reverseIter != filePath.rend() && (*reverseIter != '\\' && *reverseIter != '/'))
    {
        fileName = *reverseIter + fileName;
        reverseIter++;
    }
    
    return fileName;
}

///-----------------------------------------------------------------------------------------------
/// Extracts, and returns the file name without the extension from the given file path.
/// @param[in] filePath the input file path.
/// @returns the file name (without the extension) from the file path given.
inline std::string GetFileNameWithoutExtension(const std::string& filePath)
{
    std::string fileName = "";
    
    auto isRecordingFileName = false;
    auto reverseIter = filePath.rbegin();
    while (reverseIter != filePath.rend() && (*reverseIter != '\\' && *reverseIter != '/'))
    {
        
        if (!isRecordingFileName)
        {
            isRecordingFileName = *reverseIter == '.';
        }
        else
        {
            fileName = *reverseIter + fileName;
        }
        
        reverseIter++;
    }
    
    return fileName;
}

///-----------------------------------------------------------------------------------------------
/// Returns a vector of filenames (not absolute paths) in a given directory.
/// @param[in] directory to search in.
/// @returns a vector of filenames found in the given directory.
inline std::vector<std::string> GetAllFilenamesInDirectory(const std::string& directory)
{
    std::vector<std::string> fileNames;
    
#ifndef _WIN32
    DIR *dir;
    struct dirent *ent;
    
    if ((dir = opendir(directory.c_str())) != nullptr)
    {
        while ((ent = readdir(dir)) != nullptr)
        {
            const std::string fileName(ent->d_name);
            
            if (fileName[0] != '.')
            {
                fileNames.push_back(fileName);
            }
        }
        
        closedir(dir);
    }
#else
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        fileNames.push_back(GetFileName(entry.path().string()));
    }
#endif
    
    std::sort(fileNames.begin(), fileNames.end());
    return fileNames;
}

///-----------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* FileUtils_h */
