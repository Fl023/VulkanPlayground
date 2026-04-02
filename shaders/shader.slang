// ==========================================
// INPUTS
// ==========================================
struct VSInput {
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
};

// ==========================================
// SET 0: CAMERA DATA 
// ==========================================
struct UniformBuffer {
    float4x4 view;
    float4x4 proj;
};
[[vk::binding(0, 0)]] 
ConstantBuffer<UniformBuffer> ubo;

// ==========================================
// SET 1: GLOBAL BINDLESS ARRAYS
// ==========================================
[[vk::binding(0, 1)]] Sampler2D bindlessTextures[1000]; // Binding 0
[[vk::binding(1, 1)]] SamplerCube bindlessCubemaps[1000]; // Binding 1 (Declared to match C++ layout!)

// ==========================================
// PUSH CONSTANTS (80 Bytes)
// ==========================================
struct PushConstants {
    float4x4 model;
    uint textureIndex;
};
[[vk::push_constant]] 
PushConstants push;

struct VSOutput {
    float4 pos : SV_Position;
    float3 fragColor;
    float2 fragTexCoord;
};

// ==========================================
// VERTEX SHADER
// ==========================================
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = mul(ubo.proj, mul(ubo.view, mul(push.model, float4(input.inPos, 1.0))));
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

// ==========================================
// FRAGMENT SHADER
// ==========================================
[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
   // Use the 2D Texture Array!
   float3 texColor = bindlessTextures[NonUniformResourceIndex(push.textureIndex)].Sample(vertIn.fragTexCoord).rgb;
   return float4(vertIn.fragColor * texColor, 1.0);
}