#include <Windows.h>

#include "XEngine.Render.Internal.Shaders.h"

#include "..\Intermediate\Shaders\ScreenQuadVS.cso.h"
#include "..\Intermediate\Shaders\DepthBufferDownscalePS.cso.h"
#include "..\Intermediate\Shaders\LightingPassPS.cso.h"
#include "..\Intermediate\Shaders\BloomFilterAndDownscaleX4PS.cso.h"
#include "..\Intermediate\Shaders\BloomBlurPS.cso.h"
#include "..\Intermediate\Shaders\BloomDownscaleX2PS.cso.h"
#include "..\Intermediate\Shaders\ToneMappingPS.cso.h"

#include "..\Intermediate\Shaders\Effect_NormalVS.cso.h"
#include "..\Intermediate\Shaders\Effect_NormalTexcoordVS.cso.h"
#include "..\Intermediate\Shaders\Effect_NormalTangentTexcoordVS.cso.h"
#include "..\Intermediate\Shaders\Effect_PerMaterialAlbedoRoughtnessMetalnessPS.cso.h"
#include "..\Intermediate\Shaders\Effect_AlbedoTexturePerMaterialRoughtnessMetalnessPS.cso.h"
#include "..\Intermediate\Shaders\Effect_AlbedoNormalRoughtnessMetalnessTexturePS.cso.h"

#include "..\Intermediate\Shaders\SceneGeometryPositionOnlyVS.cso.h"

#include "..\Intermediate\Shaders\UIColorVS.cso.h"
#include "..\Intermediate\Shaders\UIColorPS.cso.h"
#include "..\Intermediate\Shaders\UIColorAlphaTextureVS.cso.h"
#include "..\Intermediate\Shaders\UIColorAlphaTexturePS.cso.h"

#include "..\Intermediate\Shaders\DebugWhitePS.cso.h"

using namespace XEngine::Render::Internal;

const ShaderData Shaders::ScreenQuadVS = { ScreenQuadVSData, sizeof(ScreenQuadVSData) };
const ShaderData Shaders::DepthBufferDownscalePS = { DepthBufferDownscalePSData, sizeof(DepthBufferDownscalePSData) };
const ShaderData Shaders::LightingPassPS = { LightingPassPSData, sizeof(LightingPassPSData) };
const ShaderData Shaders::BloomFilterAndDownscaleX4PS = { BloomFilterAndDownscaleX4PSData, sizeof(BloomFilterAndDownscaleX4PSData) };
const ShaderData Shaders::BloomBlurPS = { BloomBlurPSData, sizeof(BloomBlurPSData) };
const ShaderData Shaders::BloomDownscaleX2PS = { BloomDownscaleX2PSData, sizeof(BloomDownscaleX2PSData) };
const ShaderData Shaders::ToneMappingPS = { ToneMappingPSData, sizeof(ToneMappingPSData) };

const ShaderData Shaders::Effect_NormalVS = { Effect_NormalVSData, sizeof(Effect_NormalVSData) };
const ShaderData Shaders::Effect_NormalTexcoordVS = { Effect_NormalTexcoordVSData, sizeof(Effect_NormalTexcoordVSData) };
const ShaderData Shaders::Effect_NormalTangentTexcoordVS = { Effect_NormalTangentTexcoordVSData, sizeof(Effect_NormalTangentTexcoordVSData) };
const ShaderData Shaders::Effect_PerMaterialAlbedoRoughtnessMetalnessPS = { Effect_PerMaterialAlbedoRoughtnessMetalnessPSData, sizeof(Effect_PerMaterialAlbedoRoughtnessMetalnessPSData) };
const ShaderData Shaders::Effect_AlbedoTexturePerMaterialRoughtnessMetalnessPS = { Effect_AlbedoTexturePerMaterialRoughtnessMetalnessPSData, sizeof(Effect_AlbedoTexturePerMaterialRoughtnessMetalnessPSData) };
const ShaderData Shaders::Effect_AlbedoNormalRoughtnessMetalnessTexturePS = { Effect_AlbedoNormalRoughtnessMetalnessTexturePSData, sizeof(Effect_AlbedoNormalRoughtnessMetalnessTexturePSData) };

const ShaderData Shaders::SceneGeometryPositionOnlyVS = { SceneGeometryPositionOnlyVSData, sizeof(SceneGeometryPositionOnlyVSData) };

const ShaderData Shaders::UIColorVS = { UIColorVSData, sizeof(UIColorVSData) };
const ShaderData Shaders::UIColorPS = { UIColorPSData, sizeof(UIColorPSData) };
const ShaderData Shaders::UIColorAlphaTextureVS = { UIColorAlphaTextureVSData, sizeof(UIColorAlphaTextureVSData) };
const ShaderData Shaders::UIColorAlphaTexturePS = { UIColorAlphaTexturePSData, sizeof(UIColorAlphaTexturePSData) };

const ShaderData Shaders::DebugWhitePS = { DebugWhitePSData, sizeof(DebugWhitePSData) };