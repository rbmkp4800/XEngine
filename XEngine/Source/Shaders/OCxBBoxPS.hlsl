RWStructuredBuffer<uint> geometryInstancesVisibility : register(u0);

struct PSInput
{
	float4 position : SV_Position;
	nointerpolation uint geometryInstanceId : GEOMETRYINSTANCEID;
};

[earlydepthstencil]
void main(PSInput input)
{
	geometryInstancesVisibility[input.geometryInstanceId] = 1;
}