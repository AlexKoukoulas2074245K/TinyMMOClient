///------------------------------------------------------------------------------------------------
///  RenderingUtils.cpp                                                                                        
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 31/10/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/RenderingUtils.h>
#include <engine/rendering/OpenGL.h>
#include <engine/rendering/IRenderer.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <engine/rendering/stb_image_write.h>
#include <engine/resloading/TextureResource.h>
#include <engine/scene/Scene.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>

///------------------------------------------------------------------------------------------------

static constexpr int NEW_TEXTURE_SIZE = 4096;

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

void ExportToPNG(const std::string& exportFilePath, std::vector<std::shared_ptr<scene::SceneObject>>& sceneObjects)
{
    GLint oldFrameBuffer;
    GLint oldRenderBuffer;
    GL_CALL(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFrameBuffer));
    GL_CALL(glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldRenderBuffer));
    
    GLuint frameBuffer, textureId;
    GL_CALL(glGenFramebuffers(1, &frameBuffer));
    GL_CALL(glGenTextures(1, &textureId));
    
    GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer));
    
    GL_CALL(glBindTexture(GL_TEXTURE_2D, textureId));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(NEW_TEXTURE_SIZE), static_cast<GLsizei>(NEW_TEXTURE_SIZE), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
    GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0));
    
    GLuint depthbuffer;
    GL_CALL(glGenRenderbuffers(1, &depthbuffer));
    GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer));
    GL_CALL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, static_cast<GLsizei>(NEW_TEXTURE_SIZE), static_cast<GLsizei>(NEW_TEXTURE_SIZE)));
    GL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer));
    
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    
    Camera cam;
    cam.SetZoomFactor(30.0f);
    CoreSystemsEngine::GetInstance().GetRenderer().VRenderSceneObjectsToTexture(sceneObjects, cam);
    
    if (!exportFilePath.empty())
    {
        const auto width = static_cast<GLsizei>(NEW_TEXTURE_SIZE);
        const auto height = static_cast<GLsizei>(NEW_TEXTURE_SIZE);
        
        GLvoid* pixels = malloc(sizeof(GLubyte) * width * height * 4);
        GL_CALL(glReadPixels(
           0,
           0,
           width,
           height,
           GL_RGBA,
           GL_UNSIGNED_BYTE,
           pixels
        ));
        
        stbi_write_png(exportFilePath.c_str(), width, height, 4, pixels, width * 4);
        
        logging::Log(logging::LogType::INFO, "Wrote texture to file %s", exportFilePath.c_str());
        
        free(pixels);
    }
    
    GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, oldFrameBuffer));
    GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, oldRenderBuffer));
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    
    GL_CALL(glDeleteFramebuffers(1, &frameBuffer));
    GL_CALL(glDeleteRenderbuffers(1, &depthbuffer));
    
}

///------------------------------------------------------------------------------------------------

int GetDisplayRefreshRate()
{
    SDL_DisplayMode mode;
    int displayIndex = SDL_GetWindowDisplayIndex(&CoreSystemsEngine::GetInstance().GetContextWindow());
    
    // If we can't find the refresh rate, we'll return this:
    constexpr int DEFAULT_REFRESH_RATE = 60;
    if (SDL_GetDesktopDisplayMode(displayIndex, &mode) != 0)
    {
        return DEFAULT_REFRESH_RATE;
    }
    if (mode.refresh_rate == 0)
    {
        return DEFAULT_REFRESH_RATE;
    }
    return mode.refresh_rate;
}

///------------------------------------------------------------------------------------------------

}
