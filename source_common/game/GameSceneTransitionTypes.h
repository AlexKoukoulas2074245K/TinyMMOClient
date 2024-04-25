///------------------------------------------------------------------------------------------------
///  GameSceneTransitionTypes.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 12/12/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef GameSceneTransitionTypes_h
#define GameSceneTransitionTypes_h

///------------------------------------------------------------------------------------------------

enum class SceneChangeType
{
    MODAL_SCENE,
    CONCRETE_SCENE_SYNC_LOADING,
    CONCRETE_SCENE_ASYNC_LOADING
};

enum class PreviousSceneDestructionType
{
    DESTROY_PREVIOUS_SCENE,
    RETAIN_PREVIOUS_SCENE
};

///------------------------------------------------------------------------------------------------

#endif /* GameSceneTransitionTypes_h */
