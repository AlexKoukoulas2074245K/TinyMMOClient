///------------------------------------------------------------------------------------------------
///  ShopSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 14/01/2023
///------------------------------------------------------------------------------------------------

#ifndef ShopSceneLogicManager_h
#define ShopSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>
#include <functional>
#include <variant>
#include <vector>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class GuiObjectManager;
class CardTooltipController;
class ShopSceneLogicManager final: public ISceneLogicManager, public events::IListener
{    
public:
    ShopSceneLogicManager();
    ~ShopSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    void RegisterForEvents();
    void OnWindowResize(const events::WindowResizeEvent& event);
    void OnCardDeletionAnimationFinished(const events::CardDeletionAnimationFinishedEvent&);
    void OnGuiRewardAnimationFinished(const events::GuiRewardAnimationFinishedEvent&);
    void OnProductPurchaseEnded(const events::ProductPurchaseEndedEvent&);
    void CreateDynamicSceneObjects();
    void HandleAlreadyBoughtProducts();
    void FadeInDynamicSceneObjects();
    void CreateProducts();
    void HighlightProduct(const size_t productShelfIndex, const size_t productShelfItemIndex);
    void DehighlightProduct(const size_t productShelfIndex, const size_t productShelfItemIndex);
    void SelectProduct(const size_t productShelfIndex, const size_t productShelfItemIndex);
    void DeselectProduct(const size_t productShelfIndex, const size_t productShelfItemIndex);
    void CreateCardTooltip(const glm::vec3& cardOriginPostion, const std::string& tooltipText);
    void DestroyCardTooltip();
    void OnBuyProductAttempt(const size_t productShelfIndex, const size_t productShelfItemIndex);
    void OnCantBuyProductConfirmationButtonPressed();
    void FindHighlightedProduct(size_t& productShelfIndex, size_t& productShelfItemIndex);
    void ChangeAndAnimateCoinValueReduction(long long coinValueReduction);
    void AnimateBoughtCardToLibrary(const size_t productShelfIndex, const size_t productShelfItemIndex);
    void UpdateProductPriceTags();
    void CheckProductsFinishedFadingIn();
    bool IsDisconnected() const;
    bool IsProductCoins(const size_t productShelfIndex, const size_t productShelfItemIndex);
    
private:
    struct ProductInstance
    {
        ProductInstance(const strutils::StringId& productName)
            : mProductName(productName)
            , mHighlighted(false)
        {
        }
        
        const strutils::StringId mProductName;
        std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
        bool mHighlighted;
    };
    
    enum class SceneState
    {
        CREATING_DYNAMIC_OBJECTS,
        BROWSING_SHOP,
        SELECTED_PRODUCT,
        CANT_BUY_PRODUCT_CONFIRMATION,
        BUYING_CARD_PRODUCT,
        BUYING_NON_CARD_PRODUCT,
        BUYING_PERMA_SHOP_PRODUCT,
        FINISHING_PRODUCT_PURCHASE,
        LEAVING_SHOP
    };
    
private:
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    std::vector<std::vector<std::unique_ptr<ProductInstance>>> mProducts;
    std::unique_ptr<CardTooltipController> mCardTooltipController;
    std::shared_ptr<GuiObjectManager> mGuiManager;
    std::shared_ptr<scene::Scene> mScene;
    SceneState mSceneState;
    glm::vec3 mSelectedProductInitialPosition;
    bool mItemsFinishedFadingIn;
    float mCoinAnimationValue;
    bool mAnimatingCoinValue;
    bool mWaitingForPermaProductAnimation;
    bool mHasSentTutorialTrigger;
};

///------------------------------------------------------------------------------------------------

#endif /* ShopSceneLogicManager_h */
