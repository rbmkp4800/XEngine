struct CameraConstants
{
    float4x4 viewProjection;
    float4x4 view;
};
 
struct InstanceConstants
{
    //uint baseTransformIndex;
    uint textureId;
};
 
ConstantBuffer<CameraConstants> cameraConstants : register(b0);
ConstantBuffer<InstanceConstants> instanceConstants : register(b1);
//StructuredBuffer<float4x3> transformsBuffer : register(t0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float4x3 transform : TRANSFORM;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    uint textureId : TEXTUREID;
    float2 texCoord : TEXCOORD;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	float3 worldPosition = mul(float4(input.position, 1.0f), input.transform);
	output.position = mul(cameraConstants.viewProjection, float4(worldPosition, 1.0f));
	float3 normal = mul(input.normal, (float3x3)input.transform);
    output.normal = normalize(mul((float3x3) cameraConstants.view, normal));
    output.textureId = instanceConstants.textureId;
	output.texCoord = input.texCoord;
	return output;
}