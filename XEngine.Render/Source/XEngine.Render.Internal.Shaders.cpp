#include <Windows.h>

#include "XEngine.Render.Internal.Shaders.h"

#include "..\Intermediate\Shaders\ScreenQuadVS.cso.h"
#include "..\Intermediate\Shaders\DepthDeprojectAndDownscalePS.cso.h"
#include "..\Intermediate\Shaders\DepthDownscalePS.cso.h"
#include "..\Intermediate\Shaders\LightingPassPS.cso.h"
#include "..\Intermediate\Shaders\BloomFilterAndDownscalePS.cso.h"
#include "..\Intermediate\Shaders\BloomBlurHorizontalPS.cso.h"
#include "..\Intermediate\Shaders\BloomBlurVerticalPS.cso.h"
#include "..\Intermediate\Shaders\BloomBlurVerticalAndAccumulatePS.cso.h"
#include "..\Intermediate\Shaders\BloomDownscalePS.cso.h"
#include "..\Intermediate\Shaders\ToneMappingPS.cso.h"

#include "..\Intermediate\Shaders\Effect_NormalVS.cso.h"
#include "..\Intermediate\Shaders\Effect_NormalTexcoordVS.cso.h"
#include "..\Intermediate\Shaders\Effect_NormalTangentTexcoordVS.cso.h"
#include "..\Intermediate\Shaders\Effect_PerMaterialAlbedoRoughtnessMetalnessPS.cso.h"
#include "..\Intermediate\Shaders\Effect_AlbedoTexturePerMaterialRoughtnessMetalnessPS.cso.h"
#include "..\Intermediate\Shaders\Effect_AlbedoNormalRoughtnessMetalnessTexturePS.cso.h"
#include "..\Intermediate\Shaders\Effect_PerMaterialEmissiveColorPS.cso.h"

#include "..\Intermediate\Shaders\SceneGeometryPositionOnlyVS.cso.h"

#include "..\Intermediate\Shaders\UIColorVS.cso.h"
#include "..\Intermediate\Shaders\UIColorPS.cso.h"
#include "..\Intermediate\Shaders\UIColorAlphaTextureVS.cso.h"
#include "..\Intermediate\Shaders\UIColorAlphaTexturePS.cso.h"

#include "..\Intermediate\Shaders\DebugWhitePS.cso.h"

using namespace XEngine::Render::Internal;

const ShaderData Shaders::ScreenQuadVS = { ScreenQuadVSData, sizeof(ScreenQuadVSData) };
const ShaderData Shaders::DepthDeprojectAndDownscalePS = { DepthDeprojectAndDownscalePSData, sizeof(DepthDeprojectAndDownscalePSData) };
const ShaderData Shaders::DepthDownscalePS = { DepthDownscalePSData, sizeof(DepthDownscalePSData) };
const ShaderData Shaders::LightingPassPS = { LightingPassPSData, sizeof(LightingPassPSData) };
const ShaderData Shaders::BloomFilterAndDownscalePS = { BloomFilterAndDownscalePSData, sizeof(BloomFilterAndDownscalePSData) };
const ShaderData Shaders::BloomDownscalePS = { BloomDownscalePSData, sizeof(BloomDownscalePSData) };
const ShaderData Shaders::BloomBlurHorizontalPS = { BloomBlurHorizontalPSData, sizeof(BloomBlurHorizontalPSData) };
const ShaderData Shaders::BloomBlurVerticalPS = { BloomBlurVerticalPSData, sizeof(BloomBlurVerticalPSData) };
const ShaderData Shaders::BloomBlurVerticalAndAccumulatePS = { BloomBlurVerticalAndAccumulatePSData, sizeof(BloomBlurVerticalAndAccumulatePSData) };
const ShaderData Shaders::ToneMappingPS = { ToneMappingPSData, sizeof(ToneMappingPSData) };

const ShaderData Shaders::Effect_NormalVS = { Effect_NormalVSData, sizeof(Effect_NormalVSData) };
const ShaderData Shaders::Effect_NormalTexcoordVS = { Effect_NormalTexcoordVSData, sizeof(Effect_NormalTexcoordVSData) };
const ShaderData Shaders::Effect_NormalTangentTexcoordVS = { Effect_NormalTangentTexcoordVSData, sizeof(Effect_NormalTangentTexcoordVSData) };
const ShaderData Shaders::Effect_PerMaterialAlbedoRoughtnessMetalnessPS = { Effect_PerMaterialAlbedoRoughtnessMetalnessPSData, sizeof(Effect_PerMaterialAlbedoRoughtnessMetalnessPSData) };
const ShaderData Shaders::Effect_AlbedoTexturePerMaterialRoughtnessMetalnessPS = { Effect_AlbedoTexturePerMaterialRoughtnessMetalnessPSData, sizeof(Effect_AlbedoTexturePerMaterialRoughtnessMetalnessPSData) };
const ShaderData Shaders::Effect_AlbedoNormalRoughtnessMetalnessTexturePS = { Effect_AlbedoNormalRoughtnessMetalnessTexturePSData, sizeof(Effect_AlbedoNormalRoughtnessMetalnessTexturePSData) };
const ShaderData Shaders::Effect_PerMaterialEmissiveColorPS = { Effect_PerMaterialEmissiveColorPSData, sizeof(Effect_PerMaterialEmissiveColorPSData) };

const ShaderData Shaders::SceneGeometryPositionOnlyVS = { SceneGeometryPositionOnlyVSData, sizeof(SceneGeometryPositionOnlyVSData) };

const ShaderData Shaders::UIColorVS = { UIColorVSData, sizeof(UIColorVSData) };
const ShaderData Shaders::UIColorPS = { UIColorPSData, sizeof(UIColorPSData) };
const ShaderData Shaders::UIColorAlphaTextureVS = { UIColorAlphaTextureVSData, sizeof(UIColorAlphaTextureVSData) };
const ShaderData Shaders::UIColorAlphaTexturePS = { UIColorAlphaTexturePSData, sizeof(UIColorAlphaTexturePSData) };

const ShaderData Shaders::DebugWhitePS = { DebugWhitePSData, sizeof(DebugWhitePSData) };