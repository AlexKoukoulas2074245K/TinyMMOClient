///------------------------------------------------------------------------------------------------
///  ShaderResource.h
///  Predators
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#ifndef ShaderResource_h
#define ShaderResource_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/IResource.h>
#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <string>
#include <unordered_map>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

using GLuint = unsigned int;

///------------------------------------------------------------------------------------------------

class ShaderResource final: public IResource
{
public:
    ShaderResource() = default;
    ShaderResource
    (
        const std::unordered_map<strutils::StringId, GLuint, strutils::StringIdHasher>& uniformNamesToLocations,
        const std::unordered_map<strutils::StringId, int, strutils::StringIdHasher>& uniformArrayElementCounts,
        const std::vector<strutils::StringId>& uniformSamplerNamesInOrder,
        const GLuint programId
    );
    ShaderResource& operator = (const ShaderResource&);
    ShaderResource(const ShaderResource&);
    
public:
    bool SetMatrix4fv(const strutils::StringId& uniformName, const glm::mat4& matrix, const GLuint count = 1, const bool transpose = false) const;
    bool SetMatrix4Array(const strutils::StringId& uniformName, const std::vector<glm::mat4>& values) const;
    bool SetFloatVec4Array(const strutils::StringId& uniformName, const std::vector<glm::vec4>& values) const;
    bool SetFloatVec3Array(const strutils::StringId& uniformName, const std::vector<glm::vec3>& values) const;
    bool SetFloatVec4(const strutils::StringId& uniformName, const glm::vec4& vec) const;
    bool SetFloatVec3(const strutils::StringId& uniformName, const glm::vec3& vec) const;
    bool SetFloat(const strutils::StringId& uniformName, const float value) const;
    bool SetFloatArray(const strutils::StringId& uniformName, const std::vector<float>& values) const;
    bool SetInt(const strutils::StringId& uniformName, const int value) const;
    bool SetBool(const strutils::StringId& uniformName, const bool value) const;

    GLuint GetProgramId() const;    

    const std::unordered_map<strutils::StringId, GLuint, strutils::StringIdHasher>& GetUniformNamesToLocations() const;
    const std::vector<strutils::StringId>& GetUniformSamplerNames() const;
    
    void CopyConstruction(const ShaderResource&);
    
private:
    std::unordered_map<strutils::StringId, GLuint, strutils::StringIdHasher> mShaderUniformNamesToLocations;
    std::vector<strutils::StringId> mUniformSamplerNamesInOrder;
    std::unordered_map<strutils::StringId, int, strutils::StringIdHasher> mUniformArrayElementCounts;
    GLuint mProgramId;    
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* ShaderResource_h */
