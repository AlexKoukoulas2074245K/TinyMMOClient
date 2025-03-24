///------------------------------------------------------------------------------------------------
///  BoardView.h
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 28/02/2025
///------------------------------------------------------------------------------------------------

#ifndef BoardView_h
#define BoardView_h

///------------------------------------------------------------------------------------------------

#include <game/PaylineView.h>
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
    enum class SpinAnimationState
    {
        IDLE,
        PRE_SPIN_LOADING,
        SPINNING,
        POST_SPINNING,
        WAITING_FOR_PAYLINES
    };
    
    struct PendingSymbolData
    {
        enum class PendingSymbolDataState
        {
            LOCKED,
            UNLOCKED,
            FINISHED
        };

        PendingSymbolDataState mState = PendingSymbolDataState::LOCKED;
        std::vector<slots::SymbolType> mSymbols;
    };

public:
    static const std::string& GetSymbolTexturePath(slots::SymbolType symbol);

public:
    BoardView(scene::Scene& scene, const slots::Board& boardModel);
    
    void Update(const float dtMillis);
    
    std::vector<std::shared_ptr<scene::SceneObject>> GetSceneObjects();
    const std::string& GetSpinAnimationStateName() const;
    const std::string& GetPendingSymbolDataStateName(const int reelIndex) const;
    SpinAnimationState GetSpinAnimationState() const;

    void BeginSpin();
    void WaitForPaylines(const slots::BoardStateResolutionData& boardResolutionData);
    void CompleteSpin();
    void ResetBoardSymbols();
    void AnimatePaylineReveal(const slots::PaylineResolutionData& paylineResolutionData, const float revealAnimationDurationSecs, const float hidingAnimationDurationSecs, const float delaySecs = 0.0f);
private:
    void AnimateReelSymbolsToFinalPosition(const int reelIndex);
    void UpdateSceneObjectDuringReelAnimation(std::shared_ptr<scene::SceneObject> sceneObject, const float dtMillis, const int reelIndex);

private:
    scene::Scene& mScene;
    const slots::Board& mBoardModel;
    std::vector<PaylineView> mPaylines;
    std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
    PendingSymbolData mPendingSymbolData[slots::BOARD_COLS];
    SpinAnimationState mSpinAnimationState;
    float mSymbolSpinSpeed;
};

///------------------------------------------------------------------------------------------------

#endif /* BoardView_h */
