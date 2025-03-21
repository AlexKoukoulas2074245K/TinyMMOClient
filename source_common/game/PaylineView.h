///------------------------------------------------------------------------------------------------
///  PaylineView.h
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 20/03/2025
///------------------------------------------------------------------------------------------------

#ifndef PaylineView_h
#define PaylineView_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/scene/Scene.h>
#include <engine/utils/StringUtils.h>
#include <net_common/Paylines.h>
#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene { struct SceneObject; }

///------------------------------------------------------------------------------------------------

class PaylineView final
{
public:
    static const std::string& GetPaylineName(const slots::PaylineType payline);
    
public:
    PaylineView(scene::Scene& scene, const slots::PaylineType payline);

    void AnimatePaylineReveal(const float revealAnimationDurationSecs, const float hidingAnimationDurationSecs, const float delaySecs = 0.0f);
    std::shared_ptr<scene::SceneObject> GetSceneObject();

private:
    void ResetAnimationVars();

private:
    scene::Scene& mScene;
    const slots::PaylineType mPayline;
    std::shared_ptr<scene::SceneObject> mSceneObject;
};

///------------------------------------------------------------------------------------------------

#endif /* PaylineView_h */
