#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 vColor;

void main() {
    outColor = vec4(vColor, 1.0);
}