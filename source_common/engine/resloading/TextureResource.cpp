///------------------------------------------------------------------------------------------------
///  TextureResource.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#include <cassert>
#include <engine/rendering/OpenGL.h>
#include <engine/resloading/TextureResource.h>
#include <SDL_pixels.h>

///------------------------------------------------------------------------------------------------

namespace resources
{

///------------------------------------------------------------------------------------------------

TextureResource::~TextureResource()
{
    GL_CALL(glDeleteTextures(1, &mGLTextureId));
}

///------------------------------------------------------------------------------------------------

GLuint TextureResource::GetGLTextureId() const
{
    return mGLTextureId;
}

///------------------------------------------------------------------------------------------------

glm::vec2 TextureResource::GetDimensions() const
{
    return mDimensions;
}

///------------------------------------------------------------------------------------------------

TextureResource::TextureResource
(
    const int width,
    const int height,
    const int mode,
    const int format,
    GLuint glTextureId
)
    : mDimensions(width, height)
    , mMode(mode)
    , mFormat(format)
    , mGLTextureId(glTextureId)
{
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

