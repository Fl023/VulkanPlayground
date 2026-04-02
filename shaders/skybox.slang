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
[[vk::binding(0, 1)]] Sampler2D bindlessTextures[1000];   // Binding 0 (Declared to match C++ layout!)
[[vk::binding(1, 1)]] SamplerCube bindlessCubemaps[1000]; // Binding 1 

// ==========================================
// PUSH CONSTANTS (80 Bytes)
// ==========================================
struct PushConstants {
    float4x4 model; // Unused by skybox, but required to match C++!
    uint textureIndex;
};
[[vk::push_constant]] 
PushConstants push;

struct VSOutput {
    float4 pos : SV_Position;
    float3 texCoord;
};

// ==========================================
// VERTEX SHADER
// ==========================================
[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    
    // The 3D position IS the texture coordinate for a cubemap
    output.texCoord = input.inPos;
    
    // 1. Extract ONLY the 3x3 rotation matrix (drops the translation entirely)
    float3x3 viewRotOnly = (float3x3)ubo.view;
    
    // 2. Scale the cube up slightly
    float3 scaledPos = input.inPos * 10.0;
    
    // 3. Multiply by rotation, then extend back to float4 for the projection
    float3 viewPos = mul(viewRotOnly, scaledPos);
    float4 clipPos = mul(ubo.proj, float4(viewPos, 1.0));
    
    // 4. Force depth to 1.0 (Maximum depth) so it stays in the background
    output.pos = clipPos.xyww; 
    
    return output;
}

// ==========================================
// FRAGMENT SHADER
// ==========================================
[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
   // Use the Cubemap Array!
   return bindlessCubemaps[NonUniformResourceIndex(push.textureIndex)].Sample(vertIn.texCoord);
}