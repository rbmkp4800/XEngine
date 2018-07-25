#include <Windows.h>

#include "XEngine.Render.Internal.Shaders.h"

#include "..\Intermediate\Shaders\ScreenQuadVS.cso.h"
#include "..\Intermediate\Shaders\LightingPassPS.cso.h"

#include "..\Intermediate\Shaders\EffectPlainVS.cso.h"
#include "..\Intermediate\Shaders\EffectPlainPS.cso.h"
#include "..\Intermediate\Shaders\ShadowPassVS.cso.h"

#include "..\Intermediate\Shaders\UIColorVS.cso.h"
#include "..\Intermediate\Shaders\UIColorPS.cso.h"
#include "..\Intermediate\Shaders\UIColorAlphaTextureVS.cso.h"
#include "..\Intermediate\Shaders\UIColorAlphaTexturePS.cso.h"

using namespace XEngine::Render::Internal;

const ShaderData Shaders::ScreenQuadVS = { ScreenQuadVSData, sizeof(ScreenQuadVSData) };
const ShaderData Shaders::LightingPassPS = { LightingPassPSData, sizeof(LightingPassPSData) };

const ShaderData Shaders::EffectPlainVS = { EffectPlainVSData, sizeof(EffectPlainVSData) };
const ShaderData Shaders::EffectPlainPS = { EffectPlainPSData, sizeof(EffectPlainPSData) };
const ShaderData Shaders::ShadowPassVS = { ShadowPassVSData, sizeof(ShadowPassVSData) };

const ShaderData Shaders::UIColorVS = { UIColorVSData, sizeof(UIColorVSData) };
const ShaderData Shaders::UIColorPS = { UIColorPSData, sizeof(UIColorPSData) };
const ShaderData Shaders::UIColorAlphaTextureVS = { UIColorAlphaTextureVSData, sizeof(UIColorAlphaTextureVSData) };
const ShaderData Shaders::UIColorAlphaTexturePS = { UIColorAlphaTexturePSData, sizeof(UIColorAlphaTexturePSData) };