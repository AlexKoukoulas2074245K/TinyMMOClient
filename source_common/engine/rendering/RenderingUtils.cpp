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

struct Pixel
{
    GLubyte r, g, b, a;
};

// Function to convert GLubyte array to vector of pixels
static std::vector<Pixel> ConvertToPixels(GLubyte* pixelData, int width, int height)
{
    std::vector<Pixel> pixels(width * height);
    for (int i = 0; i < width * height; ++i) 
    {
        pixels[i].r = pixelData[i * 4];
        pixels[i].g = pixelData[i * 4 + 1];
        pixels[i].b = pixelData[i * 4 + 2];
        pixels[i].a = pixelData[i * 4 + 3];
    }
    return pixels;
}

// Function to convert vector of pixels to GLubyte array
static void ConvertToGLubyte(std::vector<Pixel>& pixels, GLubyte* pixelData, int width, int height)
{
    for (int i = 0; i < width * height; ++i) 
    {
        pixelData[i * 4] = pixels[i].r;
        pixelData[i * 4 + 1] = pixels[i].g;
        pixelData[i * 4 + 2] = pixels[i].b;
        pixelData[i * 4 + 3] = pixels[i].a;
    }
}

static void ApplyGaussianBlur(std::vector<Pixel>& pixels, int width, int height)
{
    static float SIGMA = 15.5f;
    
    // Define the kernel size based on sigma (standard deviation)
    int kernelSize = static_cast<int>(ceil(3 * SIGMA));

    // Calculate the Gaussian kernel
    std::vector<float> kernel(2 * kernelSize + 1);
    float sigmaSquared = SIGMA * SIGMA;
    float sum = 0.0f;
    for (int i = -kernelSize; i <= kernelSize; ++i) 
    {
        float x = static_cast<float>(i);
        kernel[i + kernelSize] = exp(-(x * x) / (2 * sigmaSquared));
        sum += kernel[i + kernelSize];
    }

    // Normalize the kernel
    for (int i = 0; i < kernel.size(); ++i) 
    {
        kernel[i] /= sum;
    }
    
    logging::Log(logging::LogType::INFO, "Starting blurring...");
    
    // Perform horizontal blur
    std::vector<Pixel> tempPixels(pixels);
    for (int y = 0; y < height; ++y) 
    {
        if (y % (NEW_TEXTURE_SIZE/10) == 0)
        {
            logging::Log(logging::LogType::INFO, "Blurring horizontally %d%% complete...", 1 + static_cast<int>(100.0f * y/static_cast<float>(height)));
        }
        for (int x = 0; x < width; ++x)
        {
            float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
            for (int i = -kernelSize; i <= kernelSize; ++i) 
            {
                int newX = std::min(std::max(x + i, 0), width - 1);
                r += kernel[i + kernelSize] * tempPixels[y * width + newX].r;
                g += kernel[i + kernelSize] * tempPixels[y * width + newX].g;
                b += kernel[i + kernelSize] * tempPixels[y * width + newX].b;
                a += kernel[i + kernelSize] * tempPixels[y * width + newX].a;
            }
            
            pixels[y * width + x].r = static_cast<GLubyte>(r);
            pixels[y * width + x].g = static_cast<GLubyte>(g);
            pixels[y * width + x].b = static_cast<GLubyte>(b);
            pixels[y * width + x].a = static_cast<GLubyte>(a);
        }
    }

    // Perform vertical blur
    tempPixels = pixels;
    for (int y = 0; y < height; ++y) 
    {
        if (y % (NEW_TEXTURE_SIZE/10) == 0)
        {
            logging::Log(logging::LogType::INFO, "Blurring vertically %d%% complete...", 1 + static_cast<int>(100.0f * y/static_cast<float>(height)));
        }
        for (int x = 0; x < width; ++x)
        {
            float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
            for (int i = -kernelSize; i <= kernelSize; ++i) 
            {
                int newY = std::min(std::max(y + i, 0), height - 1);
                r += kernel[i + kernelSize] * tempPixels[newY * width + x].r;
                g += kernel[i + kernelSize] * tempPixels[newY * width + x].g;
                b += kernel[i + kernelSize] * tempPixels[newY * width + x].b;
                a += kernel[i + kernelSize] * tempPixels[newY * width + x].a;
            }
            pixels[y * width + x].r = static_cast<GLubyte>(r);
            pixels[y * width + x].g = static_cast<GLubyte>(g);
            pixels[y * width + x].b = static_cast<GLubyte>(b);
            pixels[y * width + x].a = static_cast<GLubyte>(a);
        }
    }
}

///------------------------------------------------------------------------------------------------

void ExportToPNG(const std::string& exportFilePath, std::vector<std::shared_ptr<scene::SceneObject>>& sceneObjects, const BlurStep blurStep)
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
        
        GLubyte* pixels = static_cast<GLubyte*>(malloc(sizeof(GLubyte) * width * height * 4));
        GL_CALL(glReadPixels(
           0,
           0,
           width,
           height,
           GL_RGBA,
           GL_UNSIGNED_BYTE,
           pixels
        ));
        
        // Calculate the size of one row in bytes
        size_t rowSize = width * 4;

        // Allocate memory for a single row to use for swapping
        GLubyte* rowBuffer = static_cast<GLubyte*>(malloc(rowSize));

        // Iterate through each row and swap with its corresponding row from the bottom
        for (int y = 0; y < height / 2; ++y) {
            int swapY = height - 1 - y; // Calculate the y-coordinate of the row to swap with

            // Copy the current row to the temporary buffer
            memcpy(rowBuffer, pixels + y * rowSize, rowSize);
            
            // Copy the corresponding row from the bottom to the current row
            memcpy(pixels + y * rowSize, pixels + swapY * rowSize, rowSize);
            
            // Copy the contents of the temporary buffer (original current row) to the corresponding row from the bottom
            memcpy(pixels + swapY * rowSize, rowBuffer, rowSize);
        }

        // Free the temporary buffer
        free(rowBuffer);
        
        // Blur image if needed
        if (blurStep == BlurStep::BLUR)
        {
            std::vector<Pixel> pixelVector = ConvertToPixels(pixels, width, height);
            ApplyGaussianBlur(pixelVector, width, height);
            ConvertToGLubyte(pixelVector, pixels, width, height);
        }
        
        
        stbi_write_png(exportFilePath.c_str(), width, height, 4, pixels, width * 4);
        
        logging::Log(logging::LogType::INFO, "Wrote texture to file %s", exportFilePath.c_str());
        
        free(pixels);
        
        GL_CALL(glDeleteTextures(1, &textureId));
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
