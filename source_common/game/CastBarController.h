///------------------------------------------------------------------------------------------------
///  CastBarController.h
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 03/02/2026
///------------------------------------------------------------------------------------------------

#ifndef CastBarController_h
#define CastBarController_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <memory>

///------------------------------------------------------------------------------------------------

namespace scene { class Scene; }
class FillableBar;
class CastBarController
{
public:
    CastBarController(std::shared_ptr<scene::Scene> scene);
    ~CastBarController();
    
    void ShowCastBar(const float revealSecs);
    void HideCastBar(const float hideSecs);
    void BeginCast(const float duration, std::function<void()> onCompleteCallback);
    void CancelCast();
    bool IsCastBarFilling() const;

private:
    std::shared_ptr<scene::Scene> mScene;
    std::unique_ptr<FillableBar> mCastBar;
    std::function<void()> mOnCompleteCallback;
};

///------------------------------------------------------------------------------------------------

#endif /* CastBarController_h */
