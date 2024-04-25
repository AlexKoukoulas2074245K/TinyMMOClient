///------------------------------------------------------------------------------------------------
///  MeshResource.h
///  Predators
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#ifndef MeshResource_h
#define MeshResource_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/IResource.h>
#include <engine/utils/MathUtils.h>
#include <memory>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

using GLuint = unsigned int;

class MeshResource final: public IResource
{
    friend class OBJMeshLoader;
    
public:
    struct MeshData
    {
        MeshData(const GLuint vertexBufferId, const GLuint texCoordBufferId, const GLuint normalBufferId, const std::vector<glm::vec3>& orderedVertices, const std::vector<glm::vec2>& orderedTexCoords, const std::vector<glm::vec3>& orderedNormals)
            : mVertexBufferId(vertexBufferId)
            , mTexCoordBufferId(texCoordBufferId)
            , mNormalBufferId(normalBufferId)
            , mVertices(orderedVertices)
            , mTexCoords(orderedTexCoords)
            , mNormals(orderedNormals)
        {
        }
        
        const GLuint mVertexBufferId;
        const GLuint mTexCoordBufferId;
        const GLuint mNormalBufferId;
        std::vector<glm::vec3> mVertices;
        std::vector<glm::vec2> mTexCoords;
        std::vector<glm::vec3> mNormals;
    };
    
public:
    void ApplyDirectTransformToData(std::function<void(MeshData&)> transform);
    
    const GLuint& GetVertexArrayObject() const;
    const GLuint& GetElementCount() const;
    const glm::vec3& GetDimensions() const;
    const std::vector<glm::vec3>& GetMeshVertices() const;
    const std::vector<glm::vec3>& GetMeshNormals() const;
    
private:
    MeshResource(const GLuint vertexArrayObject, const GLuint elementCount, const glm::vec3& meshDimensions, std::unique_ptr<MeshData> meshData = nullptr);
    
private:
    const GLuint mVertexArrayObject;
    const GLuint mElementCount;
    const glm::vec3 mDimensions;
    std::unique_ptr<MeshData> mMeshData;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* MeshResource_h */
