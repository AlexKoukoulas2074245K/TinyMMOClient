///------------------------------------------------------------------------------------------------
///  OpenGL.h
///  Predators
///
///  Created by Alex Koukoulas on 19/09/2023.
///-----------------------------------------------------------------------------------------------

#ifndef OpenGL_h
#define OpenGL_h

///-----------------------------------------------------------------------------------------------

#include <assert.h>

#ifdef __APPLE__
    #if __has_include(<OpenGLES/ES3/gl.h>)
        #define GLES_SILENCE_DEPRECATION
        #include <OpenGLES/ES3/gl.h>
    #else
        #define GL_SILENCE_DEPRECATION
        #include <OpenGL/gl3.h>
    #endif
    #define GL_CALL(func) do { func; auto err = glGetError(); if (err != GL_NO_ERROR) { printf("GLError: %d\n", err); assert(false); } } while (0)
    #define GL_NO_CHECK_CALL(func) func
#else
    #define GLEW_STATIC
    #include <engine/rendering/glew/glew.h>
    #define GL_CALL(func) do { func; auto err = glGetError(); if (err != GL_NO_ERROR) { printf("GLError: %d\n", err); assert(false); } } while (0)
    #define GL_NO_CHECK_CALL(func) func
#endif


///-----------------------------------------------------------------------------------------------

#endif /* OpenGL_h */
