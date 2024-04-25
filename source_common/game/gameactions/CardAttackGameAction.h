///------------------------------------------------------------------------------------------------
///  CardAttackGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 27/10/2023
///------------------------------------------------------------------------------------------------

#ifndef CardAttackGameAction_h
#define CardAttackGameAction_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------


class CardAttackGameAction final: public BaseGameAction
{
public:
    static const std::string CARD_INDEX_PARAM;
    static const std::string PLAYER_INDEX_PARAM;
    
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
    
private:
    int mPendingAnimations;
    int mPendingDamage;
    int mAmountOfArmorDamaged;
    int mAmountOfHealthDamaged;
    bool mLifestealHealedAtLeast1Hp;
    glm::vec3 mOriginalCardPosition;
    glm::vec3 mOriginalCardScale;
};

///------------------------------------------------------------------------------------------------

#endif /* CardAttackGameAction_h */
