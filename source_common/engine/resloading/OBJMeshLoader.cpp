///------------------------------------------------------------------------------------------------
///  OBJMeshLoader.cpp
///  Predators
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

// Disable CRT_SECURE warnings for fopen, fscanf etc..
#ifdef _WIN32
#pragma warning(disable: 4996)
#endif

#include <cassert>
#include <cstdio>
#include <engine/rendering/OpenGL.h>
#include <engine/resloading/OBJMeshLoader.h>
#include <engine/resloading/MeshResource.h>
#include <engine/utils/FileUtils.h>
#include <engine/utils/MathUtils.h>
#include <engine/utils/OSMessageBox.h>
#include <engine/utils/StringUtils.h>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

void OBJMeshLoader::VInitialize()
{
}

///------------------------------------------------------------------------------------------------

bool OBJMeshLoader::VCanLoadAsync() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<IResource> OBJMeshLoader::VCreateAndLoadResource(const std::string& path) const
{
    auto trimmedPath = path;
    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    
    std::vector<glm::vec3> tempVertices;
    std::vector<glm::vec2> tempUvs;
    std::vector<glm::vec3> tempNormals;
    
    std::vector<glm::vec3> finalVertices;
    std::vector<glm::vec2> finalUvs;
    std::vector<glm::vec3> finalNormals;
    
    std::vector<unsigned short> finalIndices;
    
    float minX = 100.0f, maxX = -100.0f, minY = 100.0f, maxY = -100.0f, minZ = 100.0f, maxZ = -100.0f;
    
    FILE * file = std::fopen(trimmedPath.c_str(), "r");
    
    if (!file)
    {
        ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "File could not be found", path.c_str());
        return nullptr;
    }
    
    const auto fileNameWithoutExtension = fileutils::GetFileNameWithoutExtension(path);
    bool dynamicMesh = strutils::StringContains(fileNameWithoutExtension, "dynamic");
    
    while(1)
    {
        char lineHeader[128];
        // read the first word of the line
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF)
            break; // EOF = End Of File. Quit the loop.
        
        if (strcmp(lineHeader, "v") == 0)
        {
            glm::vec3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
            //vertex.z = -vertex.z;
            tempVertices.push_back(vertex);
            
            if (vertex.x < minX) minX = vertex.x;
            if (vertex.x > maxX) maxX = vertex.x;
            if (vertex.y < minY) minY = vertex.y;
            if (vertex.y > maxY) maxY = vertex.y;
            if (vertex.z < minZ) minZ = vertex.z;
            if (vertex.z > maxZ) maxZ = vertex.z;
        }
        else if (strcmp(lineHeader, "vt") == 0)
        {
            glm::vec2 uv;
            fscanf(file, "%f %f\n", &uv.x, &uv.y );
            tempUvs.push_back(uv);
        }
        else if (strcmp(lineHeader, "vn") == 0)
        {
            glm::vec3 normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
            tempNormals.push_back(normal);
        }
        else if (strcmp(lineHeader, "f") == 0)
        {
            std::string vertex1, vertex2, vertex3;
            unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
            int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
            if (matches != 9)
            {
                assert(false && "File can't be read by this simple parser");
                fclose(file);
            }
            
            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);
            
            uvIndices.push_back(uvIndex[0]);
            uvIndices.push_back(uvIndex[1]);
            uvIndices.push_back(uvIndex[2]);
            
            normalIndices.push_back(normalIndex[0]);
            normalIndices.push_back(normalIndex[1]);
            normalIndices.push_back(normalIndex[2]);
        }
        else
        {
            // Probably a comment, eat up the rest of the line
            char stupidBuffer[1000];
            fgets(stupidBuffer, 1000, file);
        }
    }
    
    // For each vertex of each triangle
    for(unsigned int i=0; i<vertexIndices.size(); i++)
    {
        // Get the indices of its attributes
        unsigned int vertexIndex = vertexIndices[i];
        unsigned int uvIndex = uvIndices[i];
        unsigned int normalIndex = normalIndices[i];
        
        // Get the attributes thanks to the index
        glm::vec3 vertex = tempVertices[ vertexIndex-1 ];
        glm::vec2 uv = tempUvs[ uvIndex-1 ];
        glm::vec3 normal = tempNormals[ normalIndex-1 ];
        
        // Put the attributes in buffers
        finalVertices.push_back(vertex);
        finalUvs.push_back(uv);
        finalNormals.push_back(normal);
        finalIndices.push_back(static_cast<unsigned short>(i));
    }
    
    std::fclose(file);
    
    GLuint vertexArrayObject;
    GLuint vertexBufferObject;
    GLuint uvCoordsBufferObject;
    GLuint normalsBufferObject;
    GLuint indexBufferObject;
    
    GLenum usage = dynamicMesh ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    
    // Create Buffers
    GL_CALL(glGenVertexArrays(1, &vertexArrayObject));
    GL_CALL(glGenBuffers(1, &vertexBufferObject));
    GL_CALL(glGenBuffers(1, &uvCoordsBufferObject));
    GL_CALL(glGenBuffers(1, &normalsBufferObject));
    GL_CALL(glGenBuffers(1, &indexBufferObject));
    
    // Prepare VAO to record buffer state
    GL_CALL(glBindVertexArray(vertexArrayObject));
    
    // Bind and Buffer VBO
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, finalVertices.size() * sizeof(glm::vec3), &finalVertices[0], usage));
    
    // 1st attribute buffer : vertices
    GL_CALL(glEnableVertexAttribArray(0));
    GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));
    
    // Bind and buffer TBO
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, uvCoordsBufferObject));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, finalUvs.size() * sizeof(glm::vec2), &finalUvs[0], usage));
    
    // 2nd attribute buffer: tex coords
    GL_CALL(glEnableVertexAttribArray(1));
    GL_CALL(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0));
    
    // Bind and Buffer NBO
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, normalsBufferObject));
    GL_CALL(glBufferData(GL_ARRAY_BUFFER, finalNormals.size() * sizeof(glm::vec3), &finalNormals[0], usage));
    
    // 3rd attribute buffer: normals
    GL_CALL(glEnableVertexAttribArray(2));
    GL_CALL(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));
    
    // Bind and Buffer IBO
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObject));
    GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, finalIndices.size() * sizeof(unsigned short), &finalIndices[0], usage));
    
    GL_CALL(glBindVertexArray(0));
    
    // Decide whether to forward all mesh data as well
    std::unique_ptr<MeshResource::MeshData> meshData = nullptr;
    
    if (dynamicMesh)
    {
        meshData = std::make_unique<MeshResource::MeshData>(vertexBufferObject, uvCoordsBufferObject, normalsBufferObject, finalVertices, finalUvs, finalNormals);
    }
    
    // Calculate dimensions
    glm::vec3 meshDimensions(math::Abs(minX - maxX), math::Abs(minY - maxY), math::Abs(minZ - maxZ));
    return std::shared_ptr<MeshResource>(new MeshResource(vertexArrayObject, (GLuint)finalIndices.size(), meshDimensions, std::move(meshData)));
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
