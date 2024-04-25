///------------------------------------------------------------------------------------------------
///  CloudKitUtils.mm
///  Predators
///
///  Created by Alex Koukoulas on 22/01/2024.
///-----------------------------------------------------------------------------------------------

#include <platform_utilities/AppleUtils.h>
#include <platform_utilities/CloudKitUtils.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <nlohmann/json.hpp>
#import <CloudKit/CloudKit.h>

///-----------------------------------------------------------------------------------------------

namespace cloudkit_utils
{

///-----------------------------------------------------------------------------------------------

CKRecord* currentProgressRecord = nil;
bool saveInProgress = false;

///-----------------------------------------------------------------------------------------------

void QueryPlayerProgress(std::function<void(QueryResultData)> onQueryCompleteCallback)
{
    if (!apple_utils::IsConnectedToTheInternet())
    {
        return;
    }
    
    NSString* containerIdentifier = @"iCloud.com.alexkoukoulas2074245k.Predators";
    CKContainer* customContainer = [CKContainer containerWithIdentifier:containerIdentifier];
    CKDatabase* privateDatabase = [customContainer privateCloudDatabase];
    
    CKQuery* query = [[CKQuery alloc] initWithRecordType:@"PlayerProgress" predicate:[NSPredicate predicateWithValue:YES]];
    query.sortDescriptors = @[[NSSortDescriptor sortDescriptorWithKey:@"modificationDate" ascending:NO]];
    
    [privateDatabase performQuery:query inZoneWithID:nil completionHandler:^(NSArray *results, NSError *error) {
        QueryResultData resultData;
        if (error)
        {
            NSLog(@"Error querying progress: %@", error);
        }
        else
        {
            if (results.count > 0)
            {
                currentProgressRecord = results.firstObject;
                NSData* persistentData = currentProgressRecord[@"persistent"];
                NSData* storyData = currentProgressRecord[@"story"];
                NSData* lastBattleData = currentProgressRecord[@"last_battle"];
                
                NSString* persistentDataString = [[NSString alloc] initWithData:persistentData encoding:NSUTF8StringEncoding];
                NSString* storyDataString = [[NSString alloc] initWithData:storyData encoding:NSUTF8StringEncoding];
                NSString* lastBattleDataString = [[NSString alloc] initWithData:lastBattleData encoding:NSUTF8StringEncoding];
                
                resultData.mPersistentProgressRawString = std::string([persistentDataString UTF8String]);
                resultData.mStoryProgressRawString = std::string([storyDataString UTF8String]);
                resultData.mLastBattleRawString = std::string([lastBattleDataString UTF8String]);
                resultData.mSuccessfullyQueriedAtLeastOneFileField = true;
                
                onQueryCompleteCallback(resultData);
            }
            else
            {
                currentProgressRecord = [[CKRecord alloc] initWithRecordType:@"PlayerProgress"];
                NSLog(@"No progress data found");
            }
        }
    }];
}

///-----------------------------------------------------------------------------------------------

void SavePlayerProgress()
{
    if (!apple_utils::IsConnectedToTheInternet())
    {
        return;
    }
    
    if (!currentProgressRecord)
    {
        QueryPlayerProgress([](QueryResultData){});
        return;
    }
    
    auto persistentDataFileReaderLambda = [](const std::string& persistentFileNameWithoutExtension)
    {
        std::string dataFileExtension = ".json";
        
        auto filePath = apple_utils::GetPersistentDataDirectoryPath() + persistentFileNameWithoutExtension + dataFileExtension;
        
        std::ifstream dataFile(filePath);
        
        if (dataFile.is_open())
        {
            return std::string((std::istreambuf_iterator<char>(dataFile)), std::istreambuf_iterator<char>());
        }
        
        return std::string();
    };
    
    NSData* persistentData = [[NSString stringWithUTF8String:persistentDataFileReaderLambda("persistent").c_str()] dataUsingEncoding:NSUTF8StringEncoding];
    NSData* storyData = [[NSString stringWithUTF8String:persistentDataFileReaderLambda("story").c_str()] dataUsingEncoding:NSUTF8StringEncoding];
    NSData* lastBattleData = [[NSString stringWithUTF8String:persistentDataFileReaderLambda("last_battle").c_str()] dataUsingEncoding:NSUTF8StringEncoding];
    
    currentProgressRecord[@"persistent"] = persistentData;
    currentProgressRecord[@"story"] = storyData;
    currentProgressRecord[@"last_battle"] = lastBattleData;
    
    // Batch cloud writes
    if (saveInProgress)
    {
        return;
    }
    
    // Recheck and only save if we are ahead or equal in seen transactions
    NSString* containerIdentifier = @"iCloud.com.alexkoukoulas2074245k.Predators";
    CKContainer* customContainer = [CKContainer containerWithIdentifier:containerIdentifier];
    CKDatabase* privateDatabase = [customContainer privateCloudDatabase];
    
    CKQuery* query = [[CKQuery alloc] initWithRecordType:@"PlayerProgress" predicate:[NSPredicate predicateWithValue:YES]];
    query.sortDescriptors = @[[NSSortDescriptor sortDescriptorWithKey:@"modificationDate" ascending:NO]];
    
    [privateDatabase performQuery:query inZoneWithID:nil completionHandler:^(NSArray *results, NSError *error) {
        saveInProgress = false;
        QueryResultData resultData;
        if (error)
        {
            NSLog(@"Error querying progress: %@", error);
        }
        else
        {
            bool canSave = false;
            if (results.count == 0)
            {
                canSave = true;
            }
            if (results.count > 0)
            {
                NSString* cloudPersistentData = [[NSString alloc] initWithData:results.firstObject[@"persistent"] encoding:NSUTF8StringEncoding];
                std::string cloudPersistentDataString([cloudPersistentData UTF8String]);
                
                NSString* localPersistentData = [[NSString alloc] initWithData:currentProgressRecord[@"persistent"] encoding:NSUTF8StringEncoding];
                std::string localPersistentDataString([localPersistentData UTF8String]);
                
                auto populateSuccessfulTransactionIds = [](const std::string& dataString, std::vector<std::string>& transactionIds)
                {
                    std::string pattern = "successful_transaction_ids\":";
                    std::stringstream extractedString;
                    
                    auto pos = dataString.find(pattern);
                    if (pos == dataString.npos)
                    {
                        return;
                    }
                    
                    pos += pattern.size();
                    do
                    {
                        extractedString << dataString.at(pos);
                    } while(dataString.at(pos++) != ']');
                    
                    auto test = extractedString.str();
                    auto parsedJson = nlohmann::json::parse(extractedString.str());
                    transactionIds = parsedJson.get<std::vector<std::string>>();
                };
                
                std::vector<std::string> cloudSuccessfulTransactionIds;
                std::vector<std::string> localSuccessfulTransactionIds;
                
                populateSuccessfulTransactionIds(cloudPersistentDataString, cloudSuccessfulTransactionIds);
                populateSuccessfulTransactionIds(localPersistentDataString, localSuccessfulTransactionIds);
                
                // If local progression transaction ids are behind cloud then we do not save.
                canSave = localSuccessfulTransactionIds.size() >= cloudSuccessfulTransactionIds.size();
            }
            
            if (canSave)
            {
                [privateDatabase saveRecord:currentProgressRecord completionHandler:^(CKRecord *record, NSError *error) {
                    if (error) {
                        NSLog(@"Error saving progress: %@", error);
                    } else {
                        NSLog(@"Cloud progress saved successfully");
                        currentProgressRecord = record;
                    }
                }];
            }
        }
    }];
    
    saveInProgress = true;
}

///-----------------------------------------------------------------------------------------------

}

///-----------------------------------------------------------------------------------------------
