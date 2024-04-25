///------------------------------------------------------------------------------------------------
///  MeshResource.cpp
///  Predators
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#include <engine/rendering/OpenGL.h>
#include <engine/resloading/MeshResource.h>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

void MeshResource::ApplyDirectTransformToData(std::function<void(MeshData&)> transform)
{
    if (mMeshData)
    {
        transform(*mMeshData);
        
        GL_CALL(glBindVertexArray(mVertexArrayObject));
        
        // Bind and Buffer VBO
        GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, mMeshData->mVertexBufferId));
        GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, mMeshData->mVertices.size() * sizeof(glm::vec3), &mMeshData->mVertices[0]));
        
        GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, mMeshData->mTexCoordBufferId));
        GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, mMeshData->mTexCoords.size() * sizeof(glm::vec2), &mMeshData->mTexCoords[0]));
        
        GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, mMeshData->mNormalBufferId));
        GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, mMeshData->mNormals.size() * sizeof(glm::vec3), &mMeshData->mNormals[0]));
        
        GL_CALL(glBindVertexArray(0));
    }
}

///------------------------------------------------------------------------------------------------

const GLuint& MeshResource::GetVertexArrayObject() const
{
    return mVertexArrayObject;
}

///------------------------------------------------------------------------------------------------

const GLuint& MeshResource::GetElementCount() const
{
    return mElementCount;
}

///------------------------------------------------------------------------------------------------

const glm::vec3& MeshResource::GetDimensions() const
{
    return mDimensions;
}

///------------------------------------------------------------------------------------------------

const std::vector<glm::vec3>& MeshResource::GetMeshVertices() const
{
    static std::vector<glm::vec3> emptyVertices;
    if (mMeshData)
    {
        return mMeshData->mVertices;
    }
    
    return emptyVertices;
}

///------------------------------------------------------------------------------------------------

const std::vector<glm::vec3>& MeshResource::GetMeshNormals() const
{
    static std::vector<glm::vec3> emptyNormals;
    if (mMeshData)
    {
        return mMeshData->mNormals;
    }
    
    return emptyNormals;
}

///------------------------------------------------------------------------------------------------

MeshResource::MeshResource(const GLuint vertexArrayObject, const GLuint elementCount, const glm::vec3& meshDimensions, std::unique_ptr<MeshData> meshData /* = nullptr */)
    : mVertexArrayObject(vertexArrayObject)
    , mElementCount(elementCount)
    , mDimensions(meshDimensions)
    , mMeshData(std::move(meshData))
{
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
