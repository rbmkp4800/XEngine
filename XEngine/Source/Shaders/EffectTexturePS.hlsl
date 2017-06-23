Texture2D<float4> diffuseTextures[16] : register(t0, space1);
SamplerState defaultSampler : register(s0);

struct PSInput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    uint textureId : TEXTUREID;
    float2 texCoord : TEXCOORD;
};

struct PSOutput
{
	float4 diffuse : SV_Target0;
	float2 normal : SV_Target1;
};

PSOutput main(PSInput input)
{
	PSOutput output;
    output.diffuse = input.textureId != uint(-1) ?
        diffuseTextures[input.textureId].Sample(defaultSampler, input.texCoord) :
        float4(0.0f, 1.0f, 0.0f, 1.0f);
	output.normal = normalize(input.normal).xy;
	return output;
}