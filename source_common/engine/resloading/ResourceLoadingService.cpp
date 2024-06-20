///------------------------------------------------------------------------------------------------
///  ResourceLoadingService.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#include <cassert>
#include <engine/resloading/DataFileLoader.h>
#include <engine/resloading/IResource.h>
#include <engine/resloading/ImageSurfaceLoader.h>
#include <engine/resloading/OBJMeshLoader.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/resloading/ShaderLoader.h>
#include <engine/resloading/TextureLoader.h>
#include <engine/resloading/TextureResource.h>
#include <engine/utils/FileUtils.h>
#include <engine/utils/Logging.h>
#include <engine/utils/OSMessageBox.h>
#include <engine/utils/StringUtils.h>
#include <engine/utils/ThreadSafeQueue.h>
#include <engine/utils/TypeTraits.h>
#include <fstream>
#include <thread>

//#define UNZIP_FLOW
bool ARTIFICIAL_ASYNC_LOADING_DELAY = false;

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
std::string ResourceLoadingService::RES_ROOT = "../../assets/";
#elif __APPLE__
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
std::string ResourceLoadingService::RES_ROOT = "assets/";
#elif TARGET_OS_IPHONE
std::string ResourceLoadingService::RES_ROOT = "assets/";
#else
std::string ResourceLoadingService::RES_ROOT = "";
#endif
#else
std::string ResourceLoadingService::RES_ROOT = "";
#endif

std::string ResourceLoadingService::RES_DATA_ROOT          = RES_ROOT + "data/";
std::string ResourceLoadingService::RES_SCRIPTS_ROOT       = RES_ROOT + "scripts/";
std::string ResourceLoadingService::RES_MESHES_ROOT        = RES_ROOT + "meshes/";
std::string ResourceLoadingService::RES_MUSIC_ROOT         = RES_ROOT + "music/";
std::string ResourceLoadingService::RES_SOUNDS_ROOT        = RES_ROOT + "sounds/";
std::string ResourceLoadingService::RES_SHADERS_ROOT       = RES_ROOT + "shaders/";
std::string ResourceLoadingService::RES_TEXTURES_ROOT      = RES_ROOT + "textures/";
std::string ResourceLoadingService::RES_ATLASES_ROOT       = RES_TEXTURES_ROOT + "atlases/";
std::string ResourceLoadingService::RES_FONT_MAP_DATA_ROOT = RES_DATA_ROOT + "font_maps/";

static const std::string ZIPPED_ASSETS_FILE_NAME = "assets.zip";

///------------------------------------------------------------------------------------------------

class LoadingJob
{
public:
    LoadingJob(const IResourceLoader* loader, const std::string& resourcePath, const ResourceId targetResourceId)
    : mLoader(loader)
    , mResourcePath(resourcePath)
    , mTargetResourceId(targetResourceId)
    {
    }
    
    const IResourceLoader* mLoader;
    const std::string mResourcePath;
    const ResourceId mTargetResourceId;
};

class JobResult
{
public:
    JobResult(std::shared_ptr<IResource> resource, const IResourceLoader* loader, const std::string& resourcePath, const ResourceId targetResourceId)
    : mResource(std::move(resource))
    , mLoader(loader)
    , mResourcePath(resourcePath)
    , mTargetResourceId(targetResourceId)
    {
    }
    
    std::shared_ptr<IResource> mResource;
    const IResourceLoader* mLoader;
    const std::string mResourcePath;
    const ResourceId mTargetResourceId;
};


class ResourceLoadingService::AsyncLoaderWorker
{
public:
    void StartWorker()
    {
        mThread = std::thread([&]
                              {
            while(true)
            {
                using namespace std::chrono_literals;
                auto job = mJobs.dequeue();
                auto resource = job.mLoader->VCreateAndLoadResource(job.mResourcePath);
                
                if (ARTIFICIAL_ASYNC_LOADING_DELAY)
                {
                    std::this_thread::sleep_for(100ms);
                }
                
                mResults.enqueue({resource, job.mLoader, job.mResourcePath, job.mTargetResourceId});
            }
        });
        mThread.detach();
    }
    
public:
    ThreadSafeQueue<LoadingJob> mJobs;
    ThreadSafeQueue<JobResult> mResults;
    
private:
    std::thread mThread;
    
};

///------------------------------------------------------------------------------------------------

ResourceLoadingService::ResourceLoadingService()
{
    
}

///------------------------------------------------------------------------------------------------

ResourceLoadingService::~ResourceLoadingService()
{
}

///------------------------------------------------------------------------------------------------

void ResourceLoadingService::Initialize()
{
    using namespace strutils;
    
    RES_DATA_ROOT          = RES_ROOT + "data/";
    RES_SCRIPTS_ROOT       = RES_ROOT + "scripts/";
    RES_MESHES_ROOT        = RES_ROOT + "meshes/";
    RES_MUSIC_ROOT         = RES_ROOT + "music/";
    RES_SOUNDS_ROOT        = RES_ROOT + "sfx/";
    RES_SHADERS_ROOT       = RES_ROOT + "shaders/";
    RES_TEXTURES_ROOT      = RES_ROOT + "textures/";
    RES_ATLASES_ROOT       = RES_TEXTURES_ROOT + "atlases/";
    RES_FONT_MAP_DATA_ROOT = RES_DATA_ROOT + "font_maps/";
    
#ifdef UNZIP_FLOW
    objectiveC_utils::UnzipAssets((RES_ROOT + ZIPPED_ASSETS_FILE_NAME).c_str(), RES_ROOT.c_str());
#endif
    
    // No make unique due to constructing the loaders with their private constructors
    // via friendship
    mResourceLoaders.push_back(std::unique_ptr<ImageSurfaceLoader>(new ImageSurfaceLoader));
    mResourceLoaders.push_back(std::unique_ptr<DataFileLoader>(new DataFileLoader));
    mResourceLoaders.push_back(std::unique_ptr<ShaderLoader>(new ShaderLoader));
    mResourceLoaders.push_back(std::unique_ptr<OBJMeshLoader>(new OBJMeshLoader));
    mResourceLoaders.push_back(std::unique_ptr<TextureLoader>(new TextureLoader));
    
    // Map resource extensions to loaders
    mResourceExtensionsToLoadersMap[StringId("png")]  = mResourceLoaders[0].get();
    mResourceExtensionsToLoadersMap[StringId("json")] = mResourceLoaders[1].get();
    mResourceExtensionsToLoadersMap[StringId("dat")]  = mResourceLoaders[1].get();
    mResourceExtensionsToLoadersMap[StringId("txt")]  = mResourceLoaders[1].get();
    mResourceExtensionsToLoadersMap[StringId("lua")]  = mResourceLoaders[1].get();
    mResourceExtensionsToLoadersMap[StringId("xml")]  = mResourceLoaders[1].get();
    mResourceExtensionsToLoadersMap[StringId("vs")]   = mResourceLoaders[2].get();
    mResourceExtensionsToLoadersMap[StringId("fs")]   = mResourceLoaders[2].get();
    mResourceExtensionsToLoadersMap[StringId("obj")]  = mResourceLoaders[3].get();
    
    for (auto& resourceLoader: mResourceLoaders)
    {
        resourceLoader->VInitialize();
    }
    
    mInitialized = true;
    mAsyncLoaderWorker = std::make_unique<AsyncLoaderWorker>();
    mAsyncLoaderWorker->StartWorker();
}

///------------------------------------------------------------------------------------------------

void ResourceLoadingService::Update()
{
    while (mAsyncLoaderWorker->mResults.size())
    {
        auto finishedJob = mAsyncLoaderWorker->mResults.dequeue();
        mResourceMap[finishedJob.mTargetResourceId] = finishedJob.mResource;
        
        if (dynamic_cast<const ImageSurfaceLoader*>(finishedJob.mLoader) && !IsNavmapImage(finishedJob.mResourcePath))
        {
            mResourceMap[finishedJob.mTargetResourceId] = mResourceLoaders.back()->VCreateAndLoadResource(finishedJob.mResourcePath);
        }
        
        mResourceIdToPaths[finishedJob.mTargetResourceId] = finishedJob.mResourcePath;
        mOutandingAsyncResourceIdsCurrentlyLoading.erase(finishedJob.mTargetResourceId);
        mOutstandingLoadingJobCount--;
    }
}

///------------------------------------------------------------------------------------------------

void ResourceLoadingService::SetAsyncLoading(const bool asyncLoading)
{
    mAsyncLoading = asyncLoading;
    if (asyncLoading)
    {
        mOutandingAsyncResourceIdsCurrentlyLoading.clear();
        mOutstandingLoadingJobCount = 0;
    }
}

///------------------------------------------------------------------------------------------------

ResourceId ResourceLoadingService::GetResourceIdFromPath(const std::string& path, const bool isDynamicallyGenerated, const ResourceLoadingPathType resourceLoadingPathType /* = ResourceLoadingPathType::RELATIVE */)
{
    return strutils::GetStringHash(isDynamicallyGenerated ? path : AdjustResourcePath(path, resourceLoadingPathType));
}

///------------------------------------------------------------------------------------------------

ResourceId ResourceLoadingService::LoadResource(const std::string& resourcePath, const ResourceReloadMode resourceReloadingMode /* = ResourceReloadMode::DONT_RELOAD */, const ResourceLoadingPathType resourceLoadingPathType /* = ResourceLoadingPathType::RELATIVE */)
{
    const auto adjustedPath = AdjustResourcePath(resourcePath, resourceLoadingPathType);
    const auto resourceId = strutils::GetStringHash(adjustedPath);
    
    if (resourceReloadingMode == ResourceReloadMode::RELOAD_EVERY_SECOND)
    {
        mResourceIdMapToAutoReload[resourceId] = adjustedPath;
    }
    
    if (mResourceMap.count(resourceId))
    {
        return resourceId;
    }
    else
    {
        LoadResourceInternal(adjustedPath, resourceId, resourceLoadingPathType);
        return resourceId;
    }
}

///------------------------------------------------------------------------------------------------

void ResourceLoadingService::LoadResources(const std::vector<std::string>& resourcePaths)
{
    for (const auto& path: resourcePaths)
    {
        LoadResource(path);
    }
}

///------------------------------------------------------------------------------------------------

ResourceId ResourceLoadingService::AddDynamicallyCreatedTextureResourceId(const std::string& resourceName, unsigned int textureId, const int width, const int height)
{
    const auto resourceId = strutils::GetStringHash(resourceName);
    if (!mResourceMap.count(resourceId))
    {
        mResourceIdToPaths[resourceId] = resourceName;
        mResourceMap[resourceId] = std::unique_ptr<TextureResource>(new TextureResource(width, height, 0, 0, textureId));
        mDynamicallyCreatedTextureResourceIds.insert(resourceId);
    }
    return resourceId;
}

///------------------------------------------------------------------------------------------------

bool ResourceLoadingService::DoesResourceExist(const std::string& resourcePath, const ResourceLoadingPathType resourceLoadingPathType /* = ResourceLoadingPathType::RELATIVE */) const
{
    const auto adjustedPath = AdjustResourcePath(resourcePath, resourceLoadingPathType);
    std::fstream resourceFileCheck(resourcePath);
    return resourceFileCheck.operator bool();
}

///------------------------------------------------------------------------------------------------

bool ResourceLoadingService::HasLoadedResource(const ResourceId resourceId) const
{
    return mResourceMap.count(resourceId) != 0;
}

///------------------------------------------------------------------------------------------------

bool ResourceLoadingService::HasLoadedResource(const std::string& resourcePath, const bool isDynamicallyGenerated, const ResourceLoadingPathType resourceLoadingPathType /* = ResourceLoadingPathType::RELATIVE */) const
{
    const auto adjustedPath = AdjustResourcePath(resourcePath, resourceLoadingPathType);
    const auto resourceId = strutils::GetStringHash(isDynamicallyGenerated ? resourcePath : adjustedPath);
    
    return HasLoadedResource(resourceId);
}

///------------------------------------------------------------------------------------------------

void ResourceLoadingService::UnloadResource(const std::string& resourcePath, const ResourceLoadingPathType resourceLoadingPathType /* = ResourceLoadingPathType::RELATIVE */)
{
    const auto adjustedPath = AdjustResourcePath(resourcePath, resourceLoadingPathType);
    const auto resourceId = strutils::GetStringHash(adjustedPath);
    mResourceMap.erase(resourceId);
}

///------------------------------------------------------------------------------------------------

void ResourceLoadingService::UnloadResource(const ResourceId resourceId)
{
    logging::Log(logging::LogType::INFO, "Unloading asset: %s", std::to_string(resourceId).c_str());
    mResourceMap.erase(resourceId);
}

///------------------------------------------------------------------------------------------------

void ResourceLoadingService::UnloadAllDynamicallyCreatedTextures()
{
    for (const auto& dynamicallyCreatedResourceId: mDynamicallyCreatedTextureResourceIds)
    {
        UnloadResource(dynamicallyCreatedResourceId);
    }
    mDynamicallyCreatedTextureResourceIds.clear();
}

///------------------------------------------------------------------------------------------------

void ResourceLoadingService::ReloadMarkedResourcesFromDisk()
{
    for (auto [resourceId, relativePath]: mResourceIdMapToAutoReload)
    {
        UnloadResource(resourceId);
        LoadResourceInternal(relativePath, resourceId, ResourceLoadingPathType::RELATIVE);
    }
}

///------------------------------------------------------------------------------------------------

IResource& ResourceLoadingService::GetResource(const std::string& resourcePath, const ResourceLoadingPathType resourceLoadingPathType /* = ResourceLoadingPathType::RELATIVE */)
{
    const auto adjustedPath = AdjustResourcePath(resourcePath, resourceLoadingPathType);
    const auto resourceId = strutils::GetStringHash(adjustedPath);
    return GetResource(resourceId);
}

///------------------------------------------------------------------------------------------------

IResource& ResourceLoadingService::GetResource(const ResourceId resourceId)
{
    if (mResourceMap.count(resourceId))
    {
        return *mResourceMap[resourceId];
    }
    
    assert(false && "Resource could not be found");
    return *mResourceMap[resourceId];
}

///------------------------------------------------------------------------------------------------

std::string ResourceLoadingService::GetResourcePath(const ResourceId resourceId) const
{
    auto resourcePathEntryIter = mResourceIdToPaths.find(resourceId);
    if (resourcePathEntryIter != mResourceIdToPaths.cend())
    {
        return resourcePathEntryIter->second;
    }
    return "";
}

///------------------------------------------------------------------------------------------------

int ResourceLoadingService::GetOustandingLoadingJobCount() const
{
    return mOutstandingLoadingJobCount;
}

///------------------------------------------------------------------------------------------------

void ResourceLoadingService::AddArtificialLoadingJobCount(const int artificialLoadingJobCount)
{
    mOutstandingLoadingJobCount += artificialLoadingJobCount;
}

///------------------------------------------------------------------------------------------------

void ResourceLoadingService::LoadResourceInternal(const std::string& resourcePath, const ResourceId resourceId, const ResourceLoadingPathType resourceLoadingPathType)
{
    // Get resource extension
    const auto resourceFileExtension = fileutils::GetFileExtension(resourcePath);
    const auto resourceFileName = fileutils::GetFileName(resourcePath);
    
    // Pick appropriate loader
    strutils::StringId fileExtension(fileutils::GetFileExtension(resourcePath));
    auto loadersIter = mResourceExtensionsToLoadersMap.find(fileExtension);
    if (loadersIter != mResourceExtensionsToLoadersMap.end())
    {
        auto* selectedLoader = mResourceExtensionsToLoadersMap.at(strutils::StringId(fileutils::GetFileExtension(resourcePath)));
        
        if (mAsyncLoading && selectedLoader->VCanLoadAsync() && !mOutandingAsyncResourceIdsCurrentlyLoading.count(resourceId))
        {
            if (resourceLoadingPathType == ResourceLoadingPathType::RELATIVE)
            {
                mAsyncLoaderWorker->mJobs.enqueue(LoadingJob(selectedLoader, RES_ROOT + resourcePath, resourceId));
            }
            else
            {
                mAsyncLoaderWorker->mJobs.enqueue(LoadingJob(selectedLoader, resourcePath, resourceId));
            }
            
            mOutstandingLoadingJobCount++;
            mOutandingAsyncResourceIdsCurrentlyLoading.insert(resourceId);
        }
        else if (!mOutandingAsyncResourceIdsCurrentlyLoading.count(resourceId))
        {
            auto loadedResource = resourceLoadingPathType == ResourceLoadingPathType::RELATIVE ? selectedLoader->VCreateAndLoadResource(RES_ROOT + resourcePath) : selectedLoader->VCreateAndLoadResource(resourcePath);
            mResourceMap[resourceId] = std::move(loadedResource);
            
            // Images are loaded in 2 steps so that we can separate the file I/O and GL part
            // for async loading
            if (dynamic_cast<ImageSurfaceLoader*>(selectedLoader) && !IsNavmapImage(resourceFileName))
            {
                if (resourceLoadingPathType == ResourceLoadingPathType::RELATIVE)
                {
                    loadedResource = mResourceLoaders.back()->VCreateAndLoadResource(RES_ROOT + resourcePath);
                }
                else
                {
                    loadedResource = mResourceLoaders.back()->VCreateAndLoadResource(resourcePath);
                }
                
                mResourceMap[resourceId] = std::move(loadedResource);
            }
            
            logging::Log(logging::LogType::INFO, "Finished loading asset: %s in %s", resourcePath.c_str(), std::to_string(resourceId).c_str());
            mResourceIdToPaths[resourceId] = resourcePath;
        }
    }
    else
    {
        ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "Unable to find loader for given extension", "A loader could not be found for extension: " + fileExtension.GetString());
    }
}

///------------------------------------------------------------------------------------------------

std::string ResourceLoadingService::AdjustResourcePath(const std::string& resourcePath, const ResourceLoadingPathType resourceLoadingPathType) const
{
    if (resourceLoadingPathType == ResourceLoadingPathType::ABSOLUTE)
    {
        return resourcePath;
    }
    
    return !strutils::StringStartsWith(resourcePath, RES_ROOT) ? resourcePath : resourcePath.substr(RES_ROOT.size(), resourcePath.size() - RES_ROOT.size());
}

///------------------------------------------------------------------------------------------------

bool ResourceLoadingService::IsNavmapImage(const std::string& fileName) const
{
    return strutils::StringEndsWith(fileName, "_navmap.png");
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
