//
// Created by clx on 25-4-7.
//

#ifndef TOY_RENDERER_UPDATE_SHADER_H
#define TOY_RENDERER_UPDATE_SHADER_H

enum SHADER_TYPE
{
    WIRE,
    SOLID,
    MATERIAL
};

class Shader {
public:
    virtual void cleanup() = 0;
};


#endif //TOY_RENDERER_UPDATE_SHADER_H
