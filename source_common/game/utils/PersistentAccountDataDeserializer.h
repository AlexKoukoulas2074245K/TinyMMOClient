///------------------------------------------------------------------------------------------------
///  PersistentAccountDataDeserializer.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 05/01/2024
///------------------------------------------------------------------------------------------------

#ifndef PersistentAccountDataDeserializer_h
#define PersistentAccountDataDeserializer_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>
#include <engine/utils/BaseDataFileDeserializer.h>

///------------------------------------------------------------------------------------------------

class DataRepository;
class PersistentAccountDataDeserializer final: public serial::BaseDataFileDeserializer
{
public:
    PersistentAccountDataDeserializer(DataRepository& dataRepository);
};

///------------------------------------------------------------------------------------------------

#endif /* StoryDeserializer_h */
