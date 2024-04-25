///------------------------------------------------------------------------------------------------
///  ShaderLoader.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#include <engine/rendering/OpenGL.h>
#include <engine/resloading/ShaderResource.h>
#include <engine/resloading/ShaderLoader.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/Logging.h>
#include <engine/utils/OSMessageBox.h>
#include <engine/utils/StringUtils.h>
#include <fstream>  
#include <streambuf>

///------------------------------------------------------------------------------------------------

//#define DEBUG_SHADER_LOADING

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

const std::string ShaderLoader::VERTEX_SHADER_FILE_EXTENSION = ".vs";
const std::string ShaderLoader::FRAGMENT_SHADER_FILE_EXTENSION = ".fs";

///------------------------------------------------------------------------------------------------

static void ExtractUniformFromLine(const std::string& line, const std::string& shaderName, const GLuint programId, std::unordered_map<strutils::StringId, GLuint, strutils::StringIdHasher>& outUniformNamesToLocations,     std::unordered_map<strutils::StringId, int, strutils::StringIdHasher>& outUniformArrayElementCounts, std::vector<strutils::StringId>& outSamplerNamesInOrder);

///------------------------------------------------------------------------------------------------

void ShaderLoader::VInitialize()
{
    mGlslVersion = reinterpret_cast<const char*>(GL_NO_CHECK_CALL(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    strutils::StringReplaceAllOccurences("\\.", "", mGlslVersion);
}

///------------------------------------------------------------------------------------------------

bool ShaderLoader::VCanLoadAsync() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<IResource> ShaderLoader::VCreateAndLoadResource(const std::string& resourcePathWithExtension) const
{
    // Since the shader loading is signalled by the .vs or .fs extension, we need to trim it here after
    // being added by the ResourceLoadingService prior to this call
    const auto resourcePath = resourcePathWithExtension.substr(0, resourcePathWithExtension.size() - 3);
    
    // Generate vertex shader id
    const auto vertexShaderId = GL_NO_CHECK_CALL(glCreateShader(GL_VERTEX_SHADER));
    
    // Read vertex shader source
    std::string finalVertexShaderContents;
    auto vertexShaderFileContents = ReadFileContents(resourcePath + VERTEX_SHADER_FILE_EXTENSION);
    PrependPreprocessorVars(vertexShaderFileContents);
    ReplaceIncludeDirectives(vertexShaderFileContents, finalVertexShaderContents);
    
    const char* vertexShaderFileContentsPtr = finalVertexShaderContents.c_str();
    
    // Compile vertex shader
    GL_CALL(glShaderSource(vertexShaderId, 1, &vertexShaderFileContentsPtr, nullptr));
    GL_CALL(glCompileShader(vertexShaderId));
    
    // Check vertex shader compilation
    std::string vertexShaderInfoLog;
    GLint vertexShaderInfoLogLength;
    GL_CALL(glGetShaderiv(vertexShaderId, GL_INFO_LOG_LENGTH, &vertexShaderInfoLogLength));
    if (vertexShaderInfoLogLength > 0)
    {
        vertexShaderInfoLog.clear();
        vertexShaderInfoLog.reserve(vertexShaderInfoLogLength);
        GL_CALL(glGetShaderInfoLog(vertexShaderId, vertexShaderInfoLogLength, nullptr, &vertexShaderInfoLog[0]));
        
        bool containsError = strutils::StringContains(std::string(vertexShaderInfoLog.c_str()), "ERROR:");
        if (containsError)
        {
            ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "Error Compiling Vertex Shader: " +  std::string(resourcePath.c_str()), vertexShaderInfoLog.c_str());
        }
        logging::Log((containsError ? logging::LogType::ERROR : logging::LogType::WARNING), "%s Compiling Vertex Shader: %s\n%s", (containsError ? "Error" : "Warning"), resourcePath.c_str(), vertexShaderInfoLog.c_str());
    }
    
    // Generate fragment shader id
    const auto fragmentShaderId = GL_NO_CHECK_CALL(glCreateShader(GL_FRAGMENT_SHADER));
    
    // Read vertex shader source
    std::string finalFragmentShaderContents;
    auto fragmentShaderFileContents = ReadFileContents(resourcePath + FRAGMENT_SHADER_FILE_EXTENSION);
    PrependPreprocessorVars(fragmentShaderFileContents);
    ReplaceIncludeDirectives(fragmentShaderFileContents, finalFragmentShaderContents);
    
#if defined(DEBUG_SHADER_LOADING)
    DumpFinalShaderContents(finalVertexShaderContents, finalFragmentShaderContents, resourcePath);
#endif
    
    const char* fragmentShaderFileContentsPtr = finalFragmentShaderContents.c_str();
    
    GL_CALL(glShaderSource(fragmentShaderId, 1, &fragmentShaderFileContentsPtr, nullptr));
    GL_CALL(glCompileShader(fragmentShaderId));
    
    std::string fragmentShaderInfoLog;
    GLint fragmentShaderInfoLogLength;
    GL_CALL(glGetShaderiv(fragmentShaderId, GL_INFO_LOG_LENGTH, &fragmentShaderInfoLogLength));
    if (fragmentShaderInfoLogLength > 0)
    {
        fragmentShaderInfoLog.clear();
        fragmentShaderInfoLog.reserve(fragmentShaderInfoLogLength);
        GL_CALL(glGetShaderInfoLog(fragmentShaderId, fragmentShaderInfoLogLength, nullptr, &fragmentShaderInfoLog[0]));
        
        bool containsError = strutils::StringContains(std::string(fragmentShaderInfoLog.c_str()), "ERROR:");
        if (containsError)
        {
            ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "Error Compiling Fragment Shader: " +  std::string(resourcePath.c_str()), fragmentShaderInfoLog.c_str());
        }
        logging::Log((containsError ? logging::LogType::ERROR : logging::LogType::WARNING), "%s Compiling Fragment Shader: %s\n%s", (containsError ? "Error" : "Warning"), resourcePath.c_str(), fragmentShaderInfoLog.c_str());
    }
    
    // Link shader program
    const auto programId = GL_NO_CHECK_CALL(glCreateProgram());
    GL_CALL(glAttachShader(programId, vertexShaderId));
    GL_CALL(glAttachShader(programId, fragmentShaderId));
    GL_CALL(glLinkProgram(programId));
    
    // Destroy intermediate compiled shaders
    GL_CALL(glDetachShader(programId, vertexShaderId));
    GL_CALL(glDetachShader(programId, fragmentShaderId));
    GL_CALL(glDeleteShader(vertexShaderId));
    GL_CALL(glDeleteShader(fragmentShaderId));
    
    std::unordered_map<strutils::StringId, int, strutils::StringIdHasher> uniformArrayElementCounts;
    std::vector<strutils::StringId> samplerNamesInOrder;
    
    const auto uniformNamesToLocations = GetUniformNamesToLocationsMap(programId, resourcePath,  finalVertexShaderContents, finalFragmentShaderContents, uniformArrayElementCounts, samplerNamesInOrder);
    
    return std::make_shared<ShaderResource>(uniformNamesToLocations, uniformArrayElementCounts, samplerNamesInOrder, programId);
}

///------------------------------------------------------------------------------------------------

std::string ShaderLoader::ReadFileContents(const std::string& filePath) const
{
    std::ifstream file(filePath);
    
    if (!file.good())
    {
        ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "File could not be found", filePath.c_str());
        return "";
    }
    
    std::string contents;
    
    file.seekg(0, std::ios::end);
    contents.reserve(static_cast<size_t>(file.tellg()));
    file.seekg(0, std::ios::beg);
    
    contents.assign((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
    
    return contents;
}

///------------------------------------------------------------------------------------------------

void ShaderLoader::PrependPreprocessorVars(std::string& shaderSource) const
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    std::string platform = "#define WIN32\n";
    std::string version = "#version " + mGlslVersion + " core\n";
#elif __APPLE__
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
    std::string platform = "#define IOS\n";
    std::string version = "#version 300 core\n";
#elif TARGET_OS_IPHONE
    std::string platform = "#define IOS\n";
    std::string version = "#version 300 core\n";
#else //MAC
    std::string platform = "#define MAC\n";
    std::string version = "#version " + mGlslVersion + " core\n";
#endif
#endif
    
    shaderSource = version + platform + shaderSource;
}

///------------------------------------------------------------------------------------------------

void ShaderLoader::ReplaceIncludeDirectives(const std::string& inputFileString, std::string& outFinalShaderSource) const
{
    std::stringstream reconstructedSourceBuilder;
    auto shaderSourceSplitByLine = strutils::StringSplit(inputFileString, '\n');
    for (const auto& line: shaderSourceSplitByLine)
    {
        if (strutils::StringStartsWith(line, "#include"))
        {
            const auto fileSplitByQuotes = strutils::StringSplit(line, '"');
            reconstructedSourceBuilder << '\n';
            outFinalShaderSource += reconstructedSourceBuilder.str();
            ReplaceIncludeDirectives(ReadFileContents(ResourceLoadingService::RES_SHADERS_ROOT +  fileSplitByQuotes[fileSplitByQuotes.size() - 1]), outFinalShaderSource);
            reconstructedSourceBuilder.str("");
        }
        else
        {
            reconstructedSourceBuilder << '\n' << line;
        }
    }
    
    outFinalShaderSource += reconstructedSourceBuilder.str();
}

///------------------------------------------------------------------------------------------------

std::unordered_map<strutils::StringId, GLuint, strutils::StringIdHasher> ShaderLoader::GetUniformNamesToLocationsMap
 (
  const GLuint programId,
  const std::string& shaderName,
  const std::string& vertexShaderFileContents,
  const std::string& fragmentShaderFileContents,
  std::unordered_map<strutils::StringId, int, strutils::StringIdHasher>& uniformArrayElementCounts,
  std::vector<strutils::StringId>& samplerNamesInOrder
  ) const
{
    std::unordered_map<strutils::StringId, GLuint, strutils::StringIdHasher> uniformNamesToLocationsMap;
    
    const auto vertexShaderContentSplitByNewline = strutils::StringSplit(vertexShaderFileContents, '\n');
    for (const auto& vertexShaderLine: vertexShaderContentSplitByNewline)
    {
        if (strutils::StringStartsWith(vertexShaderLine, "uniform"))
        {
            ExtractUniformFromLine(vertexShaderLine, shaderName, programId, uniformNamesToLocationsMap, uniformArrayElementCounts, samplerNamesInOrder);
        }
    }
    
    const auto fragmentShaderContentSplitByNewline = strutils::StringSplit(fragmentShaderFileContents, '\n');
    for (const auto& fragmentShaderLine: fragmentShaderContentSplitByNewline)
    {
        if (strutils::StringStartsWith(fragmentShaderLine, "uniform"))
        {
            ExtractUniformFromLine(fragmentShaderLine, shaderName, programId, uniformNamesToLocationsMap, uniformArrayElementCounts, samplerNamesInOrder);
        }
    }
    
    return uniformNamesToLocationsMap;
}

///------------------------------------------------------------------------------------------------

void ExtractUniformFromLine(const std::string& line, const std::string& shaderName, const GLuint programId, std::unordered_map<strutils::StringId, GLuint, strutils::StringIdHasher>& outUniformNamesToLocations, std::unordered_map<strutils::StringId, int, strutils::StringIdHasher>& outUniformArrayElementCounts, std::vector<strutils::StringId>& outSamplerNamesInOrder)
{
    const auto uniformLineSplitBySpace = strutils::StringSplit(line, ' ');
    
    // Uniform names will always be the third components in the line
    // e.g. uniform bool foo
    auto uniformName = uniformLineSplitBySpace[2].substr(0, uniformLineSplitBySpace[2].size() - 1);
    
    // Check for uniform array
    if (uniformName.at(uniformName.size() - 1) == ']')
    {
        uniformName = uniformName.substr(0, uniformName.size() - 1);
        const auto uniformNameSplitByLeftSquareBracket = strutils::StringSplit(uniformName, '[');
        
        uniformName = uniformNameSplitByLeftSquareBracket[0];
        
        if (strutils::StringIsInt(uniformNameSplitByLeftSquareBracket[1]) == false)
        {
            ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "Error Extracting Uniform", "Could not parse array element count for uniform: " + uniformName);
        }
        
        const auto numberOfElements = std::stoi(uniformNameSplitByLeftSquareBracket[1]);
        
        for (int i = 0; i < numberOfElements; ++i)
        {
            const auto indexedUniformName = uniformName + "[" + std::to_string(i) + "]";
            const auto uniformLocation = GL_NO_CHECK_CALL(glGetUniformLocation(programId, indexedUniformName.c_str()));
            outUniformNamesToLocations[strutils::StringId(indexedUniformName)] = uniformLocation;
            
            if (uniformLocation == -1)
            {
                logging::Log(logging::LogType::WARNING, "At %s, Unused uniform at location -1: %s", shaderName.c_str(), indexedUniformName.c_str());
            }
        }
        
        outUniformArrayElementCounts[strutils::StringId(uniformName)] = numberOfElements;
    }
    // Normal uniform
    else
    {
        auto uniformLocation = GL_NO_CHECK_CALL(glGetUniformLocation(programId, uniformName.c_str()));
        outUniformNamesToLocations[strutils::StringId(uniformName)] = uniformLocation;
        
        if (strutils::StringContains(line, "sampler2D"))
        {
            outSamplerNamesInOrder.push_back(strutils::StringId(uniformName));
        }
        
        if (uniformLocation == -1)
        {
            logging::Log(logging::LogType::WARNING, "At %s, Unused uniform at location -1: %s", shaderName.c_str(), uniformName.c_str());
        }
    }
}

///------------------------------------------------------------------------------------------------

void ShaderLoader::DumpFinalShaderContents(const std::string& vertexShaderContents, const std::string& fragmentShaderContents, const std::string& resourcePath) const
{
    logging::Log(logging::LogType::INFO, "Postprocessed contents of %s", (resourcePath + VERTEX_SHADER_FILE_EXTENSION).c_str());
    {
        const auto vertexShaderContentsSplitByNewline = strutils::StringSplit(vertexShaderContents, '\n');
        for (size_t i = 0; i < vertexShaderContentsSplitByNewline.size(); ++i)
        {
            logging::Log(logging::LogType::INFO, "%d) %s", i + 1, vertexShaderContentsSplitByNewline[i].c_str());
        }
    }
    
    logging::Log(logging::LogType::INFO, "Postprocessed contents of %s", (resourcePath + FRAGMENT_SHADER_FILE_EXTENSION).c_str());
    {
        const auto fragmentShaderContentsSplitByNewline = strutils::StringSplit(fragmentShaderContents, '\n');
        for (size_t i = 0; i < fragmentShaderContentsSplitByNewline.size(); ++i)
        {
            logging::Log(logging::LogType::INFO, "%d) %s", i + 1, fragmentShaderContentsSplitByNewline[i].c_str());
        }
    }
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
