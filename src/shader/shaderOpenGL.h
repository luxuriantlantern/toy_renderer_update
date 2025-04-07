//
// Created by clx on 25-4-7.
//

#ifndef TOY_RENDERER_UPDATE_SHADEROPENGL_H
#define TOY_RENDERER_UPDATE_SHADEROPENGL_H

#include "shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class shaderOpenGL : public Shader {
public:
    void cleanup() {
        if(!mProgram) glDeleteProgram(mProgram);
        mProgram = 0;
    }
private:
    unsigned int mProgram;
};


#endif //TOY_RENDERER_UPDATE_SHADEROPENGL_H
