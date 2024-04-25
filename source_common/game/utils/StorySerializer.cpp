///------------------------------------------------------------------------------------------------
///  StorySerializer.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/01/2024
///------------------------------------------------------------------------------------------------

#include <game/utils/StorySerializer.h>

///------------------------------------------------------------------------------------------------

StorySerializer::StorySerializer()
    : serial::BaseDataFileSerializer("story", serial::DataFileType::PERSISTENCE_FILE_TYPE, serial::DataFileOpeningBehavior::DELAY_DATA_FILE_OPENING_TILL_FLUSH)
{
}

///------------------------------------------------------------------------------------------------
