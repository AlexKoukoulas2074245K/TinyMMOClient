///------------------------------------------------------------------------------------------------
///  CloudKitUtils.h
///  Predators
///
///  Created by Alex Koukoulas on 22/01/2024.
///-----------------------------------------------------------------------------------------------

#ifndef CloudKitUtils_h
#define CloudKitUtils_h

///-----------------------------------------------------------------------------------------------

#include <functional>
#include <stdarg.h>
#include <stdio.h>  
#include <string>

///-----------------------------------------------------------------------------------------------

namespace cloudkit_utils
{

///-----------------------------------------------------------------------------------------------

struct QueryResultData
{
    bool mSuccessfullyQueriedAtLeastOneFileField = false;
    std::string mPersistentProgressRawString;
    std::string mStoryProgressRawString;
    std::string mLastBattleRawString;
};
void QueryPlayerProgress(std::function<void(QueryResultData)> onQueryCompleteCallback);
void SavePlayerProgress();

///-----------------------------------------------------------------------------------------------

}

///-----------------------------------------------------------------------------------------------

#endif /* CloudKitUtils_h */
