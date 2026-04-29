#version 450

layout(push_constant) uniform PushConstants {
    mat4 proj;
    uint textureIndex;
    uint _pad0;
    uint _pad1;
    uint _pad2;
} pc;

layout(set = 0, binding = 0) uniform sampler2D texSampler[4096];

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texel = texture(texSampler[pc.textureIndex], fragUV);
    outColor = texel * fragColor;
}
