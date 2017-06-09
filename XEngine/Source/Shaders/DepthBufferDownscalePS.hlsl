Texture2D<float> depthTexture : register(t2);

float main(float4 position : SV_Position) : SV_Depth
{
    int2 texPosition = int2(position.xy) * 4;

    float depth = 0.0f;
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        [unroll]
        for (int j = 0; j < 4; j++)
        {
            depth = max(depth, depthTexture[texPosition + int2(j, i)]);
        }
    }

    return depth;
}