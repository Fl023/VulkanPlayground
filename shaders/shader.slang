struct VSInput {
    float2 inPos;
    float3 inColor;
    float2 inTexCoord;
};

// ==========================================
// SET 0: KAMERA DATEN (Wird 1x pro Frame gebunden)
// ==========================================
struct UniformBuffer {
    float4x4 view;
    float4x4 proj;
};

[[vk::binding(0, 0)]] // Binding 0, Set 0
ConstantBuffer<UniformBuffer> ubo;

// ==========================================
// PUSH CONSTANTS: OBJEKT DATEN (Pro Objekt)
// ==========================================
struct PushConstants {
    float4x4 model;
};

[[vk::push_constant]] 
PushConstants push;

struct VSOutput
{
    float4 pos : SV_Position;
    float3 fragColor;
    float2 fragTexCoord;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    // (Hinweis: Falls deine GLM-Matrizen in C++ column-major sind,
    // sorgt Slang in der Regel automatisch für die richtige Übersetzung)
    output.pos = mul(ubo.proj, mul(ubo.view, mul(push.model, float4(input.inPos, 0.0, 1.0))));
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

// ==========================================
// SET 1: MATERIAL DATEN (Pro Material)
// ==========================================
[[vk::binding(0, 1)]] // Binding 0, Set 1
Sampler2D texture;

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
   // Wir sampeln die Textur und multiplizieren sie mit der Vertex-Farbe
   return float4(vertIn.fragColor * texture.Sample(vertIn.fragTexCoord).rgb, 1.0);
}