///------------------------------------------------------------------------------------------------
///  LocalPlayerInputController.h                                                                                          
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 27/12/2025                                                       
///------------------------------------------------------------------------------------------------

#ifndef LocalPlayerInputController_h
#define LocalPlayerInputController_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>

///------------------------------------------------------------------------------------------------

class LocalPlayerInputController
{
public:
    static glm::vec2 GetMovementDirection();
    
private:
    LocalPlayerInputController(){};
};

///------------------------------------------------------------------------------------------------

#endif /* LocalPlayerInputController_h */
