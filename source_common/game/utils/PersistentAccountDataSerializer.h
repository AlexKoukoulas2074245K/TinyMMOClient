///------------------------------------------------------------------------------------------------
///  PersistentAccountDataSerializer.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 05/01/2024
///------------------------------------------------------------------------------------------------

#ifndef PersistentAccountDataSerializer_h
#define PersistentAccountDataSerializer_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/BaseDataFileSerializer.h>
#include <engine/utils/StringUtils.h>
#include <vector>

///------------------------------------------------------------------------------------------------

class PersistentAccountDataSerializer final: public serial::BaseDataFileSerializer
{
public:
    PersistentAccountDataSerializer();
};

///------------------------------------------------------------------------------------------------

#endif /* PersistentAccountDataSerializer_h */
