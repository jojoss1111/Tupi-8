#version 450

layout(push_constant) uniform PushConstants {
    mat4 proj;
    uint textureIndex;
    uint _pad0;
    uint _pad1;
    uint _pad2;
} pc;

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragUV;

void main() {
    fragColor = inColor;
    fragUV = inUV;
    gl_Position = pc.proj * vec4(inPos, 0.0, 1.0);
}
