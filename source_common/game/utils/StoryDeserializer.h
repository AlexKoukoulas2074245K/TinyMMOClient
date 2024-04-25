///------------------------------------------------------------------------------------------------
///  StoryDeserializer.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/01/2024
///------------------------------------------------------------------------------------------------

#ifndef StoryDeserializer_h
#define StoryDeserializer_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>
#include <engine/utils/BaseDataFileDeserializer.h>

///------------------------------------------------------------------------------------------------

class DataRepository;
class StoryDeserializer final: public serial::BaseDataFileDeserializer
{
public:
    StoryDeserializer(DataRepository& dataRepository);
};

///------------------------------------------------------------------------------------------------

#endif /* StoryDeserializer_h */
