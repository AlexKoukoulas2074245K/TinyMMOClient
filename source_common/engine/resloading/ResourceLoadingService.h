///------------------------------------------------------------------------------------------------
///  ResourceLoadingService.h
///  Predators
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#ifndef ResourceLoadingService_h
#define ResourceLoadingService_h

///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/StringUtils.h>
#include <memory>
#include <string>        
#include <unordered_map>
#include <unordered_set>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

using ResourceId = size_t;
class IResource;
class IResourceLoader;

///------------------------------------------------------------------------------------------------

struct ResourceIdHasher
{
    std::size_t operator()(const ResourceId& key) const
    {
        return static_cast<std::size_t>(key);
    }
};

///------------------------------------------------------------------------------------------------
/// Dictates whether a resource will be force reloaded from disk every second or not.
/// (used for real time asset debugging)
enum ResourceReloadMode
{
    DONT_RELOAD, RELOAD_EVERY_SECOND
};

///------------------------------------------------------------------------------------------------
/// A service class aimed at providing resource loading, simple file IO, etc.
class ResourceLoadingService final
{
    friend struct CoreSystemsEngine::SystemsImpl;
public:
    static std::string RES_ROOT;    
    static std::string RES_DATA_ROOT;
    static std::string RES_SCRIPTS_ROOT;   
    static std::string RES_MESHES_ROOT;
    static std::string RES_MUSIC_ROOT;
    static std::string RES_SOUNDS_ROOT;
    static std::string RES_SHADERS_ROOT;
    static std::string RES_TEXTURES_ROOT;     
    static std::string RES_ATLASES_ROOT;
    static std::string RES_FONT_MAP_DATA_ROOT;
    
    ~ResourceLoadingService();
    ResourceLoadingService(const ResourceLoadingService&) = delete;
    ResourceLoadingService(ResourceLoadingService&&) = delete;
    const ResourceLoadingService& operator = (const ResourceLoadingService&) = delete;
    ResourceLoadingService& operator = (ResourceLoadingService&&) = delete;
    
    /// Initializes loaders for different types of assets.
    /// Called internally by the engine.
    void Initialize();
    
    /// Polls finished loading jobs in async mode
    void Update();
    
    /// Starts/Stop async loading of resources
    /// @param[in] asyncLoading whether or not the service will start loading resources asynchronously
    void SetAsyncLoading(const bool asyncLoading);
    
    /// Computes the hashed resource id, for a given file path.
    ///
    /// Both full paths, relative paths including the Resource Root, and relative
    /// paths excluding the Resource Root are supported.
    /// @param[in] resourcePath the path of the resource file.
    /// @param[in] isDynamicallyGenerated whether or not the resource has been dynamically generated on runtime
    /// @returns the computed resource id.
    ResourceId GetResourceIdFromPath(const std::string& resourcePath, const bool isDynamicallyGenerated);

    /// Loads and returns the resource id of the loaded resource that lives on the given path.
    ///
    /// Both full paths, relative paths including the Resource Root, and relative
    /// paths excluding the Resource Root are supported.
    /// @param[in] resourcePath the path of the resource file.
    /// @returns the loaded resource's id.
    ResourceId LoadResource(const std::string& resourcePath, const ResourceReloadMode resourceReloadingMode = ResourceReloadMode::DONT_RELOAD);

    /// Loads a collection of resources based on a given vector with their paths.
    ///
    /// Both full paths, relative paths including the Resource Root, and relative
    /// paths excluding the Resource Root are supported.
    /// @param[in] resourcePaths a vector containing the paths of the resource files.    
    void LoadResources(const std::vector<std::string>& resourcePaths);
    
    /// Creates a resource to correspond with a on-the-fly generated texture and returns its resourceId.
    ///
    /// @param[in] resourceName the name under which the resource will be hashed the id will be returned
    /// @param[in] textureId the texture id to set in the newly created texture resource
    /// @param[in] width the width of the texture
    /// @param[in] height the height of the texture
    /// @returns the generated resourceid
    ResourceId AddDynamicallyCreatedTextureResourceId(const std::string& resourceName, const unsigned int textureId, const int width, const int height);
    
    /// Checks whether a resource file exists under the given path.
    ///
    /// Both full paths, relative paths including the Resource Root, and relative
    /// paths excluding the Resource Root are supported.
    /// @param[in] resourcePath the path of the resource file.
    /// @returns whether or not a physical file exists in the specified path.
    bool DoesResourceExist(const std::string& resourcePath) const;
    
    /// Checks whether a resource has been loaded based on a file that exists under the given path.
    ///
    /// Both full paths, relative paths including the Resource Root, and relative
    /// paths excluding the Resource Root are supported.
    /// @param[in] resourcePath the path of the resource file.
    /// @param[in] isDynamicallyGenerated whether or not the resource has been dynamically generated on runtime
    /// @returns whether or not the resource has been loaded.
    bool HasLoadedResource(const std::string& resourcePath, const bool isDynamicallyGenerated) const;
    
    /// Unloads the specified resource loaded based on the given path.
    ///
    /// Any subsequent calls to get that
    /// resource will need to be preceeded by another Load to get the resource 
    /// back to the map of resources held by this service.
    /// Both full paths, relative paths including the Resource Root, and relative
    /// paths excluding the Resource Root are supported.
    /// @param[in] resourcePath the path of the resource file.        
    void UnloadResource(const std::string& resourcePath);
    
    /// Unloads the specified resource loaded based on the given path.
    ///
    /// Any subsequent calls to get that
    /// resource will need to be preceeded by another Load to get the resource
    /// back to the map of resources held by this service.  
    /// @param[in] resourceId the id of the resource to unload.    
    void UnloadResource(const ResourceId resourceId);
    
    /// Unloads all currently loaded dynamically created texture resources (i.e. via render to texture)
    void UnloadAllDynamicallyCreatedTextures();
    
    /// Unloads and then reloads all resources marked as RELOAD_EVERY_SECOND.
    void ReloadMarkedResourcesFromDisk();
    
    /// Gets the concrete type of the resource that was loaded based on the given path.
    ///    
    /// Both full paths, relative paths including the Resource Root, and relative
    /// paths excluding the Resource Root are supported.
    /// @tparam ResourceType the derived type of the requested resource.
    /// @param[in] resourcePath the path of the resource file.  
    /// returns the derived type of the resource.
    template<class ResourceType>
    inline ResourceType& GetResource(const std::string& resourcePath)
    {
        return static_cast<ResourceType&>(GetResource(resourcePath));
    }

    /// Gets the concrete type of the resource based on a given resource id.
    ///        
    /// @tparam ResourceType the derived type of the requested resource.
    /// @param[in] resourceId the id of the resource. 
    /// returns the derived type of the resource.
    template<class ResourceType>
    inline ResourceType& GetResource(const ResourceId resourceId)
    {
        return static_cast<ResourceType&>(GetResource(resourceId));
    }
    
    /// Gets the path of a resource given its ID.
    ///
    /// @param[in] resourceId the id of the resource.
    /// returns the original path of the resource
    std::string GetResourcePath(const ResourceId resourceId) const;
    
    /// Gets the number of  loading jobs to be completed
    int GetOustandingLoadingJobCount() const;
    
    /// Adds/Removes artificial loading jobs that will contribute to the outstanding loading job counts.
    ///
    /// @param[in] artificialLoadingJobCount count of artificial loading jobs to add (can be negative).
    void AddArtificialLoadingJobCount(const int artificialLoadingJobCount);
    
private:
    ResourceLoadingService();
    
    IResource& GetResource(const std::string& resourceRelativePath);
    IResource& GetResource(const ResourceId resourceId);    
    void LoadResourceInternal(const std::string& resourceRelativePath, const ResourceId resourceId);
   
    // Strips the leading RES_ROOT from the resourcePath given, if present
    std::string AdjustResourcePath(const std::string& resourcePath) const;
    
private:
    class AsyncLoaderWorker;
    
private:
    std::unordered_map<ResourceId, std::shared_ptr<IResource>, ResourceIdHasher> mResourceMap;
    std::unordered_map<strutils::StringId, IResourceLoader*, strutils::StringIdHasher> mResourceExtensionsToLoadersMap;
    std::unordered_map<ResourceId, std::string, ResourceIdHasher> mResourceIdMapToAutoReload;
    std::unordered_map<ResourceId, std::string, ResourceIdHasher> mResourceIdToPaths;
    std::unordered_set<ResourceId, ResourceIdHasher> mDynamicallyCreatedTextureResourceIds;
    std::unordered_set<ResourceId> mOutandingAsyncResourceIdsCurrentlyLoading;
    std::vector<std::unique_ptr<IResourceLoader>> mResourceLoaders;
    std::unique_ptr<AsyncLoaderWorker> mAsyncLoaderWorker;
    std::atomic<int> mOutstandingLoadingJobCount = 0;
    bool mInitialized = false;
    bool mAsyncLoading = false;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* ResourceLoadingService_h */
