// ==========================================
// INPUTS
// ==========================================
// We reuse the same layout as your standard meshes so we can use the "Cube" mesh
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

[[vk::binding(0, 0)]] // Binding 0, Set 0
ConstantBuffer<UniformBuffer> ubo;

// ==========================================
// SET 1: SKYBOX CUBEMAP
// ==========================================
[[vk::binding(0, 1)]] // Binding 0, Set 1
SamplerCube skyboxTexture;

// ==========================================
// SHADER OUTPUTS
// ==========================================
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
    
    output.texCoord = input.inPos;
    
    // 1. Extract ONLY the 3x3 rotation matrix (drops the translation entirely)
    float3x3 viewRotOnly = (float3x3)ubo.view;
    
    // 2. Scale the cube up slightly so it doesn't get clipped by the camera's near-plane
    float3 scaledPos = input.inPos * 10.0;
    
    // 3. Multiply by rotation, then extend back to float4 for the projection
    float3 viewPos = mul(viewRotOnly, scaledPos);
    float4 clipPos = mul(ubo.proj, float4(viewPos, 1.0));
    
    // 4. Force depth to 1.0 (Maximum depth)
    output.pos = clipPos.xyww; 
    
    return output;
}

// ==========================================
// FRAGMENT SHADER
// ==========================================
[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_Target {
    // Sample the cubemap using the 3D direction vector
    return skyboxTexture.Sample(vertIn.texCoord);
}