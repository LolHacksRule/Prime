#include "D3DStateManager.h"
#include "../../../IGraphicsDriver.h"
#include "../../../SexyMath.h"

using namespace Sexy;

void D3DStateManager::InitRenderState(ulong inIndex, const std::string& inStateName, ulong inDefaultValue, bool inHasContextDefault, ulong inContextDefaultValue, const char* inValueEnumName) //53-60
{
	std::string aName = StrFormat("RS:%s", inStateName.c_str());
	if (inHasContextDefault)
		mRenderStates[inIndex].Init(StateValue(inDefaultValue), StateValue(inContextDefaultValue), aName, inValueEnumName); //?
	else
		mRenderStates[inIndex].Init(StateValue(inDefaultValue), aName, inValueEnumName);
}
void D3DStateManager::InitRenderStateFloat(ulong inIndex, const std::string& inStateName, float inDefaultValue) //62-64
{
	InitRenderState(inIndex, inStateName, inDefaultValue, false, 0);
}

///////////////////////////////////////////////////////////////////////////////
void D3DStateManager::InitTextureStageState(ulong inFirstStage, ulong inLastStage, ulong inIndex, const std::string& inStateName, ulong inDefaultValue, bool inHasContextDefault, ulong inContextDefaultValue, const char* inValueEnumName) //68-78
{
	for (ulong i = inFirstStage; i <= inLastStage; i++)
	{
		std::string aName = StrFormat("TSS:%s[%d]", inStateName.c_str(), i);
		if (inHasContextDefault)
			mTextureStageStates[inIndex][i].Init(StateValue(inDefaultValue), StateValue(inContextDefaultValue), aName, inValueEnumName); //?
		else
			mTextureStageStates[inIndex][i].Init(StateValue(inDefaultValue), aName, inValueEnumName);
	}
}
void D3DStateManager::InitTextureStageStateFloat(ulong inFirstStage, ulong inLastStage, ulong inIndex, const std::string& inStateName, float inDefaultValue) //80-82
{
	InitTextureStageState(inFirstStage, inLastStage, inIndex, inStateName, inDefaultValue, false, 0);
}

///////////////////////////////////////////////////////////////////////////////
void D3DStateManager::InitSamplerState(ulong inFirstStage, ulong inLastStage, ulong inIndex, const std::string& inStateName, ulong inDefaultValue, bool inHasContextDefault, ulong inContextDefaultValue, const char* inValueEnumName) //86-96
{
	for (ulong i = inFirstStage; i <= inLastStage; i++)
	{
		std::string aName = StrFormat("SS:%s[%d]", inStateName.c_str(), i);
		if (inHasContextDefault)
			mSamplerStates[inIndex][i].Init(StateValue(inDefaultValue), StateValue(inContextDefaultValue), aName, inValueEnumName); //?
		else
			mSamplerStates[inIndex][i].Init(StateValue(inDefaultValue), aName, inValueEnumName);
	}
}

void D3DStateManager::InitStates() //99-452 (TODO)
{
	assert("Not yet decompiled.");
	const ulong kSamplerStages = ST_COUNT_SS;
	const ulong kNumPSConstants = 0;
	const ulong kLightCount = ST_COUNT_LIGHT;
	const ulong kNumTextures = 0;
	const ulong kNumClipPlanes = 0;
	const ulong kTssStages = ST_COUNT_TSS;
	const ulong kStreamCount = 0;
	const ulong kNumVSConstants = 0;
	for (ulong i = 0; i < ST_COUNT_RS; i++)
		mRenderStates.push_back(State(this, SG_RS, i, 0, 0));

	InitRenderState(7, "ZENABLE", 1, true, 0); //Do they use their custom structs idk
	InitRenderState(8, "FILLMODE", 3, true, 3, "D3DFILLMODE");
	InitRenderState(9, "SHADEMODE", 2, false, 0, "D3DSHADEMODE");
	InitRenderState(14, "ZWRITEENABLE", 1, true, 0);
	InitRenderState(15, "ALPHATESTENABLE", 0, true, 0);
	InitRenderState(16, "LASTPIXEL", 1, false, 0);
	InitRenderState(19, "SRCBLEND", 2, true, 5, "D3DBLEND");
	InitRenderState(20, "DESTBLEND", 1, true, 6, "D3DBLEND");
	InitRenderState(22, "CULLMODE", 3, true, 1, "D3DCULL");
	InitRenderState(23, "ZFUNC", 4, false, 0, "D3DCULL");
	InitRenderState(24, "ALPHAREF", 0, true, 0);
	InitRenderState(25, "ALPHAFUNC", 8, true, 5, "D3DCMPFUNC");
	InitRenderState(26, "DITHERENABLE", 0, true, 0);
	InitRenderState(27, "ALPHABLENDENABLE", 0, true, 1);
	InitRenderState(28, "FOGENABLE", 0, false, 0);
	InitRenderState(29, "SPECULARENABLE", 0, true, 0);
	InitRenderState(34, "FOGCOLOR", 0, false, 0);
	InitRenderState(35, "FOGTABLEMODE", 0, false, 0, "D3DFOGMODE");
	InitRenderStateFloat(36, "FOGSTART", 0.0);
	InitRenderStateFloat(37, "FOGEND", 1.0);
	InitRenderStateFloat(38, "FOGDENSITY", 1.0);
	InitRenderState(48, "RANGEFOGENABLE", 0, false, 0);
	InitRenderState(52, "STENCILENABLE", 0, false, 0);
	InitRenderState(53, "STENCILFAIL", 1, false, 0, "D3DSTENCILOP");
	InitRenderState(54, "STENCILZFAIL", 1, false, 0, "D3DSTENCILOP");
	InitRenderState(55, "STENCILPASS", 1, false, 0, "D3DSTENCILOP");
	InitRenderState(56, "STENCILFUNC", 8, false, 0, "D3DCMPFUNC");
	InitRenderState(57, "STENCILREF", 0, false, 0);
	InitRenderState(58, "STENCILMASK", 0xFFFFFFFF, false, 0);
	InitRenderState(59, "STENCILWRITEMASK", 0xFFFFFFFF, false, 0);
	InitRenderState(60, "TEXTUREFACTOR", 0xFFFFFFFF, false, 0);
	InitRenderState(128, "WRAP0", 0, false, 0);
	InitRenderState(129, "WRAP1", 0, false, 0);
	InitRenderState(130, "WRAP2", 0, false, 0);
	InitRenderState(131, "WRAP3", 0, false, 0);
	InitRenderState(132, "WRAP4", 0, false, 0);
	InitRenderState(133, "WRAP5", 0, false, 0);
	InitRenderState(134, "WRAP6", 0, false, 0);
	InitRenderState(135, "WRAP7", 0, false, 0);
	InitRenderState(136, "CLIPPING", 1, true, 1);
	InitRenderState(137, "LIGHTING", 1, true, 1);
	InitRenderState(139, "AMBIENT", 1, true, 1);
	InitRenderState(140, "FOGVERTEXMODE", 1, true, 1, "D3DFOGMODE");
	InitRenderState(141, "COLORVERTEX", 1, false, 0);
	InitRenderState(142, "LOCALVIEWER", 1, false, 0);
	InitRenderState(143, "NORMALIZENORMALS", 0, false, 0);
	InitRenderState(145, "DIFFUSEMATERIALSOURCE", 1, false, 0, "D3DMATERIALCOLORSOURCE");
	InitRenderState(146, "SPECULARMATERIALSOURCE", 2, false, 0, "D3DMATERIALCOLORSOURCE");
	InitRenderState(147, "AMBIENTMATERIALSOURCE", 0, false, 0, "D3DMATERIALCOLORSOURCE");
	InitRenderState(148, "EMISSIVEMATERIALSOURCE", 0, false, 0, "D3DMATERIALCOLORSOURCE");
	InitRenderState(151, "VERTEXBLEND", 0, false, 0, "D3DVERTEXBLENDFLAGS");
	InitRenderState(152, "CLIPPLANEENABLE", 0, false, 0);
	InitRenderStateFloat(154, "POINTSIZE", 64.0);
	InitRenderStateFloat(155, "POINTSIZE_MIN", 1.0);
	InitRenderState(156, "POINTSPRITEENABLE", 0, false, 0);
	InitRenderState(157, "POINTSCALEENABLE", 0, false, 0);
	InitRenderStateFloat(158, "POINTSCALE_A", 1.0);
	InitRenderStateFloat(159, "POINTSCALE_B", 0.0);
	InitRenderStateFloat(160, "POINTSCALE_C", 0.0);
	InitRenderState(161, "MULTISAMPLEANTIALIAS", 1, false, 0);
	InitRenderState(162, "MULTISAMPLEMASK", 0xFFFFFFFF, false, 0);
	InitRenderState(163, "PATCHEDGESTYLE", 0, false, 0, "D3DPATCHEDGESTYLE");
	InitRenderStateFloat(166, "POINTSIZE_MAX", 64.0);
	InitRenderState(167, "INDEXEDVERTEXBLENDENABLE", 0, false, 0);
	InitRenderState(168, "COLORWRITEENABLE", 15, false, 0);
	InitRenderStateFloat(170, "TWEENFACTOR", 0.0);
	InitRenderState(171, "BLENDOP", 1, false, 0, "D3DBLENDOP");
	InitRenderState(172, "POSITIONDEGREE", 3, false, 0, "D3DDEGREETYPE");
	InitRenderState(173, "NORMALDEGREE", 1, false, 0, "D3DDEGREETYPE");
	InitRenderState(174, "SCISSORTESTENABLE", 0, false, 0);
	InitRenderState(175, "SLOPESCALEDEPTHBIAS", 0, false, 0);
	InitRenderState(176, "ANTIALIASEDLINEENABLE", 0, false, 0);
	InitRenderStateFloat(178, "MINTESSELLATIONLEVEL", 1.0);
	InitRenderStateFloat(179, "MAXTESSELLATIONLEVEL", 1.0);
	InitRenderStateFloat(180, "ADAPTIVETESS_X", 0.0);
	InitRenderStateFloat(181, "ADAPTIVETESS_Y", 0.0);
	InitRenderStateFloat(182, "ADAPTIVETESS_Z", 0.0);
	InitRenderStateFloat(183, "ADAPTIVETESS_W", 0.0);
	InitRenderState(184, "ENABLEADAPTIVETESSELLATION", 0, false, 0);
	InitRenderState(185, "TWOSIDEDSTENCILMODE", 0, false, 0);
	InitRenderState(186, "CCW_STENCILFAIL", 1, false, 0, "D3DSTENCILOP");
	InitRenderState(187, "CCW_STENCILZFAIL", 1, false, 0, "D3DSTENCILOP");
	InitRenderState(188, "CCW_STENCILPASS", 1, false, 0, "D3DSTENCILOP");
	InitRenderState(189, "CCW_STENCILFUNC", 8, false, 0, "D3DCMPFUNC");
	InitRenderState(190, "COLORWRITEENABLE1", 0, false, 0);
	InitRenderState(191, "COLORWRITEENABLE2", 0, false, 0);
	InitRenderState(192, "COLORWRITEENABLE3", 0, false, 0);
	InitRenderState(193, "BLENDFACTOR", 0xFFFFFFFF, false, 0);
	InitRenderState(194, "SRGBWRITEENABLE", 0, false, 0);
	InitRenderStateFloat(195, "DEPTHBIAS", 0.0);
	InitRenderState(198, "WRAP8", 0, false, 0);
	InitRenderState(199, "WRAP9", 0, false, 0);
	InitRenderState(200, "WRAP10", 0, false, 0);
	InitRenderState(201, "WRAP11", 0, false, 0);
	InitRenderState(202, "WRAP12", 0, false, 0);
	InitRenderState(203, "WRAP13", 0, false, 0);
	InitRenderState(204, "WRAP14", 0, false, 0);
	InitRenderState(205, "WRAP15", 0, false, 0);
	InitRenderState(206, "SEPARATEALPHABLENDENABLE", 0, 0, 0);
	InitRenderState(207, "SRCBLENDALPHA", 2, false, 0, "D3DBLEND");
	InitRenderState(208, "DESTBLENDALPHA", 1, false, 0, "D3DBLEND");
	InitRenderState(209, "BLENDOPALPHA", 1, false, 0, "D3DBLENDOP");
	//TODO
	for (ulong i = 0; i < kTssStages; i++)
		mTextureStageStates[i].push_back(State(this, 0, i, 0, 0)); //?
	for (ulong i = 0; i < kTssStages; i++)
	{
		for (ulong j = 0; j < IGraphicsDriver::RENDERMODE_COUNT; j++) //?
			mTextureStageStates[i].push_back(State(this, SG_TSS, i, j, 0)); //?
	}
	//TODO
	InitTextureStageState(0, 0, 1, "COLOROP", 4, true, 4, "D3DTEXTUREOP");
	InitTextureStageState(1, 1, 1, "COLOROP", 1, true, 1, "D3DTEXTUREOP");
	InitTextureStageState(2, 7, 1, "COLOROP", 1, false, 0, "D3DTEXTUREOP");
	InitTextureStageState(0, 0, 2, "COLORARG1", 2, true, 2);
	InitTextureStageState(1, 7, 2, "COLORARG1", 2, false, 0);
	InitTextureStageState(0, 0, 3, "COLORARG2", 1, true, 0);
	InitTextureStageState(1, 7, 3, "COLORARG2", 1, false, 0);
	InitTextureStageState(0, 0, 4, "ALPHAOP", 2, true, 4, "D3DTEXTUREOP");
	InitTextureStageState(1, 1, 4, "ALPHAOP", 1, true, 1, "D3DTEXTUREOP");
	InitTextureStageState(2, 7, 4, "ALPHAOP", 1, false, 0, "D3DTEXTUREOP");
	InitTextureStageState(0, 0, 5, "ALPHAARG1", 2, true, 2);
	InitTextureStageState(1, 7, 5, "ALPHAARG1", 2, false, 0);
	InitTextureStageState(0, 0, 6, "ALPHAARG2", 1, true, 0);
	InitTextureStageState(1, 7, 6, "ALPHAARG2", 1, false, 0);
	InitTextureStageStateFloat(0, 7, 7, "BUMPENVMAT00", 0.0);
	InitTextureStageStateFloat(0, 7, 8, "BUMPENVMAT01", 0.0);
	InitTextureStageStateFloat(0, 7, 9, "BUMPENVMAT10", 0.0);
	InitTextureStageStateFloat(0, 7, 10, "BUMPENVMAT11", 0.0);

	for (ulong i = 0; i < IGraphicsDriver::RENDERMODE_COUNT; i++)
		InitTextureStageState(i, i, 0xB, "TEXCOORDINDEX", i, 0, 0);

	InitTextureStageStateFloat(0, 7, 22, "BUMPENVLSCALE", 0.0);
	InitTextureStageStateFloat(0, 7, 23, "BUMPENVLOFFSET", 0.0);
	InitTextureStageState(0, 7, 24, "TEXTURETRANSFORMFLAGS", 0, 1, 0);
	InitTextureStageState(0, 7, 26, "COLORARG0", true, 0, 0);
	InitTextureStageState(0, 7, 27, "ALPHAARG0", true, 0, 0);
	InitTextureStageState(0, 7, 28, "RESULTARG", true, 0, 0);
	InitTextureStageState(0, 7, 32, "CONSTANT", false, 0, 0);

	for (ulong i = 0; i < kSamplerStages; i++)
		mSamplerStates[i].push_back(State(this, 0, i, 0, 0)); //?

	for (ulong i = 0; i < kSamplerStages; i++)
	{
		for (ulong j = 0; j < IGraphicsDriver::RENDERMODE_COUNT; j++) //?
			mSamplerStates[i].push_back(State(this, SG_SS, i, j, 0)); //?
	}

	InitSamplerState(0, 0, 1, "ADDRESSU", 1, true, 3, "D3DTEXTUREADDRESS");
	InitSamplerState(0, 0, 2, "ADDRESSV", 1, true, 3, "D3DTEXTUREADDRESS");
	InitSamplerState(1, 7, 1, "ADDRESSU", 1, false, 0, "D3DTEXTUREADDRESS");
	InitSamplerState(1, 7, 2, "ADDRESSV", 1, false, 0, "D3DTEXTUREADDRESS");
	InitSamplerState(0, 7, 3, "ADDRESSW", 1, false, 0, "D3DTEXTUREADDRESS");
	InitSamplerState(0, 7, 4, "BORDERCOLOR", false, 0, 0);
	InitSamplerState(0, 0, 5, "MAGFILTER", 1, true, 1, "D3DTEXTUREFILTERTYPE");
	InitSamplerState(0, 0, 6, "MINFILTER", 1, true, 1, "D3DTEXTUREFILTERTYPE");
	InitSamplerState(0, 0, 7, "MIPFILTER", 0, true, 1, "D3DTEXTUREFILTERTYPE");
	InitSamplerState(1, 7, 5, "MAGFILTER", 1, false, 0, "D3DTEXTUREFILTERTYPE");
	InitSamplerState(0, 7, 8, "MIPMAPLODBIAS", 0, false, 0);
	InitSamplerState(0, 7, 9, "MAXMIPLEVEL", 0, false, 0);
	InitSamplerState(0, 7, 10, "MAXANISOTROPY", 1, false, 0);
	InitSamplerState(0, 7, 11, "SRGBTEXTURE", 0, false, 0);
	InitSamplerState(0, 7, 12, "ELEMENTINDEX", 0, false, 0);
	InitSamplerState(0, 7, 13, "DMAPOFFSET", 0, false, 0);

	for (ulong i = 0; i < kLightCount; i++)
		mLightStates[i].push_back(State(this, 3, i, 0, 0)); //?

	for (ulong i = 0; i < kLightCount; i++)
	{
		for (ulong j = 0; j < IGraphicsDriver::RENDERMODE_COUNT; j++) //?
			mLightStates[i].push_back(State(this, SG_LIGHT, i, j, 0)); //?
	}

	for (ulong i = 0; i < IGraphicsDriver::RENDERMODE_COUNT; i++)
	{
		mLightStates[ST_LIGHT_ENABLED][i].Init(0UL, StrFormat("LIGHT:ENABLED[0]", i));
		mLightStates[ST_LIGHT_TYPE][i].Init(0UL, StrFormat("LIGHT:TYPE[0]", i), "D3DLIGHTTYPE");
		mLightStates[ST_LIGHT_DIFFUSE][i].Init(StateValue(1.0, 1.0, 10.0, 0.0), StrFormat("LIGHT:DIFFUSE[0]", i));
		mLightStates[ST_LIGHT_SPECULAR][i].Init(StateValue(0.0, 0.0, 0.0, 0.0), StrFormat("LIGHT:SPECULAR[0]", i));
		mLightStates[ST_LIGHT_AMBIENT][i].Init(StateValue(0.0, 0.0, 0.0, 0.0), StrFormat("LIGHT:AMBIENT[0]", i));
		mLightStates[ST_LIGHT_POSITION][i].Init(StateValue(0.0, 0.0, 0.0, 0.0), StrFormat("LIGHT:POSITION[0]", i));
		mLightStates[ST_LIGHT_DIRECTION][i].Init(StateValue(0.0, 0.0, 1.0, 0.0), StrFormat("LIGHT:DIRECTION[0]", i));
		mLightStates[ST_LIGHT_RANGE][i].Init(0UL, StrFormat("LIGHT:RANGE[%d]", i));
		mLightStates[ST_LIGHT_FALLOFF][i].Init(0UL, StrFormat("LIGHT:FALLOFF[%d]", i));
		mLightStates[ST_LIGHT_ATTENUATION][i].Init(StateValue(0.0, 0.0, 0.0, 0.0), StrFormat("LIGHT:ATTENUATION[0]", i));
		mLightStates[ST_LIGHT_ANGLES][i].Init(StateValue(0.0, 0.0, 0.0, 0.0), StrFormat("LIGHT:ANGLES[0]", i));
	}
	for (ulong i = 0; i < ST_COUNT_MAT; i++)
		mTransformStates[i].push_back(State(this, SG_MATERIAL, i, 0, 0));

	mMaterialStates[ST_MAT_DIFFUSE].Init(StateValue(0.0, 0.0, 0.0, 0.0), "MAT:DIFFUSE");
	mMaterialStates[ST_MAT_AMBIENT].Init(StateValue(0.0, 0.0, 0.0, 0.0), "MAT:AMBIENT");
	mMaterialStates[ST_MAT_SPECULAR].Init(StateValue(0.0, 0.0, 0.0, 0.0), "MAT:SPECULAR");
	mMaterialStates[ST_MAT_EMISSIVE].Init(StateValue(0.0, 0.0, 0.0, 0.0), "MAT:EMISSIVE");
	mMaterialStates[ST_MAT_POWER].Init(StateValue(0.0f), "MAT:POWER");

	for (ulong i = 0; i < ST_COUNT_STREAM; i++)
		mStreamStates[i].push_back(State(this, SG_STREAM, i, 0, 0));

	for (ulong i = 0; i < kStreamCount; i++)
	{
		for (ulong j = 0; j < IGraphicsDriver::RENDERMODE_COUNT; j++) //?
			mStreamStates[i].push_back(State(this, SG_STREAM, i, j, 0)); //?
	}

	for (ulong i = 0; i < IGraphicsDriver::RENDERMODE_COUNT; i++)
	{
		mLightStates[ST_STREAM_DATA][i].Init(0UL, StrFormat("STREAM:DATA[%d]", i));
		mLightStates[ST_STREAM_OFFSET][i].Init(0UL, StrFormat("STREAM:OFFSET[%d]", i));
		mLightStates[ST_STREAM_STRIDE][i].Init(0UL, StrFormat("STREAM:STRIDE[%d]", i));
		mLightStates[ST_STREAM_FREQ][i].Init(0UL, StrFormat("STREAM:FREQ[%d]", i));
	}

	for (ulong i = 0; i < ST_COUNT_TRANSFORM; i++)
		mTransformStates[i].push_back(State(this, SG_TRANSFORM, i, 0, 0));

	for (ulong i = 0; i < kStreamCount; i++)
	{
		for (ulong j = 0; j < IGraphicsDriver::RENDERMODE_COUNT; j++) //?
			mTransformStates[i].push_back(State(this, SG_TRANSFORM, i, j, 0)); //?
	}

	for (ulong i = 0; i < ST_COUNT_TRANSFORM; i++)
	{
		std::string aName;
		if (i == 0)
			aName = "WORLD";
		else if (i == 1)
			aName = "VIEW";
		else if (i == 2)
			aName = "PROJECTION";
		else if (i >= 3U && i <= 10U)
			aName = StrFormat("TEXTURE%d", i - 3U);
		else
			aName = StrFormat("%d", i);
		for (ulong j = 0; j < 4; j++)
			mTransformStates[i][j].Init(StateValue(0.0, 0.0, 0.0, 0.0), StrFormat("TRANSFORM:%s[%d]", aName, i));
	}

	for (ulong i = 0; i < ST_COUNT_VIEWPORT; i++)
		mViewportStates.push_back(State(this, SG_VIEWPORT, i, 0, 0));

	mViewportStates[ST_VIEWPORT_X].Init(0UL, "VIEWPORT:X");
	mViewportStates[ST_VIEWPORT_Y].Init(0UL, "VIEWPORT:Y");
	mViewportStates[ST_VIEWPORT_WIDTH].Init(0UL, "VIEWPORT:WIDTH");
	mViewportStates[ST_VIEWPORT_HEIGHT].Init(0UL, "VIEWPORT:HEIGHT");
	mViewportStates[ST_VIEWPORT_MINZ].Init(StateValue(0.0f), "VIEWPORT_MINZ");
	mViewportStates[ST_VIEWPORT_MAXZ].Init(StateValue(1.0f), "VIEWPORT_MAXZ");

	for (ulong i = 0; i < ST_COUNT_MISC; i++)
		mMiscStates[i].push_back(State(this, SG_MISC, i, 0, 0));

	for (ulong i = 0; i < ST_COUNT_MISC_SINGLE; i++)
		mMiscStates[i].push_back(State(this, SG_MISC, i, 0, 0));

	//TODO
}

void D3DStateManager::ResetStates() //455-489
{
	ulong iCount = mRenderStates.size();
	for (ulong i = 0; i < iCount; i++)
		mRenderStates[i].Reset();
	ulong iCount = mTextureStageStates.size();
	for (ulong i = 0; i < iCount; i++)
	{
		ulong jCount = mTextureStageStates.size();
		for (ulong j = 0; j < jCount; j++)
			mTextureStageStates[j][i].Reset();
	}
	ulong iCount = mSamplerStates.size();
	for (ulong i = 0; i < iCount; i++)
	{
		ulong jCount = mSamplerStates.size();
		for (ulong j = 0; j < jCount; j++)
			mSamplerStates[j][i].Reset();
	}
	ulong iCount = mLightStates.size();
	for (ulong i = 0; i < iCount; i++)
	{
		ulong jCount = mLightStates.size();
		for (ulong j = 0; j < jCount; j++)
			mLightStates[j][i].Reset();
	}
	ulong iCount = mMaterialStates.size();
	for (ulong i = 0; i < iCount; i++)
		mMaterialStates[i].Reset();
	ulong iCount = mStreamStates.size();
	for (ulong i = 0; i < iCount; i++)
	{
		ulong jCount = mStreamStates.size();
		for (ulong j = 0; j < jCount; j++)
			mStreamStates[j][i].Reset();
	}
	ulong iCount = mTransformStates.size();
	for (ulong i = 0; i < iCount; i++)
	{
		ulong jCount = mTransformStates.size();
		for (ulong j = 0; j < jCount; j++)
			mTransformStates[j][i].Reset();
	}
	ulong iCount = mMiscStates.size();
	for (ulong i = 0; i < iCount; i++)
	{
		ulong jCount = mMiscStates.size();
		for (ulong j = 0; j < jCount; j++)
			mMiscStates[j][i].Reset();
	}
}

RenderStateManager::StateValue D3DStateManager::MakeLightColorStateValue(const Color& inColor, float inAutoScale) //492-499
{
	return StateValue(
		(float)inColor.mAlpha / 255.0 * inAutoScale,
		(float)inColor.mBlue / 255.0 * inAutoScale,
		(float)inColor.mGreen / 255.0 * inAutoScale,
		(float)inColor.mRed / 255.0 * inAutoScale
	);
}
Color D3DStateManager::MakeStateValueLightColor(const State& inState) //501-509
{
	float r, g, b, a;
	inState.GetVector(r, g, b, a);
	int rInt = max(0.0, r * 255.0); //?
	int gInt = max(0.0, g * 255.0); //?
	int bInt = max(0.0, b * 255.0); //?
	int aInt = max(0.0, a * 255.0); //?
	return Color(rInt, gInt, bInt, aInt);
}

ulong D3DStateManager::MakeFVFSize(ulong inFVF) //TODO | 512-553
{
	/*ulong result = 0;
	if ((inFVF & 2) != 0)
		result = 12;
	else if ((inFVF & 4) != 0)
		result = 16;
	if ((inFVF & 16) != 0)
		result += 12;
	if ((inFVF & 64) != 0)
		result += 4;
	if ((inFVF & 128) != 0)
		result += 4;*/
	return 0;
}

void D3DStateManager::SetRenderState(ulong inRS, ulong inValue) //556-558
{
	mRenderStates[inRS].SetValue(inValue);
}
void D3DStateManager::SetTextureStageState(ulong inStage, ulong inTSS, ulong inValue) //560-562
{
	mTextureStageStates[inTSS][inStage].SetValue(inValue);
}
void D3DStateManager::SetSamplerState(ulong inStage, ulong inSS, ulong inValue) //564-566
{
	mSamplerStates[inSS][inStage].SetValue(inValue);
}
void D3DStateManager::SetLightEnabled(ulong inLightIndex, bool inEnabled) //568-570
{
	mLightStates[ST_LIGHT_ENABLED][inLightIndex].SetValue((DWORD)inEnabled); //?
}
void D3DStateManager::SetPointLight(int inLightIndex, const SexyVector3& inPos, const Graphics3D::LightColors& inColors, float inRange, const SexyVector3& inAttenuation) //572-582
{
	mLightStates[ST_LIGHT_TYPE][inLightIndex].SetValue(DWORD(SG_TSS)); //?
	mLightStates[ST_LIGHT_DIFFUSE][inLightIndex].SetValue(MakeLightColorStateValue(inColors.mDiffuse, inColors.mAutoScale));
	mLightStates[ST_LIGHT_SPECULAR][inLightIndex].SetValue(MakeLightColorStateValue(inColors.mSpecular, inColors.mAutoScale));
	mLightStates[ST_LIGHT_AMBIENT][inLightIndex].SetValue(MakeLightColorStateValue(inColors.mAmbient, inColors.mAutoScale));
	mLightStates[ST_LIGHT_POSITION][inLightIndex].SetValue(StateValue(inPos.x, inPos.y, inPos.z, 0.0));
	mLightStates[ST_LIGHT_RANGE][inLightIndex].SetValue(inRange);
	mLightStates[ST_LIGHT_FALLOFF][inLightIndex].SetValue(1.0f);
	mLightStates[ST_LIGHT_ATTENUATION][inLightIndex].SetValue(StateValue(inAttenuation.x, inAttenuation.y, inAttenuation.z, 0.0));
	mLightStates[ST_LIGHT_ANGLES][inLightIndex].SetValue(0.0, SEXYMATH_PI, 0.0, 0.0);
}
void D3DStateManager::SetDirectionalLight(int inLightIndex, const SexyVector3& inDir, const Graphics3D::LightColors& inColors) //584-594
{
	mLightStates[ST_LIGHT_TYPE][inLightIndex].SetValue(DWORD(SG_LIGHT));
	mLightStates[ST_LIGHT_DIFFUSE][inLightIndex].SetValue(MakeLightColorStateValue(inColors.mDiffuse, inColors.mAutoScale));
	mLightStates[ST_LIGHT_SPECULAR][inLightIndex].SetValue(MakeLightColorStateValue(inColors.mSpecular, inColors.mAutoScale));
	mLightStates[ST_LIGHT_AMBIENT][inLightIndex].SetValue(MakeLightColorStateValue(inColors.mAmbient, inColors.mAutoScale));
	mLightStates[ST_LIGHT_DIRECTION][inLightIndex].SetValue(StateValue(inDir.x, inDir.y, inDir.z, 0.0));
	mLightStates[ST_LIGHT_RANGE][inLightIndex].SetValue(sqrtf(SEXYMATH_PI) - 1.0f);
	mLightStates[ST_LIGHT_FALLOFF][inLightIndex].SetValue(1.0f);
	mLightStates[ST_LIGHT_ATTENUATION][inLightIndex].SetValue(StateValue(0.0, 1.0, 0.0, 0.0));
	mLightStates[ST_LIGHT_ANGLES][inLightIndex].SetValue(0.0, SEXYMATH_PI, 0.0, 0.0);
}
void D3DStateManager::SetMaterialAmbient(const Color& inColor, int inVertexColorComponent) //596-599
{
	mMaterialStates[ST_MAT_AMBIENT].SetValue(MakeLightColorStateValue(inColor, 1.0));
	mRenderStates[147].SetValue(inVertexColorComponent < 0 ? 0 : (DWORD)inVertexColorComponent++);
}
void D3DStateManager::SetMaterialDiffuse(const Color& inColor, int inVertexColorComponent) //601-604
{
	mMaterialStates[ST_MAT_DIFFUSE].SetValue(MakeLightColorStateValue(inColor, 1.0));
	mRenderStates[145].SetValue(inVertexColorComponent < 0 ? 0 : (DWORD)inVertexColorComponent++);
}
void D3DStateManager::SetMaterialSpecular(const Color& inColor, int inVertexColorComponent, float inPower) //606-611
{
	mMaterialStates[ST_MAT_SPECULAR].SetValue(MakeLightColorStateValue(inColor, 1.0));
	mMaterialStates[ST_MAT_POWER].SetValue(inPower);
	mRenderStates[146].SetValue(inVertexColorComponent < 0 ? 0 : (DWORD)inVertexColorComponent++);
	mMaterialStates[29].SetValue((DWORD)(inPower > 0.0));
}
void D3DStateManager::SetMaterialEmissive(const Color& inColor, int inVertexColorComponent) //613-616
{
	mMaterialStates[ST_MAT_EMISSIVE].SetValue(MakeLightColorStateValue(inColor, 1.0));
	mRenderStates[148].SetValue(inVertexColorComponent < 0 ? 0 : (DWORD)inVertexColorComponent++);
}

void D3DStateManager::SetStreamSource(ulong inStreamIndex, void* inVertexBuffer, ulong inOffset, ulong inStride, ulong inFreq) //619-624
{
	mStreamStates[ST_STREAM_DATA][inStreamIndex].SetValue(inVertexBuffer);
	mStreamStates[ST_STREAM_OFFSET][inStreamIndex].SetValue(inOffset);
	mStreamStates[ST_STREAM_STRIDE][inStreamIndex].SetValue(inStride);
	mStreamStates[ST_STREAM_FREQ][inStreamIndex].SetValue(inFreq);
}
void D3DStateManager::SetTransform(ulong inTS, const SexyMatrix4* inMatrix) //626-631
{
	mTransformStates[inTS][0].SetValue(inMatrix->m00, inMatrix->m01, inMatrix->m02, inMatrix->m03);
	mTransformStates[inTS][1].SetValue(inMatrix->m10, inMatrix->m11, inMatrix->m12, inMatrix->m13);
	mTransformStates[inTS][2].SetValue(inMatrix->m20, inMatrix->m21, inMatrix->m22, inMatrix->m23);
	mTransformStates[inTS][3].SetValue(inMatrix->m30, inMatrix->m31, inMatrix->m32, inMatrix->m33);
}
void D3DStateManager::SetViewport(ulong inX, ulong inY, ulong inWidth, ulong inHeight, float inMinZ, float inMaxZ) //633-640
{
	mViewportStates[ST_VIEWPORT_X].SetValue(inX);
	mViewportStates[ST_VIEWPORT_Y].SetValue(inY);
	mViewportStates[ST_VIEWPORT_WIDTH].SetValue(inWidth);
	mViewportStates[ST_VIEWPORT_HEIGHT].SetValue(inHeight);
	mViewportStates[ST_VIEWPORT_MINZ].SetValue(inMinZ);
	mViewportStates[ST_VIEWPORT_MAXZ].SetValue(inMaxZ);
}
void D3DStateManager::SetFVF(ulong inFVF) //642-648
{
	if (mMiscStates[ST_MISC_FVF][0].GetDword() != inFVF) //Not sure what the zero's are, guessing 0
	{
		mMiscStates[ST_MISC_FVF][0].SetValue(inFVF);
		mMiscStates[ST_MISC_FVFSIZE][0].SetValue(MakeFVFSize(inFVF));
	}
}
void D3DStateManager::SetIndices(void* inIndexBuffer) //650-652
{
	mMiscStates[ST_MISC_INDICES][0].SetValue(inIndexBuffer);
}
void D3DStateManager::SetPixelShader(void* inShader) //654-656
{
	mMiscStates[ST_MISC_PIXELSHADER][0].SetValue(inShader);
}
void D3DStateManager::SetVertexShader(void* inShader) //658-660
{
	mMiscStates[ST_MISC_VERTEXSHADER][0].SetValue(inShader);
}
void D3DStateManager::SetCurrentTexturePalette(ulong inPaletteIndex) //662-664
{
	mMiscStates[ST_MISC_TEXTUREPALETTE][0].SetValue(inPaletteIndex);
}
void D3DStateManager::SetScissorRect(RECT inRect) //666-668
{
	mMiscStates[ST_MISC_SCISSORRECT][0].SetValue(StateValue((float)inRect.bottom, (float)inRect.right, (float)inRect.top, (float)inRect.left));
}
void D3DStateManager::SetNPatchMode(float inSegments) //670-672
{
	mMiscStates[ST_MISC_NPATCHMODE][0].SetValue(inSegments);
}
void D3DStateManager::SetTexture(ulong inSampler, void* inTexture) //674-676
{
	mMiscStates[ST_MISC_TEXTURE][inSampler].SetValue(inTexture);
}
void D3DStateManager::SetTextureRemap(ulong inLogicalSampler, ulong inPhysicalSampler) //678-684
{
	if (mMiscStates[ST_MISC_TEXTUREREMAP][inLogicalSampler].GetDword() != inPhysicalSampler)
	{
		mMiscStates[ST_MISC_TEXTUREREMAP][inLogicalSampler].SetValue(inPhysicalSampler);
		mMiscStates[ST_MISC_TEXTURE][inLogicalSampler].SetDirty();
	}
}
void D3DStateManager::SetPixelShaderConstantF(ulong inStartRegister, const float* inConstantData, ulong inVector4fCount) //686-692 (UNMATCHING LINES)
{
	const float* f = inConstantData;
	for (int i = 0; i < inVector4fCount; i++)
	{
		mMiscStates[ST_MISC_PIXELSHADERCONST][i + inStartRegister].SetValue(*f, f[1], f[2], f[3]);
		f += 4;
	}
}
void D3DStateManager::SetVertexShaderConstantF(ulong inStartRegister, const float* inConstantData, ulong inVector4fCount) //694-700 (UNMATCHING LINES)
{
	const float* f = inConstantData;
	for (int i = 0; i < inVector4fCount; i++)
	{
		mMiscStates[ST_MISC_VERTEXSHADERCONST][i + inStartRegister].SetValue(*f, f[1], f[2], f[3]);
		f += 4;
	}
}
void D3DStateManager::SetClipPlane(ulong inIndex, const float* inPlane) //702-704
{
	mMiscStates[ST_MISC_CLIPPLANE][inIndex].SetValue(*inPlane, inPlane[1], inPlane[2], inPlane[3]);
}
void D3DStateManager::SetBlendOverride(Graphics3D::EBlendMode inSrcBlend, Graphics3D::EBlendMode inDestBlend) //706-709
{
	mMiscStates[ST_MISC_SRCBLENDOVERRIDE][0].SetValue((DWORD)inSrcBlend);
	mMiscStates[ST_MISC_DESTBLENDOVERRIDE][0].SetValue((DWORD)inDestBlend);
}
void D3DStateManager::SetBltDepth(float inDepth) //711-713
{
	mMiscStates[ST_MISC_BLTDEPTH][0].SetValue(inDepth);
}

ulong D3DStateManager::GetRenderState(ulong inRS) const //716-718
{
	return mRenderStates[inRS].GetDword(); //Needs a cast?
}
ulong D3DStateManager::GetTextureStageState(ulong inStage, ulong inTSS) const //720-722
{
	return mTextureStageStates[inTSS][inStage].GetDword(); //Needs a cast?
}
ulong D3DStateManager::GetSamplerState(ulong inSampler, ulong inSS) const //724-726
{
	return mSamplerStates[inSS][inSampler].GetDword(); //Needs a cast?
}
void D3DStateManager::GetTransform(ulong inTS, SexyMatrix4* inMatrix) const //728-733
{
	return mTransformStates[inTS][0].GetVector(inMatrix->m00, inMatrix->m01, inMatrix->m02, inMatrix->m03); //Needs a cast?
	return mTransformStates[inTS][1].GetVector(inMatrix->m01, inMatrix->m11, inMatrix->m12, inMatrix->m13); //Needs a cast?
	return mTransformStates[inTS][2].GetVector(inMatrix->m02, inMatrix->m21, inMatrix->m22, inMatrix->m23); //Needs a cast?
	return mTransformStates[inTS][3].GetVector(inMatrix->m03, inMatrix->m31, inMatrix->m32, inMatrix->m33); //Needs a cast?
}
ulong D3DStateManager::GetFVF() const //735-737
{
	return mMiscStates[ST_MISC_FVF][0].GetDword(); //Needs a cast?
}
ulong D3DStateManager::GetFVFSize() const //739-741
{
	return mMiscStates[ST_MISC_FVFSIZE][0].GetDword(); //Needs a cast?
}
void* D3DStateManager::GetTexture(ulong inSampler) const //743-745
{
	return mMiscStates[ST_MISC_TEXTURE][inSampler].GetPtr(); //Needs a cast?
}
ulong D3DStateManager::GetTextureRemap(ulong inLogicalSampler) const //747-749
{
	return mMiscStates[ST_MISC_TEXTUREREMAP][inLogicalSampler].GetDword(); //Needs a cast?
}
void D3DStateManager::GetLightInfo(int inLightIndex, Graphics3D::LightColors& outColors, SexyVector3& outPos, SexyVector3& outDir, SexyVector3& outAttenuation, float& outRange) //751-760
{
	float ignored; //?
	outColors.mDiffuse = MakeStateValueLightColor(mLightStates[ST_LIGHT_DIFFUSE][inLightIndex]);
	outColors.mSpecular = MakeStateValueLightColor(mLightStates[ST_LIGHT_SPECULAR][inLightIndex]);
	outColors.mAmbient = MakeStateValueLightColor(mLightStates[ST_LIGHT_AMBIENT][inLightIndex]);
	mLightStates[ST_LIGHT_POSITION][inLightIndex].GetVector(outPos.x, outPos.y, outPos.z, ignored);
	mLightStates[ST_LIGHT_DIRECTION][inLightIndex].GetVector(outDir.x, outDir.y, outDir.z, ignored);
	mLightStates[ST_LIGHT_ATTENUATION][inLightIndex].GetVector(outAttenuation.x, outAttenuation.y, outAttenuation.z, ignored);
	outRange = mLightStates[ST_LIGHT_RANGE][inLightIndex].GetFloat();
}
void D3DStateManager::GetMaterialInfo(Color& outAmbient, Color& outDiffuse, Color&, Color& outSpecular, Color& outEmissive, float& outSpecularPower) //762-768
{
	outAmbient = MakeStateValueLightColor(mMaterialStates[ST_MAT_AMBIENT]);
	outDiffuse = MakeStateValueLightColor(mMaterialStates[ST_MAT_DIFFUSE]);
	outSpecular = MakeStateValueLightColor(mMaterialStates[ST_MAT_SPECULAR]);
	outEmissive = MakeStateValueLightColor(mMaterialStates[ST_MAT_EMISSIVE]);
	outSpecularPower = mMaterialStates[ST_MAT_POWER].GetFloat();
}
void D3DStateManager::GetBlendOverride(Graphics3D::EBlendMode& outSrcBlend, Graphics3D::EBlendMode& outDestBlend) //770-773
{
	outSrcBlend = (Graphics3D::EBlendMode)mMiscStates[ST_MISC_SRCBLENDOVERRIDE][0].GetDword();
	outDestBlend = (Graphics3D::EBlendMode)mMiscStates[ST_MISC_DESTBLENDOVERRIDE][0].GetDword();
}
float D3DStateManager::GetBltDepth() //775-777
{
	return mMiscStates[ST_MISC_BLTDEPTH][0].GetFloat();
}
