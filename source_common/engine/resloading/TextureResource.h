///------------------------------------------------------------------------------------------------
///  TextureResource.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#ifndef TextureResource_h
#define TextureResource_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/IResource.h>
#include <engine/utils/MathUtils.h>
#include <memory>
#include <SDL_stdinc.h>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

using GLuint = unsigned int;

///------------------------------------------------------------------------------------------------

class TextureResource final: public IResource
{
    friend class TextureLoader;
    friend class ResourceLoadingService;
    
public:
    ~TextureResource();
    
    GLuint GetGLTextureId() const;
    glm::vec2 GetDimensions() const;
    
private:
    TextureResource
    (
        const int width, 
        const int height,
        const int mode,
        const int format,
        GLuint glTextureId
    );
    
private:
    glm::vec2 mDimensions;
    int mMode;
    int mFormat;
    GLuint mGLTextureId;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* TextureResource_h */
