///------------------------------------------------------------------------------------------------
///  PersistentAccountDataSerializer.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 05/01/2024
///------------------------------------------------------------------------------------------------

#include <game/utils/PersistentAccountDataSerializer.h>

///------------------------------------------------------------------------------------------------

PersistentAccountDataSerializer::PersistentAccountDataSerializer()
    : serial::BaseDataFileSerializer("persistent", serial::DataFileType::PERSISTENCE_FILE_TYPE, serial::DataFileOpeningBehavior::DELAY_DATA_FILE_OPENING_TILL_FLUSH)
{
}

///------------------------------------------------------------------------------------------------
