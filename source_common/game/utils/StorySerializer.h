///------------------------------------------------------------------------------------------------
///  StorySerializer.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/01/2024
///------------------------------------------------------------------------------------------------

#ifndef StorySerializer_h
#define StorySerializer_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/BaseDataFileSerializer.h>
#include <engine/utils/StringUtils.h>
#include <vector>

///------------------------------------------------------------------------------------------------

class StorySerializer final: public serial::BaseDataFileSerializer
{
public:
    StorySerializer();
};

///------------------------------------------------------------------------------------------------

#endif /* StorySerializer_h */
