///------------------------------------------------------------------------------------------------
///  BoardView.h
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 28/02/2025
///------------------------------------------------------------------------------------------------

#ifndef BoardView_h
#define BoardView_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/scene/Scene.h>
#include <engine/utils/StringUtils.h>
#include <net_common/Board.h>
#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene { struct SceneObject; }

///------------------------------------------------------------------------------------------------

class BoardView final
{
public:
    BoardView(scene::Scene& scene, const slots::Board& boardModel);
    
    void Update(const float dtMillis);
    
    std::vector<std::shared_ptr<scene::SceneObject>> GetSceneObjects();
    const std::string& GetSymbolTexturePath(slots::SymbolType symbol) const;
    
    void DebugFillBoard();

private:
    scene::Scene& mScene;
    const slots::Board& mBoardModel;
    std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
};

///------------------------------------------------------------------------------------------------

#endif /* BoardView_h */
