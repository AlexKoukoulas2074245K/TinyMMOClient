///------------------------------------------------------------------------------------------------
///  ShaderResource.cpp
///  Predators
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#include <engine/resloading/ShaderResource.h>
#include <engine/rendering/OpenGL.h>
#include <engine/utils/Logging.h>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

ShaderResource::ShaderResource
(
    const std::unordered_map<strutils::StringId, GLuint, strutils::StringIdHasher>& uniformNamesToLocations,
    const std::unordered_map<strutils::StringId, int, strutils::StringIdHasher>&   uniformArrayElementCounts,
    const std::vector<strutils::StringId>& uniformSamplerNamesInOrder,
    const GLuint programId
)
    : mShaderUniformNamesToLocations(uniformNamesToLocations)
    , mUniformSamplerNamesInOrder(uniformSamplerNamesInOrder)
    , mUniformArrayElementCounts(uniformArrayElementCounts)
    , mProgramId(programId)
{
    
}

///------------------------------------------------------------------------------------------------

ShaderResource& ShaderResource::operator = (const ShaderResource& rhs)
{
    CopyConstruction(rhs);
    return *this;
}

///------------------------------------------------------------------------------------------------

ShaderResource::ShaderResource(const ShaderResource& rhs)
{
    CopyConstruction(rhs);
}

///------------------------------------------------------------------------------------------------

bool ShaderResource::SetMatrix4fv
(
    const strutils::StringId& uniformName,
    const glm::mat4& matrix, 
    const GLuint count /* 1 */,
    const bool transpose /* false */
) const
{
    if (mShaderUniformNamesToLocations.count(uniformName) > 0)
    {
        GL_CALL(glUniformMatrix4fv(mShaderUniformNamesToLocations.at(uniformName), count, transpose, (GLfloat*)&matrix));
        return true;
    }    
    return false;
}

///------------------------------------------------------------------------------------------------

bool ShaderResource::SetMatrix4Array(const strutils::StringId& uniformName, const std::vector<glm::mat4>& values) const
{
    if (mUniformArrayElementCounts.count(uniformName) && static_cast<int>(values.size()) > mUniformArrayElementCounts.at(uniformName))
    {
        logging::Log(logging::LogType::WARNING, "Uniform array size exceeded");
    }
    
    for (auto i = 0U; i < values.size(); ++i)
    {
        auto setUniformResult = SetMatrix4fv(strutils::StringId(uniformName.GetString() + "[" + std::to_string(i) + "]"), values[i]);
        if (!setUniformResult)
        {
            return false;
        }
    }
    
    return true;
}

///------------------------------------------------------------------------------------------------

bool ShaderResource::SetFloatVec4Array(const strutils::StringId& uniformName, const std::vector<glm::vec4>& values) const
{
    if (mUniformArrayElementCounts.count(uniformName) && static_cast<int>(values.size()) > mUniformArrayElementCounts.at(uniformName))
    {
        logging::Log(logging::LogType::WARNING, "Uniform array size exceeded");
    }
    
    for (auto i = 0U; i < values.size(); ++i)
    {
        auto setUniformResult = SetFloatVec4(strutils::StringId(uniformName.GetString() + "[" + std::to_string(i) + "]"), values[i]);
        if (!setUniformResult)
        {
            return false;
        }
    }
    
    return true;
}

///------------------------------------------------------------------------------------------------

bool ShaderResource::SetFloatVec3Array(const strutils::StringId& uniformName, const std::vector<glm::vec3>& values) const
{
    if (mUniformArrayElementCounts.count(uniformName) && static_cast<int>(values.size()) > mUniformArrayElementCounts.at(uniformName))
    {
        logging::Log(logging::LogType::WARNING, "Uniform array size exceeded");
    }
    
    for (auto i = 0U; i < values.size(); ++i)
    {
        auto setUniformResult = SetFloatVec3(strutils::StringId(uniformName.GetString() + "[" + std::to_string(i) + "]"), values[i]);
        if (!setUniformResult)
        {
            return false;
        }
    }
    
    return true;
}

///------------------------------------------------------------------------------------------------

bool ShaderResource::SetFloatVec4(const strutils::StringId& uniformName, const glm::vec4& vec) const
{
    if (mShaderUniformNamesToLocations.count(uniformName) > 0)
    {
        GL_CALL(glUniform4f(mShaderUniformNamesToLocations.at(uniformName), vec.x, vec.y, vec.z, vec.w));
        return true;
    }
    return false;
}

///------------------------------------------------------------------------------------------------

bool ShaderResource::SetFloatVec3(const strutils::StringId& uniformName, const glm::vec3& vec) const
{
    if (mShaderUniformNamesToLocations.count(uniformName) > 0)
    {
        GL_CALL(glUniform3f(mShaderUniformNamesToLocations.at(uniformName), vec.x, vec.y, vec.z));
        return true;
    }
    return false;
}

///------------------------------------------------------------------------------------------------

bool ShaderResource::SetFloat(const strutils::StringId& uniformName, const float value) const
{
    if (mShaderUniformNamesToLocations.count(uniformName) > 0)
    {
        GL_CALL(glUniform1f(mShaderUniformNamesToLocations.at(uniformName), value));
        return true;
    }
    return false;
}

///------------------------------------------------------------------------------------------------

bool ShaderResource::SetFloatArray(const strutils::StringId& uniformName, const std::vector<float>& values) const
{
    if (mUniformArrayElementCounts.count(uniformName) && static_cast<int>(values.size()) > mUniformArrayElementCounts.at(uniformName))
    {
        logging::Log(logging::LogType::WARNING, "Uniform array size exceeded");
    }
    
    for (auto i = 0U; i < values.size(); ++i)
    {
        auto setUniformResult = SetFloat(strutils::StringId(uniformName.GetString() + "[" + std::to_string(i) + "]"), values[i]);
        if (!setUniformResult)
        {
            return false;
        }
    }
    
    return true;
}

///------------------------------------------------------------------------------------------------

bool ShaderResource::SetInt(const strutils::StringId& uniformName, const int value) const
{
    if (mShaderUniformNamesToLocations.count(uniformName) > 0)
    {
        GL_CALL(glUniform1i(mShaderUniformNamesToLocations.at(uniformName), value));
        return true;
    }
    return false;
}

///------------------------------------------------------------------------------------------------

bool ShaderResource::SetBool(const strutils::StringId& uniformName, const bool value) const
{
    if (mShaderUniformNamesToLocations.count(uniformName) > 0)
    {
        GL_CALL(glUniform1i(mShaderUniformNamesToLocations.at(uniformName), value ? 1 : 0));
        return true;
    }
    return false;
}

///------------------------------------------------------------------------------------------------

GLuint ShaderResource::GetProgramId() const
{
    return mProgramId;
}

///------------------------------------------------------------------------------------------------

const std::unordered_map<strutils::StringId, GLuint, strutils::StringIdHasher>& ShaderResource::GetUniformNamesToLocations() const
{
    return mShaderUniformNamesToLocations;
}

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& ShaderResource::GetUniformSamplerNames() const
{
    return mUniformSamplerNamesInOrder;
}

///------------------------------------------------------------------------------------------------

void ShaderResource::CopyConstruction(const ShaderResource& rhs)
{
    mProgramId = rhs.GetProgramId();
    mShaderUniformNamesToLocations = rhs.GetUniformNamesToLocations();
    mUniformSamplerNamesInOrder = rhs.GetUniformSamplerNames();
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
