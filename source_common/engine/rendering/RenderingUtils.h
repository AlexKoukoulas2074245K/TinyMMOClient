///------------------------------------------------------------------------------------------------
///  RenderingUtils.h                                                                                          
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 31/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef RenderingUtils_h
#define RenderingUtils_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/MathUtils.h>
#include <memory>
#include <vector>

namespace scene { struct SceneObject; }
namespace scene { class Scene; }

///------------------------------------------------------------------------------------------------

using GLuint = unsigned int;
struct SDL_Surface;

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

enum class BlurStep
{
    DONT_BLUR, BLUR
};


///------------------------------------------------------------------------------------------------

void CreateGLTextureFromSurface(SDL_Surface* surface, GLuint& glTextureId, int& mode);

///------------------------------------------------------------------------------------------------

void ExportToPNG(const std::string& exportFilePath, std::vector<std::shared_ptr<scene::SceneObject>>& sceneObjects, const BlurStep blurStep);

///------------------------------------------------------------------------------------------------

int GetDisplayRefreshRate();

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* RenderingUtils_h */
