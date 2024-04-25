///------------------------------------------------------------------------------------------------
///  BaseDataFileSerializer.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 16/12/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef BaseDataFileSerializer_h
#define BaseDataFileSerializer_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/SerializationDefinitions.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

///------------------------------------------------------------------------------------------------

namespace serial
{

///------------------------------------------------------------------------------------------------

enum class DataFileOpeningBehavior
{
    OPEN_DATA_FILE_ON_CONSTRUCTION,
    DELAY_DATA_FILE_OPENING_TILL_FLUSH
};

///------------------------------------------------------------------------------------------------

class BaseDataFileSerializer
{
public:
    BaseDataFileSerializer(const std::string& fileNameWithoutExtension, const DataFileType& dataFileType, const DataFileOpeningBehavior fileOpeningBehavior);
    virtual ~BaseDataFileSerializer() = default;
    
    void FlushStateToFile();
    
    nlohmann::json& GetState();
    
protected:
    nlohmann::json mState;
    
private:
    void OpenDataFile();
    
private:
    const DataFileType mDataFileType;
    std::string mFilename;
    std::ofstream mFile;
};

///------------------------------------------------------------------------------------------------

};

///------------------------------------------------------------------------------------------------

#endif /* BaseDataFileSerializer_h */
