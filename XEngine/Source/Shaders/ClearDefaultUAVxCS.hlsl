// TODO: solve. With RWBuffer instead of RWStructuredBuffer:
//	Root Signature doesn't match Compute Shader: A Shader is declaring a typed UAV using
//		a register mapped to a root descriptor UAV (RegisterSpace=0, ShaderRegister=0).
//		SRV or UAV root descriptors can only be Raw or Structured buffers.

RWStructuredBuffer<uint> defaultUAV : register(u0);

[numthreads(1024, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	defaultUAV[id.x] = 0;
}