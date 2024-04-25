///------------------------------------------------------------------------------------------------
///  ShaderLoader.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#ifndef ShaderLoader_h
#define ShaderLoader_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/IResourceLoader.h>
#include <engine/utils/StringUtils.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

using GLuint = unsigned int;

///------------------------------------------------------------------------------------------------

class ShaderLoader final : public IResourceLoader
{
    friend class ResourceLoadingService;

public:
    void VInitialize() override;
    bool VCanLoadAsync() const override;
    std::shared_ptr<IResource> VCreateAndLoadResource(const std::string& path) const override;

private:
    static const std::string VERTEX_SHADER_FILE_EXTENSION;
    static const std::string FRAGMENT_SHADER_FILE_EXTENSION;
    static const std::string GEOMETRY_SHADER_FILE_EXTENSION;
    
    ShaderLoader() = default;
    
    std::string ReadFileContents(const std::string& filePath) const;
    void PrependPreprocessorVars(std::string& shaderSource) const;
    void ReplaceIncludeDirectives(const std::string& inputFileString, std::string& outFinalShaderSource) const;
    std::unordered_map<strutils::StringId, GLuint, strutils::StringIdHasher> GetUniformNamesToLocationsMap
    (
        const GLuint programId,
        const std::string& shaderName, 
        const std::string& vertexShaderFileContents,
        const std::string& fragmentShaderFileContents,
        std::unordered_map<strutils::StringId, int, strutils::StringIdHasher>& uniformArrayElementCounts,
        std::vector<strutils::StringId>& samplerNamesInOrder
    ) const;
    
    void DumpFinalShaderContents(const std::string& vertexShaderContents, const std::string& fragmentShaderContents, const std::string& resourcePath) const;
    
private:
    std::string mGlslVersion;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* ShaderLoader_h */
