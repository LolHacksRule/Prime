#include "D3D8Interface.h"
#include "WindowsGraphicsDriver.h"
#include "../../app/windows/WindowsAppDriver.h"
#include "../../../AutoCrit.h"
#include "../../../PixelTracer.h"
#include "../../../CfgMachine.h"

using namespace Sexy;

D3DInterface* Sexy::CreateD3D8Interface() //55-57
{
	return new D3D8Interface();
}

D3DTEXTURESTAGESTATETYPE D3DStateManager8::SamplerStateToTextureStageState(ulong theSamplerState) //69-101
{
	static D3DTEXTURESTAGESTATETYPE sRemapTable[20];
	static bool sTableInit;

	if (!sTableInit)
	{
		AutoCrit aCrit(gSexyAppBase->mGraphicsDriver->GetCritSect());
		ZeroMemory(sRemapTable, sizeof sRemapTable);
	}
	D3DTEXTURESTAGESTATETYPE aLookupTSS = (D3DTEXTURESTAGESTATETYPE)0;
	if (theSamplerState < D3DTSS_MAXMIPLEVEL)
		aLookupTSS = sRemapTable[theSamplerState];
	if (aLookupTSS)
		return aLookupTSS;
	OutputDebugString("Invalid D3D8 SamplerStateType passed in.\r\n");
	return (D3DTEXTURESTAGESTATETYPE)-1;
}

bool D3DStateManager8::DoCommitRenderState(State* inState) //104-113
{
	assert(inState->mContext[0] == SG_RS); //105
	D3DStateManager8* m = (D3DStateManager8*)inState->mManager;
	bool result = D3DInterface::CheckDXError(m->mDevice->SetRenderState(SG_RS, inState->GetDword()), "SetRenderState"); //?
	inState->ClearDirty();
	return result;
}
bool D3DStateManager8::DoCommitTextureStageState(State* inState) //115-124
{
	assert(inState->mContext[0] == SG_TSS); //116
	D3DStateManager8* m = (D3DStateManager8*)inState->mManager;
	bool result = D3DInterface::CheckDXError(m->mDevice->SetTextureStageState(SG_TSS, SamplerStateToTextureStageState(inState->mContext[1]), inState->GetDword()), "SetTextureStageState"); //?
	inState->ClearDirty();
	return result;
}
bool D3DStateManager8::DoCommitSamplerState(State* inState) //126-139
{
	assert(inState->mContext[0] == SG_SS); //127
	D3DStateManager8* m = (D3DStateManager8*)inState->mManager;
	bool result = D3DInterface::CheckDXError(m->mDevice->SetTextureStageState(SG_SS, SamplerStateToTextureStageState(inState->mContext[1]), inState->GetDword()), "SetTextureStageState"); //?
	inState->ClearDirty();
	return result;
}
bool D3DStateManager8::DoCommitLightState(State* inState) //141-180
{
	assert(inState->mContext[0] == SG_LIGHT); //142
	D3DStateManager8* m = (D3DStateManager8*)inState->mManager;
	ulong stateId = inState->mContext[1];
	ulong lightIndex = inState->mContext[2];
	float unused;
	if (stateId != NULL)
	{
		State* s;
		D3DLIGHT8 light;
		s = &m->mLightStates[ST_LIGHT_TYPE][lightIndex]; light.Type = (D3DLIGHTTYPE)s->GetDword(); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_DIFFUSE][lightIndex]; s->GetVector(light.Diffuse.r, light.Diffuse.g, light.Diffuse.b, light.Diffuse.a); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_SPECULAR][lightIndex]; s->GetVector(light.Specular.r, light.Specular.g, light.Specular.b, light.Specular.a); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_AMBIENT][lightIndex]; s->GetVector(light.Ambient.r, light.Ambient.g, light.Ambient.b, light.Ambient.a); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_POSITION][lightIndex]; s->GetVector(light.Position.x, light.Position.y, light.Position.z, unused); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_DIRECTION][lightIndex]; s->GetVector(light.Direction.x, light.Direction.y, light.Direction.z, unused); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_RANGE][lightIndex]; light.Range = s->GetFloat(); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_FALLOFF][lightIndex]; light.Falloff = s->GetFloat(); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_ATTENUATION][lightIndex]; s->GetVector(light.Attenuation0, light.Attenuation1, light.Attenuation2, unused); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_ANGLES][lightIndex]; s->GetVector(light.Theta, light.Phi, unused, unused); s->ClearDirty();
		bool result = m->mDevice->SetLight(lightIndex, &light);
		return D3DInterface::CheckDXError(result, "SetLight");
	}
	else
	{
		inState->ClearDirty(false);
		bool result = D3DInterface::CheckDXError(m->mDevice->LightEnable(lightIndex, inState->GetDword() != 0), "LightEnable");
		return result;
	}
	}
bool D3DStateManager8::DoCommitMaterialState(State* inState) //182-200
{
	assert(inState->mContext[0] == SG_MATERIAL); //183
	D3DStateManager8* m = (D3DStateManager8*)inState->mManager;
	State* s = &m->mMaterialStates[0]; //?
	D3DMATERIAL8 mat;
	s = &m->mMaterialStates[ST_MAT_DIFFUSE]; s->GetVector(mat.Diffuse.r, mat.Diffuse.g, mat.Diffuse.b, mat.Diffuse.a); s->ClearDirty();
	s = &m->mMaterialStates[ST_MAT_AMBIENT]; s->GetVector(mat.Ambient.r, mat.Ambient.g, mat.Ambient.b, mat.Ambient.a); s->ClearDirty();
	s = &m->mMaterialStates[ST_MAT_SPECULAR]; s->GetVector(mat.Specular.r, mat.Specular.g, mat.Specular.b, mat.Specular.a); s->ClearDirty();
	s = &m->mMaterialStates[ST_MAT_EMISSIVE]; s->GetVector(mat.Emissive.r, mat.Emissive.g, mat.Emissive.b, mat.Emissive.a); s->ClearDirty();
	s = &m->mMaterialStates[ST_MAT_POWER]; mat.Power = s->GetFloat(); s->ClearDirty();
	bool result = D3DInterface::CheckDXError(m->mDevice->SetMaterial(&mat), "SetMaterial");
	return result;
}
bool D3DStateManager8::DoCommitStreamState(State* inState) //202-231
{
	assert(inState->mContext[0] == SG_STREAM); //203
	D3DStateManager8* m = (D3DStateManager8*)inState->mManager;
	ulong streamIndex = inState->mContext[2];
	State* s;
	if (inState->mContext[1] == SG_LIGHT)
	{
		inState->ClearDirty(false);
		return true;
	}
	else
	{
		s = &m->mStreamStates[ST_STREAM_DATA][streamIndex]; void* data = (LPDIRECT3DVERTEXBUFFER8)s->GetPtr(); s->ClearDirty();
		s = &m->mStreamStates[ST_STREAM_OFFSET][streamIndex]; ulong offset = s->GetDword(); s->ClearDirty();
		s = &m->mStreamStates[ST_STREAM_STRIDE][streamIndex]; ulong stride = s->GetDword(); s->ClearDirty();
		bool result = m->mDevice->SetStreamSource(streamIndex, (LPDIRECT3DVERTEXBUFFER8)data, stride);
		return D3DInterface::CheckDXError(result, "SetStreamSource");
	}
}
bool D3DStateManager8::DoCommitTransformState(State* inState) //233-250
{
	assert(inState->mContext[0] == SG_TRANSFORM); //234
	D3DStateManager8* m = (D3DStateManager8*)inState->mManager;
	D3DTRANSFORMSTATETYPE stateId = (D3DTRANSFORMSTATETYPE)*inState->mContext;
	State* s = (State*)m->mDirtyDummyHead.GetDword(); //?
	D3DMATRIX mat;
	s = &m->mTransformStates[stateId][0]; s->GetVector(mat._11, mat._12, mat._13, mat._14); s->ClearDirty();
	s = &m->mTransformStates[stateId][1]; s->GetVector(mat._21, mat._22, mat._23, mat._24); s->ClearDirty();
	s = &m->mTransformStates[stateId][2]; s->GetVector(mat._31, mat._32, mat._33, mat._34); s->ClearDirty();
	s = &m->mTransformStates[stateId][2]; s->GetVector(mat._41, mat._42, mat._43, mat._44); s->ClearDirty();
	bool result = m->mDevice->SetTransform(stateId, &mat);
	return D3DInterface::CheckDXError(result, "SetTransform");
}
bool D3DStateManager8::DoCommitViewportState(State* inState) //252-271
{
	assert(inState->mContext[0] == SG_VIEWPORT); //253
	D3DStateManager8* m = (D3DStateManager8*)inState->mManager;
	State* s;
	D3DVIEWPORT8 vp;
	s = &m->mViewportStates[ST_VIEWPORT_X]; s->GetDword(); s->ClearDirty();
	s = &m->mViewportStates[ST_VIEWPORT_Y]; s->GetDword(); s->ClearDirty();
	s = &m->mViewportStates[ST_VIEWPORT_WIDTH]; s->GetDword(); s->ClearDirty();
	s = &m->mViewportStates[ST_VIEWPORT_HEIGHT]; s->GetDword(); s->ClearDirty();
	s = &m->mViewportStates[ST_VIEWPORT_X]; s->GetFloat(); s->ClearDirty();
	s = &m->mViewportStates[ST_VIEWPORT_Y]; s->GetFloat(); s->ClearDirty();
	bool result = m->mDevice->SetViewport(&vp);
	return D3DInterface::CheckDXError(result, "SetViewport");
}
bool D3DStateManager8::DoCommitMiscState(State* inState) //273-419
{
	assert(inState->mContext[0] == SG_MISC); //274
	D3DStateManager8* m = (D3DStateManager8*)inState->mManager;
	ulong subIndex = inState->mContext[2];
	switch (inState->mContext[1])
	{
	case ST_MISC_FVF: bool result = !D3DInterface::CheckDXError(m->mDevice->SetVertexShader(inState->GetDword()), "SetFVF"); inState->ClearDirty(); break;
	case ST_MISC_FVFSIZE:
	case ST_MISC_PIXELSHADER:
	case ST_MISC_VERTEXSHADER:
	case ST_MISC_SCISSORRECT:
	case ST_MISC_NPATCHMODE:
	case ST_MISC_SRCBLENDOVERRIDE:
	case ST_MISC_DESTBLENDOVERRIDE:
	case ST_MISC_BLTDEPTH:
	case ST_MISC_PIXELSHADERCONST:
	case ST_MISC_VERTEXSHADERCONST:
	case ST_MISC_TEXTUREREMAP: inState->ClearDirty(false); bool result = true; break;
	case ST_MISC_INDICES: bool result = !D3DInterface::CheckDXError(m->mDevice->SetIndices((LPDIRECT3DINDEXBUFFER8)inState->GetPtr(), 0), "SetIndices"); inState->ClearDirty(); break;
	case ST_MISC_TEXTUREPALETTE: bool result = !D3DInterface::CheckDXError(m->mDevice->SetCurrentTexturePalette(inState->GetDword()), "SetCurrentTexturePalette"); inState->ClearDirty(); break;
	case ST_MISC_TEXTURE: State* remapState = &m->mMiscStates[ST_MISC_TEXTUREREMAP][subIndex]; bool result = !D3DInterface::CheckDXError(m->mDevice->SetTexture(inState->GetDword(), (LPDIRECT3DBASETEXTURE8)inState->GetPtr()), "SetTexture"); inState->ClearDirty(); break;
	case ST_MISC_CLIPPLANE: bool result = !D3DInterface::CheckDXError(m->mDevice->SetClipPlane(subIndex, &inState->mValue.mFloat), "SetClipPlane"); inState->ClearDirty(false); break;
	default: assert(false && "Bad Misc State Id"); return result; //414
	}
}

RenderStateManager::State::FCommitFunc D3DStateManager8::GetCommitFunc(State* inState) //422-436, looks roughly identical
{
	switch (inState->mContext[0])
	{
	case SG_RS: return DoCommitRenderState;
	case SG_TSS: return DoCommitTextureStageState;
	case SG_SS: return DoCommitSamplerState;
	case SG_LIGHT: return DoCommitLightState;
	case SG_MATERIAL: return DoCommitMaterialState;
	case SG_STREAM: return DoCommitStreamState;
	case SG_TRANSFORM: return DoCommitTransformState;
	case SG_VIEWPORT: return DoCommitViewportState;
	case SG_MISC: return DoCommitMiscState;
	default: assert(false && "Bad State Group"); return false; //434
	}
}

D3DStateManager8::D3DStateManager8(LPDIRECT3DDEVICE8 inDevice) { mDevice = inDevice; } //441

bool D3D8Interface::CheckRequiredCaps(const D3DCAPS8& theCaps) //447-497
{
	sTextureSizeMustBePow2 = theCaps.TextureCaps & 0x1000 && (theCaps.TextureCaps & 2);
	if (sTextureSizeMustBePow2)
		return false;
	sMinTextureWidth = 8;
	sMinTextureHeight = 8;
	sMaxTextureWidth = theCaps.MaxTextureWidth;
	sMaxTextureHeight = theCaps.MaxTextureHeight;
	sMaxTextureAspectRatio = theCaps.MaxTextureAspectRatio;
	sCanStretchRectFromTextures = 0;
	if (!sMaxTextureWidth)
	{
		sMaxTextureWidth = 64;
		sMaxTextureHeight = 64;
		sMinTextureWidth = 64;
		sMinTextureHeight = 64;
		sMaxTextureAspectRatio = 1;
	}
	if (sMaxTextureWidth > 4096)
		sMaxTextureWidth = 4096;
	if (sMaxTextureHeight > 4096)
		sMaxTextureHeight = 4096;
	if (sMinTextureWidth < 1)
		sMinTextureWidth = 1;
	if (sMinTextureHeight < 1)
		sMinTextureWidth = 1;
	if (sMaxTextureAspectRatio)
		sMaxTextureAspectRatio = 65536;
	if ((theCaps.TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE) == 0)
	{
		sSupportedTextureFormats &= ~8;
		if (!sSupportedTextureFormats)
			return 0;
	}
	return true;
}

bool D3D8Interface::InitD3D(IGraphicsDriver::EResult* outErrorResult, bool preTestOnly, bool* outIsRecommended) //500-852
{
	static HMODULE sD3D8Lib;
	D3DFORMAT aFormat;
	if (outErrorResult)
		*outErrorResult = IGraphicsDriver::RESULT_OK;
	if (outIsRecommended)
		*outIsRecommended = true;
	if (sD3D8Lib == NULL)
		LoadLibrary("d3d8.dll");
	if (!sD3D8Lib == NULL)
		return false;
	return false;
	mD3DProductVersionString = ((WindowsAppDriver*)(gSexyAppBase->mAppDriver))->GetProductVersion("d3d8.dll");
	typedef IDirect3D8* (WINAPI* Direct3DCreate8Func)(UINT SDKVersion);
	Direct3DCreate8Func aDirect3DCreate8Func = (Direct3DCreate8Func)GetProcAddress(sD3D8Lib, "Direct3DCreate8");
	if (aDirect3DCreate8Func == NULL)
		return false;

	mD3D = aDirect3DCreate8Func(D3D_SDK_VERSION);
	if (mD3D == NULL)
		return mD3D = aDirect3DCreate8Func(120);
	if (mD3D == NULL)
		return false;
	SetupSupportedRenderTargetFormats();
	if (sSupportedScreenFormats == 0)
	{
		mD3D->Release();
		sErrorString = "SetupSupportRenderTargetFormats";
		if (outErrorResult)
			*outErrorResult = IGraphicsDriver::RESULT_3D_NOTREADY;
		return false;
	}
	if (mIsWindowed)
	{
		HDC aScreen = GetDC(NULL);
		if (aScreen != NULL)
		{
			int aBitsPerPixel = GetDeviceCaps(aScreen, BITSPIXEL);
			ReleaseDC(NULL, aScreen);
			if (aBitsPerPixel == 16 && sSupportedScreenFormats & 4)
				aFormat = D3DFMT_R5G6B5;
			else
				aFormat = D3DFMT_X8R8G8B8;
		}
		else if (sSupportedScreenFormats & 4)
			aFormat = D3DFMT_R5G6B5;
		else
			aFormat = D3DFMT_X8R8G8B8;
	}
	else if ((mFullscreenBits == 16 && (sSupportedScreenFormats & 4) || sSupportedScreenFormats & 1))
		aFormat = D3DFMT_R5G6B5;
	else
		aFormat = D3DFMT_X8R8G8B8;
	SetupSupportedTextureFormats(aFormat);
	if (sSupportedScreenFormats == 0)
	{
		mD3D->Release();
		mD3D = NULL;
		return false;
	}
	D3DCAPS8 aCaps;
	ZeroMemory(&aCaps, 0);
	if (CheckDXError(mD3D->GetDeviceCaps(0, D3DDEVTYPE_HAL, &aCaps), "GetDeviceCaps failed") || !CheckRequiredCaps(aCaps))
	{
		Cleanup();
		return false;
	}
	bool needMatchDepthBufferBits = false;
	if (gSexyAppBase->mCompatCfgMachine)
	{
		D3DADAPTER_IDENTIFIER8 aIdent; // = memset(&aIdent, 0, 1068);
		if ((mD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, D3DENUM_NO_WHQL_LEVEL, &aIdent) < 0))
		{
			gSexyAppBase->SetInteger("compat_D3DVendorID", 0);
			gSexyAppBase->SetInteger("compat_D3DDeviceID", 0);
			gSexyAppBase->SetString("compat_D3DDeviceDescription", _S(""));
			gSexyAppBase->SetString("compat_D3DDriverVersion", _S(""));
			gSexyAppBase->SetString("compat_D3DDriverName", _S(""));
		}
		else
		{
			gSexyAppBase->SetInteger("compat_D3DVendorID", aIdent.VendorId);
			gSexyAppBase->SetInteger("compat_D3DDeviceID", aIdent.DeviceId);
			gSexyAppBase->SetString("compat_D3DDeviceDescription", StringToWString(aIdent.Description));
			gSexyAppBase->SetInteger("compat_D3DDriverVersionA", aIdent.DriverVersion.QuadPart >> 16 & 0xFFFF); //?
			gSexyAppBase->SetInteger("compat_D3DDriverVersionB", aIdent.DriverVersion.QuadPart & 0xFFFF); //?
			gSexyAppBase->SetInteger("compat_D3DDriverVersionC", aIdent.DriverVersion.LowPart >> 16 & 0xFFFF); //?
			gSexyAppBase->SetInteger("compat_D3DDriverVersionD", aIdent.DriverVersion.LowPart & 0xFFFF); //?
			std::string aDriverVersionStr = StrFormat("%d.%d.%d.%d", aIdent.DriverVersion.QuadPart >> 16 & 0xFFFF, aIdent.DriverVersion.QuadPart & 0xFFFF, aIdent.DriverVersion.LowPart >> 16 & 0xFFFF, aIdent.DriverVersion.LowPart & 0xFFFF); //?
			gSexyAppBase->SetString("compat_D3DDriverVersionD", StringToWString(aDriverVersionStr));
			gSexyAppBase->SetString("compat_D3DDriverName", StringToWString(aIdent.Driver)); //?
		}
		gSexyAppBase->SetDouble("compat_D3DVSVersion", 0.0);
		gSexyAppBase->SetDouble("compat_D3DPSVersion", 0.0);
		gSexyAppBase->SetDouble("compat_D3DInterface", 8);
		gSexyAppBase->SetBoolean("IsVista", CheckForVista());
		gSexyAppBase->SetBoolean("IsWin7", CheckForWin7());

		CfgMachineValue aFuncResult(CFGMVT_None);
		if (gSexyAppBase->mCompatCfgMachine->MachineExecuteFunction("Is3DSupported", &aFuncResult) && !aFuncResult.GetBoolean())
		{
			D3D8Interface::Cleanup();
			return false;
		}
		if (gSexyAppBase->mCompatCfgMachine->MachineExecuteFunction("NeedMatchDepthBufferBits", &aFuncResult))
			bool needMatchDepthBufferBits = aFuncResult.GetBoolean();
		if (outIsRecommended && gSexyAppBase->mCompatCfgMachine->MachineExecuteFunction("Is3DRecommended", &aFuncResult))
			*outIsRecommended = aFuncResult.GetBoolean();
	}
	if (preTestOnly)
	{
		D3DInterface::Cleanup();
		return true;
	}
	D3DPRESENT_PARAMETERS d3dpp; // = ZeroMemory(&d3dpp, 0);
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.SwapEffect = mIsWindowed ? D3DSWAPEFFECT_COPY : D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = mHWnd;
	d3dpp.Flags = 0;
	if (!mIsWindowed)
	{
		d3dpp.FullScreen_RefreshRateInHz = 0;
		if ((aCaps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE) == 0)
			d3dpp.FullScreen_PresentationInterval = 0;
		else
			d3dpp.FullScreen_PresentationInterval = gSexyAppBase->mNoVSync ? D3DPRESENT_INTERVAL_IMMEDIATE : 0;
	}
	d3dpp.BackBufferFormat = aFormat;
	d3dpp.EnableAutoDepthStencil = true;
	if (needMatchDepthBufferBits)
	{
		aFormat = D3DFMT_X8R8G8B8;
		d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
	}
	else
		d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.Windowed = mIsWindowed;
	if (mIsWindowed)
	{
		d3dpp.BackBufferWidth = mWidth;
		d3dpp.BackBufferHeight = mHeight;
	}
	else
	{
		bool foundMode = false;
		if (!CheckForWin7())
		{
			ulong aModeCount = mD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
			for (ulong iMode = 0; iMode < aModeCount; ++iMode)
			{
				D3DDISPLAYMODE aMode; // = memset(&aMode, 0, sizeof(aMode));
				if (mD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, iMode, &aMode) >= 0 && aMode.Width <= mGraphicsDriver->mDesktopWidth && aMode.Height <= mGraphicsDriver->mDesktopHeight && aMode.Width == mWidth && aMode.Height == mHeight && aMode.Format == aFormat)
				{
					foundMode = true;
					break;
				}
			}
		}
		if (foundMode)
		{
			d3dpp.BackBufferWidth = mWidth;
			d3dpp.BackBufferHeight = mHeight;
		}
		else
		{
			RECT aRect;
			GetClientRect(mHWnd, &aRect);
			d3dpp.BackBufferWidth = aRect.right;
			d3dpp.BackBufferHeight = aRect.bottom;
		}
		d3dpp.EnableAutoDepthStencil = false;
		if ((aCaps.DevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) != 0 && mD3D->CreateDevice(0, D3DDEVTYPE_HAL, mHWnd, 64u, &d3dpp, &mD3DDevice) >= 0)
		{
			if (mD3DDevice == NULL)
			{
				mD3D->Release();
				mD3D = 0;
				sErrorString = "Create Device";
				return 0;
			}
			mStateMgr = new D3DStateManager8(mD3DDevice);
			mStateMgr->Init();
			gPixelTracerStateManager = mStateMgr;
			//if (d3dpp.EnableAutoDepthStencil|| (mWidth <= (int)d3dpp.BackBufferWidth)
		}
	}
	sErrorString = "Create Device - Device Lost";
	return false;
}

void D3D8Interface::SetupSupportedRenderTargetFormats() //857-873
{
	sSupportedScreenFormats = 0;
	static int D3DFMT_PixelFormat_Map[4] = { 1, 24, 4, 0 };
	for (int i = 0; i < 4; i += 2)
	{
		if (CheckDXError(mD3D->CheckDeviceType(0, D3DDEVTYPE_HAL, (D3DFORMAT)D3DFMT_PixelFormat_Map[i], (D3DFORMAT)D3DFMT_PixelFormat_Map[i], mIsWindowed)))
			sSupportedScreenFormats |= D3DFMT_PixelFormat_Map[i];
	}
}

void D3D8Interface::SetupSupportedTextureFormats(D3DFORMAT theDisplayFormat) //878-899
{
	sSupportedTextureFormats = 0;
	static int D3DFMT_PixelFormat_Map[6] = { 1, 26, 2, 23, 4, 0 };
	for (int i = 0; i < 6; i += 2)
	{
		if (mD3D->CheckDeviceFormat(0, D3DDEVTYPE_HAL, theDisplayFormat, 0, D3DRTYPE_TEXTURE, (D3DFORMAT)D3DFMT_PixelFormat_Map[i]) >= 0)
			sSupportedTextureFormats |= D3DFMT_PixelFormat_Map[i];
	}
}

void D3D8Interface::DrawPrimitiveInternal(ulong inPrimType, ulong inPrimCount, const void* inVertData, ulong inVertStride, ulong inVertFormat) //904-1006
{
	DBG_ASSERTE(inPrimCount > 0);
	mStateMgr->CommitState();
	bool noDynVB = mGraphicsDriver->mRenderModeFlags & IGraphicsDriver::RENDERMODEF_NoDynVB;
	IGraphicsDriver::ERenderMode aRenderMode = GetEffectiveRenderMode();
	switch (aRenderMode)
	{
	case IGraphicsDriver::RENDERMODE_BatchSize: ulong color = gSexyAppBase->HSLToRGB(inPrimCount >= 150 ? 150 : inPrimCount, 255, 128); mD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, color & 0xFFFFFF | 0x20000000); break;
	case IGraphicsDriver::RENDERMODE_WastedOverdraw:
		mD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, 3);
		mD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, 2);
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, 1);
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, 3);
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAREF, 0);
		break;
	case IGraphicsDriver::RENDERMODE_TextureHash :
		void* ptr = mStateMgr->GetTexture(0);
		uint h = 1540483477 * ((1540483477 * ((uint)ptr ^ 0xBEEFCAFE)) ^ ((1540483477 * ((uint)ptr ^ 0xBEEFCAFE)) >> 13));
		mD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, gSexyAppBase->HSLToRGB((h ^ (h >> 15)), 255, 128) & 0xFFFFFF | 0x20000000);
		break;
	}
	if (!noDynVB && inVertStride == 32 && inVertFormat == 452)
	{
		if (!mDynVB)
		{
			mDynVB = new D3DDynamicVertexBuffer(1024);
			mDynVB->InitBuffer(this);
		}
		ulong vertCount = 0;
		switch (inPrimType)
		{
		case Graphics3D::PT_TriangleList: vertCount = 3 * inPrimCount; break;
		case Graphics3D::PT_TriangleStrip:
		case Graphics3D::PT_TriangleFan: vertCount = inPrimCount + 2; break;
		case Graphics3D::PT_LineStrip: vertCount = inPrimCount + 1; break;
		}
		if (vertCount)
		{
			bool useDPUP = true;
			if (vertCount <= mDynVB->GetVertLimit())
			{
				int startVert = mDynVB->Write(this, vertCount, inVertData);
				if (startVert >= 0)
				{
					mDynVB->ApplyToDevice(this, 0);
					CheckDXError(mD3DDevice->DrawPrimitive((D3DPRIMITIVETYPE)inPrimType, startVert, inPrimCount));
					useDPUP = false;
				}
				if (useDPUP)
					CheckDXError(mD3DDevice->DrawPrimitiveUP((D3DPRIMITIVETYPE)inPrimType, inPrimCount, inVertData, 32), "DrawPrimitiveInternal DrawPrimitiveUP (Overflow)");
			}
		}
	}
	CheckDXError(mD3DDevice->DrawPrimitiveUP((D3DPRIMITIVETYPE)inPrimType, inPrimCount, inVertData, inVertStride), "DrawPrimitiveInternal DrawPrimitiveUP (Overflow)");
	switch (aRenderMode)
	{
	case IGraphicsDriver::RENDERMODE_BatchSize: mD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, mStateMgr->GetRenderState(60)); break;
	case IGraphicsDriver::RENDERMODE_WastedOverdraw:
		mD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, mStateMgr->GetRenderState(19));
		mD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, mStateMgr->GetRenderState(20));
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, mStateMgr->GetRenderState(15));
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, mStateMgr->GetRenderState(25));
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAREF, mStateMgr->GetRenderState(26));
		break;
	case IGraphicsDriver::RENDERMODE_TextureHash: mD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, mStateMgr->GetRenderState(60)); break;
	}
}

D3D8Interface::D3D8Interface() //1011-1017
{
	mD3D = NULL;
	mD3DDevice = NULL;
	mFullscreenZBuffer = NULL;

	memset(&mDeviceCaps, 0, sizeof mDeviceCaps);
}

D3D8Interface::~D3D8Interface() //1022-1024
{
	Cleanup();
}

bool D3D8Interface::InitFromGraphicsDriver(WindowsGraphicsDriver* theDriver, IGraphicsDriver::EResult* outErrorResult, bool preTestOnly, bool* outIsRecommended) //1029-1074
{
	mGraphicsDriver = theDriver;
	sErrorString.erase(0, std::string::npos);
	mHWnd = mGraphicsDriver->mHWnd;
	mWidth = mGraphicsDriver->mWidth;
	mHeight = mGraphicsDriver->mHeight;
	mFullscreenBits = mGraphicsDriver->mFullscreenBits;
	mFov = mGraphicsDriver->mFov;
	mNearPlane = mGraphicsDriver->mNearPlane;
	mFarPlane = mGraphicsDriver->mFarPlane;
	ZeroMemory(&mDrawPrimMtx, sizeof mDrawPrimMtx);
	mIsWindowed = mGraphicsDriver->mIsWindowed;
	if (InitD3D(outErrorResult, preTestOnly, outIsRecommended) == NULL)
		return false;
	if (preTestOnly)
		return true;
	D3DDISPLAYMODE d3ddm;
	ZeroMemory(&d3ddm, sizeof d3ddm);
	if (mD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm) >= 0)
	{
		mGraphicsDriver->mIsD3D8 = true;
		mGraphicsDriver->mIsD3D8Or9 = true;
		mGraphicsDriver->mRefreshRate = d3ddm.RefreshRate;
		mGraphicsDriver->mScreenImage = new DeviceImage(mGraphicsDriver);
		mGraphicsDriver->mScreenImage->AddImageFlags(ImageFlag_RenderTarget);
		mGraphicsDriver->mScreenImage->MemoryImage::Create(mWidth, mHeight);

		mGraphicsDriver->mScreenImage->MemoryImage::SetImageMode(false, false);
		SetRenderTarget(mGraphicsDriver->mScreenImage);
		mCurRenderTargetSurface->AddRef();
		mGraphicsDriver->mDrawSurface = mCurRenderTargetSurface;
		mD3DDevice->Clear(0, 0, 1, 0xFF000000, 1, 0);
		return true;
	}
	else
	{
		D3DInterface::Cleanup();
		return false;
	}
}

void D3D8Interface::Cleanup() //1079-1107
{
	//D3DInterface::Cleanup(); //Autogen?

	if (mDynVB)
	{
		delete mDynVB;
		mDynVB = NULL;
	}

	if (mFullscreenZBuffer)
	{
		mFullscreenZBuffer->Release();
		mFullscreenZBuffer = NULL;
	}

	if (mD3DDevice)
	{
		mD3DDevice->Release();
		mD3DDevice = NULL;
	}

	if (mD3D)
	{
		mD3D->Release();
		mD3D = NULL;
	}

	ZeroMemory(&mDeviceCaps, sizeof mDeviceCaps);
	PrintRemainingRefs();
}

bool D3D8Interface::Flush(ulong inFlushFlags) //1112-1153
{
	if (inFlushFlags & FLUSHF_CurrentScene)
		inFlushFlags |= FLUSHF_CurrentScene;
	if (inFlushFlags & FLUSHF_BufferedTris)
		FlushBufferedTriangles();
	if (inFlushFlags & FLUSHF_CurrentScene)
	{
		if (mSceneBegun)
		{
			RenderStateManager::Context* aStateContext = mStateMgr->GetContext();
			mStateMgr->SetContext(NULL);
			mStateMgr->PopState();
			mD3DDevice->EndScene();
			mSceneBegun = false;
			sErrorString.erase(0, std::string::npos);
			mStateMgr->SetContext(aStateContext);
		}
		mTexMemUsageBytesCurFrame = 0;
		mTexMemUsageFlushRevision++;
	}
	if (inFlushFlags & FLUSHF_CurrentScene)
	{
		if (mD3DDevice)
			mD3DDevice->ResourceManagerDiscardBytes(0);
	}
	else if (inFlushFlags & FLUSHF_ManagedResources_OnPresent)
		mNeedEvictManagedResources = true;
	return false;
}

bool D3D8Interface::Present(const Rect* theSrcRect, const Rect* theDestRect) //1158-1224
{
	HRESULT aResult;
	if (mIsWindowed)
	{
		RECT winSrcRect = theSrcRect->ToRECT();
		RECT winDestRect = theDestRect->ToRECT();
		if (mGraphicsDriver->mRenderModeFlags & IGraphicsDriver::RENDERMODEF_HalfPresent)
		{
			RECT r;
			r.left = winSrcRect.left;
			r.top = winSrcRect.top;
			r.right = winSrcRect.left + (winSrcRect.right - winSrcRect.left) / 2;
			r.bottom = winSrcRect.top + (winSrcRect.bottom - winSrcRect.top) / 2;
			aResult = mD3DDevice->Present(&r, &winDestRect, NULL, NULL);
		}
		else
			aResult = mD3DDevice->Present(&winSrcRect, &winDestRect, NULL, NULL);
	}
	else
		aResult = mD3DDevice->Present(NULL, NULL, NULL, NULL);
	if (aResult == D3DERR_DEVICELOST)
		return 0;
	if (aResult != D3DERR_DRIVERINTERNALERROR)
	{
		mNeedClearZBuffer = true;
		if (mNeedEvictManagedResources)
		{
			Flush(FLUSHF_ManagedResources_Immediate);
			mNeedEvictManagedResources = false;
		}
	}
	if (mFullscreenZBuffer)
	{
		CheckDXError(mD3DDevice->SetRenderTarget(NULL, NULL), "SetRenderTarget DepthStencilSurface null pre-Reset (Present D3D8)");
		mFullscreenZBuffer->Release();
		mFullscreenZBuffer = false;
	}
	aResult = mD3DDevice->Reset(&mD3DPresentParams); //Prob correct
	mStateMgr->Reset();
	/*if (?) //?
	D3DInterface::Cleanup();
	return false;
	else*/
	{
		mNeedClearZBuffer = true;
		if (mNeedEvictManagedResources)
		{
			Flush(FLUSHF_ManagedResources_Immediate);
			mNeedEvictManagedResources = false;
		}
	}
	return false;
}

ulong D3D8Interface::GetCapsFlags() //1229-1237
{
	ulong aFlags = CAPF_AutoWindowedVSync | CAPF_ImageRenderTargets | CAPF_SingleImageTexture;
	if ((mDeviceCaps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP) != 0)
		aFlags = CAPF_CubeMaps | CAPF_AutoWindowedVSync | CAPF_ImageRenderTargets | CAPF_SingleImageTexture;
	if ((mDeviceCaps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP) != 0)
		aFlags |= CAPF_VolumeMaps;
	return aFlags;
}

ulong D3D8Interface::GetMaxTextureStages() //1240-1242
{
	return mDeviceCaps.MaxTextureBlendStages;
}

std::string D3D8Interface::GetInfoString(EInfoString theInfoString) //1247-1366
{
	switch (theInfoString)
	{
	case INFOSTRING_Adapter:
		if (mD3D && mAdapterInfoString.empty())
		{
			D3DADAPTER_IDENTIFIER8 aIdent;
			ZeroMemory(&aIdent, 1068);
			if (mD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &aIdent) >= 0)
				mAdapterInfoString += StrFormat("\"%s\" V.%x D.%x S.%x R.%x \"%s\" %d.%d.%d.%d", aIdent.Description, aIdent.VendorId, aIdent.DeviceId, aIdent.SubSysId, aIdent.Revision, aIdent.Driver, (aIdent.DriverVersion.QuadPart >> 16) & 0xFFFF, (aIdent.DriverVersion.QuadPart) & 0xFFFF, (aIdent.DriverVersion.LowPart >> 16) & 0xFFFF, (aIdent.DriverVersion.QuadPart) & 0xFFFF);
			return mAdapterInfoString;
			break;
		}
	case INFOSTRING_DrvProductVersion: return mD3DProductVersionString; break;
	case INFOSTRING_DisplayMode:
		if (!mD3D)
		{
			return "";
			break;
		}
		D3DDISPLAYMODE aMode;
		if (mD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &aMode) >= 0)
		{
			std::string aFormatStr = "";
			int aBitDepth;
			switch (aMode.Format)
			{
			case D3DFMT_A8R8G8B8: aFormatStr = "A8R8G8B8"; aBitDepth = 32; break;
			case D3DFMT_X8R8G8B8: aFormatStr = "X8R8G8B8"; aBitDepth = 32; break;
			case D3DFMT_R5G6B5: aFormatStr = "R5G6B5"; aBitDepth = 16; break;
			case D3DFMT_X1R5G5B5: aFormatStr = "X1R5G5B5"; aBitDepth = 16; break;
			case D3DFMT_A1R5G5B5: aFormatStr = "A1R5G5B5"; aBitDepth = 16; break;
			default: aFormatStr = "UnknownFmt"; aBitDepth = 0; break;
			}
			std::string aStr = StrFormat("%dx%dx%d (%s)", aMode.Width, aMode.Height, aBitDepth, aFormatStr.c_str());
			if (aMode.RefreshRate)
				aStr += StrFormat(" @ %dHz", aMode.RefreshRate);
			return aStr;
		}
		else
			return "";
		break;
	case INFOSTRING_BackBuffer:
		if (!mD3DDevice)
		{
			return "";
			break;
		}
		std::string aFormatStr = "";
		int aBitDepth;
		switch (mD3DPresentParams.BackBufferFormat)
		{
		case D3DFMT_A8R8G8B8: aFormatStr = "A8R8G8B8"; aBitDepth = 32; break;
		case D3DFMT_X8R8G8B8: aFormatStr = "X8R8G8B8"; aBitDepth = 32; break;
		case D3DFMT_R5G6B5: aFormatStr = "R5G6B5"; aBitDepth = 32; break;
		case D3DFMT_X1R5G5B5: aFormatStr = "X1R5G5B5"; aBitDepth = 32; break;
		case D3DFMT_A1R5G5B5: aFormatStr = "A1R5G5B5"; aBitDepth = 32; break;
		default: aFormatStr = "UnknownFmt"; aBitDepth = 0; break;
		}
		return StrFormat("%dx%dx%d (%s)", mD3DPresentParams.BackBufferWidth, mD3DPresentParams.BackBufferHeight, aBitDepth, aFormatStr.c_str());
		break;
	case INFOSTRING_TextureMemory:
		if (!mD3DDevice)
		{
			return "";
			break;
		}
		int aAllocedK = mTexMemUsageBytesAlloced >> 10;
		if (mTexMemUsageBytesAlloced % 1024)
			aAllocedK++;
		int aWastedBytes = mTexMemUsageBytesAlloced - mTexMemUsageBytesOriginal;
		int aWastedK = aWastedBytes / 1024;
		if (aWastedBytes % 1024)
			++aWastedK;
		int aCurFrameK = mTexMemUsageBytesCurFrame >> 10;
		if (mTexMemUsageBytesCurFrame % 0x400)
			++aCurFrameK;
		int aWastedPct = 0;
		int aCurFramePct = 0;
		if (aAllocedK > 0)
		{
			aWastedPct = (aWastedK * 100.0 / aAllocedK);
			aCurFramePct = (aCurFrameK * 100.0 / aAllocedK);
		}
		return StrFormat("%dkB Drawn (%d%%) / %dkB Alloced / %dkB Wasted (%d%%)", aCurFrameK, aCurFramePct, aAllocedK, aWastedK, aWastedPct);
		break;
	default:
		return "";
		break;
	}
}

void D3D8Interface::GetBackBufferDimensions(ulong& outWidth, ulong& outHeight) //1371-1374
{
	outWidth = mD3DPresentParams.BackBufferWidth;
	outHeight = mD3DPresentParams.BackBufferHeight;
}

IUnknown* D3D8Interface::CreateSurface(int inWidth, int inHeight, bool inRenderTarget, bool inTexture) //1379-1422
{
	IUnknown* aSurface = NULL;
	if (inRenderTarget)
		Flush(FLUSHF_ManagedResources_Immediate);
	if (inTexture)
	{
		HRESULT hr = mDisplayFormat == SEXY3DFMT_X8R8G8B8 ? mD3DDevice->CreateTexture(inWidth, inHeight, 1, inRenderTarget, D3DFMT_A8R8G8B8, (D3DPOOL)!inRenderTarget, (LPDIRECT3DTEXTURE8*)aSurface) : mD3DDevice->CreateTexture(inWidth, inHeight, 1, inRenderTarget, (D3DFORMAT)mDisplayFormat, (D3DPOOL)!inRenderTarget, (LPDIRECT3DTEXTURE8*)aSurface);
		if (!SUCCEEDED(hr)) //?
		{
			Flush(FLUSHF_ManagedResources_Immediate);
			HRESULT hr = mDisplayFormat == SEXY3DFMT_X8R8G8B8 ? mD3DDevice->CreateTexture(inWidth, inHeight, 1, inRenderTarget, D3DFMT_A8R8G8B8, (D3DPOOL)!inRenderTarget, (LPDIRECT3DTEXTURE8*)aSurface) : mD3DDevice->CreateTexture(inWidth, inHeight, 1, inRenderTarget, (D3DFORMAT)mDisplayFormat, (D3DPOOL)!inRenderTarget, (LPDIRECT3DTEXTURE8*)aSurface);
		}
		return hr ? aSurface : NULL;
	}
	else if (inRenderTarget)
	{
		HRESULT hr = mD3DDevice->CreateRenderTarget(inWidth, inHeight, (D3DFORMAT)mDisplayFormat, D3DMULTISAMPLE_NONE, 1, (LPDIRECT3DSURFACE8*)aSurface);
		if (!SUCCEEDED(hr)) //?
		{
			Flush(FLUSHF_ManagedResources_Immediate);
			HRESULT hr = mD3DDevice->CreateRenderTarget(inWidth, inHeight, (D3DFORMAT)mDisplayFormat, D3DMULTISAMPLE_NONE, true, (LPDIRECT3DSURFACE8*)aSurface);
		}
		return hr ? aSurface : NULL;
	}
	else
	{
		HRESULT hr = mD3DDevice->CreateImageSurface(inWidth, inHeight, (D3DFORMAT)mDisplayFormat, (LPDIRECT3DSURFACE8*)aSurface);
		return hr ? aSurface : NULL;
	}
}

bool D3D8Interface::CanBltSurface(bool srcSurfaceIsTexture) //1425-1428
{
	return srcSurfaceIsTexture;
}

void D3D8Interface::BltSurface(IUnknown* theSurface, const Rect& theDest, const Rect& theSrc) //1431-1558
{
	if (!PreDraw())
		return;

	RECT aSrcRect = theSrc.ToRECT();
	RECT aDestRect = theDest.ToRECT();
	LPDIRECT3DSURFACE8 aSrcSurface = (LPDIRECT3DSURFACE8)theSurface;
	D3DSURFACE_DESC aSrcDesc;
	aSrcSurface->GetDesc(&aSrcDesc);
	if (aSrcDesc.Usage & D3DUSAGE_RENDERTARGET)
	{
		LPDIRECT3DSURFACE8 aDestSurface = (LPDIRECT3DSURFACE8)mCurRenderTargetSurface;
		bool doStretchRect = false;
		LPDIRECT3DSURFACE8 aSrcTexture = NULL;
		HRESULT aResult = aSrcSurface->GetContainer(IID_IDirect3DTexture8, (void**)&aSrcTexture);
		if (aResult >= 0 && aSrcTexture)
			doStretchRect = 0;
		else
		{
			doStretchRect = 1;
			aSrcTexture = 0;
		}
		if (!doStretchRect)
		{
			D3DTextureData td(this);
			td.mWidth = aSrcDesc.Width;
			td.mHeight = aSrcDesc.Height;
			td.mTexVecHeight = 1;
			td.mTexVecWidth = 1;
			td.mTexPieceWidth = aSrcDesc.Width;
			td.mTexPieceHeight = aSrcDesc.Height;
			td.mMaxTotalV = 1.0;
			td.mMaxTotalU = 1.0;
			td.mTextures.resize(1);
			D3DTextureDataPiece& aPiece = td.mTextures[0];
			aPiece.mTexture = aSrcTexture;
			aPiece.mCubeTexture = 0;
			aPiece.mVolumeTexture = 0;
			aPiece.mWidth = td.mWidth;
			aPiece.mHeight = td.mHeight;
			float xScale = (double)theDest.mWidth / (double)theSrc.mWidth;
			float yScale = (double)theDest.mHeight / (double)theSrc.mHeight;
			SexyTransform2D aTransform;
			aTransform.Scale(xScale, yScale);
			SetupDrawMode(Graphics::DRAWMODE_NORMAL);
			SetTextureLinearFilter(0, true);
			IGraphicsDriver::ERenderMode oldRenderMode = GetEffectiveRenderMode();
			if (oldRenderMode)
			{
				mGraphicsDriver->mRenderMode = IGraphicsDriver::RENDERMODE_Default;
				PushState();
				mStateMgr->SetBlendOverride(Graphics3D::BLEND_ONE, Graphics3D::BLEND_ZERO);
				mStateMgr->SetTextureStageState(0, 1u, 4u);
				mStateMgr->SetTextureStageState(0, 2u, 2u);
				mStateMgr->SetTextureStageState(0, 3u, 0);
				mStateMgr->SetTextureStageState(0, 4u, 4u);
				mStateMgr->SetTextureStageState(0, 5u, 2u);
				mStateMgr->SetTextureStageState(0, 6u, 0);
				mStateMgr->SetTextureStageState(1u, 1u, 1u);
				mStateMgr->SetTextureStageState(1u, 4u, 1u);
				mStateMgr->SetRenderState(0x1Bu, 0);
				mStateMgr->SetRenderState(0x13u, 2u);
				mStateMgr->SetRenderState(0x14u, 1u);
				mStateMgr->SetRenderState(0xFu, 0);
				mStateMgr->SetRenderState(0x19u, 8u);
				mStateMgr->SetRenderState(0x18u, 0);
				mStateMgr->SetRenderState(8u, 3u);
				mStateMgr->CommitState();
			}
			bool wasTracingPixels = gTracingPixels;
			gTracingPixels = false;
			td.BltTransformed(this, NULL, Graphics::DRAWMODE_NORMAL, aTransform, theSrc, Color(255, 255, 255, 255), 0, 0.0, 0.0);
			gTracingPixels = wasTracingPixels;
			if (oldRenderMode)
			{
				PopState();
				mGraphicsDriver->mRenderMode = oldRenderMode;
			}
			td.mTextures.clear();
		}
		if (aSrcTexture)
			aSrcTexture->Release();
	}
}

void D3D8Interface::ClearColorBuffer(const Color& inColor) //1563-1578
{
	if (gTracingPixels)
	{
		Image* aPrev = gPixelTracerLastImage;
		gPixelTracerLastImage = NULL;
		PixelTracerCheckPrimitives(-1, 0, NULL, 0);
		gPixelTracerLastImage = aPrev;
	}
	FlushBufferedTriangles();
	mStateMgr->CommitState();
	mD3DDevice->Clear(0, NULL, 1, inColor.mBlue | (inColor.mGreen << 8) | (inColor.mRed << 16) | (inColor.mAlpha << 24), 1.0, 0);
}

void D3D8Interface::ClearDepthBuffer() //1581-1587
{
	FlushBufferedTriangles();
	mStateMgr->CommitState();

	mD3DDevice->Clear(0, NULL, 2, 0xFF000000, 1.0, 0);
	mNeedClearZBuffer = false;
}

D3DFORMAT Sexy::MakeD3DFORMAT(PixelFormat theFormat) //1592-1604
{
	switch (theFormat)
	{
	case PixelFormat_A8R8G8B8: return D3DFMT_A8R8G8B8; break;
	case PixelFormat_A4R4G4B4: return D3DFMT_A4R4G4B4; break;
	case PixelFormat_R5G6B5: return D3DFMT_R5G6B5; break;
	case PixelFormat_Palette8: return D3DFMT_P8; break;
	case PixelFormat_X8R8G8B8: return D3DFMT_X8R8G8B8; break;
	default: return D3DFMT_UNKNOWN; break;
	}
}

HRESULT D3D8Interface::InternalValidateDevice(ulong* outNumPasses) //1609-1615
{
	if (outNumPasses)
		*outNumPasses = 0;

	return mD3DDevice ? mD3DDevice->ValidateDevice(outNumPasses) : D3DERR_INVALIDCALL;
}
HRESULT D3D8Interface::InternalCreateVertexShader(const ulong* inFunction, IUnknown** outShader) //1617-1619
{
	return D3DERR_INVALIDCALL; //Not supported on D3D8 probably
}
HRESULT D3D8Interface::InternalCreatePixelShader(const ulong* inFunction, IUnknown** outShader) //1621-1623
{
	return D3DERR_INVALIDCALL; //Not supported on D3D8 probably
}
HRESULT D3D8Interface::InternalSetPaletteEntries(uint inPaletteNumber, const PALETTEENTRY* inEntries) //1625-1627
{
	return mD3DDevice->SetPaletteEntries(inPaletteNumber, inEntries);
}
HRESULT D3D8Interface::InternalGetPaletteEntries(uint inPaletteNumber, PALETTEENTRY* inEntries) //1629-1631
{
	return mD3DDevice->SetPaletteEntries(inPaletteNumber, inEntries);
}
HRESULT D3D8Interface::InternalCreateTexture(uint inWidth, uint inHeight, uint inLevels, bool inRenderTarget, PixelFormat inFormat, ulong inPool, IUnknown** outTexture) //1633-1635
{
	return mD3DDevice->CreateTexture(inWidth, inHeight, inLevels, inRenderTarget, MakeD3DFORMAT(inFormat), (D3DPOOL)inPool, (LPDIRECT3DTEXTURE8*)outTexture);
}
HRESULT D3D8Interface::InternalCreateCubeTexture(uint inEdgeLength, uint inLevels, ulong inUsage, PixelFormat inFormat, ulong inPool, IUnknown** outCubeTexture) //1637-1639
{
	return mD3DDevice->CreateCubeTexture(inEdgeLength, inLevels, inUsage, MakeD3DFORMAT(inFormat), (D3DPOOL)inPool, (LPDIRECT3DCUBETEXTURE8*)outCubeTexture);
}
HRESULT D3D8Interface::InternalCreateVolumeTexture(uint inWidth, uint inHeight, uint inDepth, uint inLevels, ulong inUsage, PixelFormat inFormat, ulong inPool, IUnknown** outVolumeTexture) //1641-1643
{
	return mD3DDevice->CreateVolumeTexture(inWidth, inHeight, inDepth, inLevels, inUsage, MakeD3DFORMAT(inFormat), (D3DPOOL)inPool, (LPDIRECT3DVOLUMETEXTURE8*)outVolumeTexture);
}
HRESULT D3D8Interface::InternalUpdateTexture(IUnknown* inSourceTexture, IUnknown* inDestTexture) //1645-1647
{
	return mD3DDevice->UpdateTexture((LPDIRECT3DBASETEXTURE8)inSourceTexture, (LPDIRECT3DBASETEXTURE8)inDestTexture);
}
HRESULT D3D8Interface::InternalCreateImageSurface(uint inWidth, uint inHeight, PixelFormat inFormat, IUnknown** outSurface) //1649-1651
{
	return mD3DDevice->CreateImageSurface(inWidth, inHeight, MakeD3DFORMAT(inFormat), (LPDIRECT3DSURFACE8*)outSurface);
}
HRESULT D3D8Interface::InternalGetRenderTargetData(IUnknown* inRenderTarget, IUnknown* inDestSurface) //1653-1655
{
	return mD3DDevice->CopyRects((LPDIRECT3DSURFACE8)inRenderTarget, NULL, 0, (LPDIRECT3DSURFACE8)inDestSurface, NULL);
}
HRESULT D3D8Interface::InternalSurfaceLockRect(IUnknown* inSurface, int& outPitch, void*& outBits) //1657-1666 (Accurate?)
{
	D3DLOCKED_RECT aLockRect;
	HRESULT hr = ((LPDIRECT3DSURFACE8)inSurface)->LockRect(&aLockRect, NULL, 0);
	if (hr >= 0)
		outPitch = aLockRect.Pitch;
	return hr;
}
HRESULT D3D8Interface::InternalSurfaceUnlockRect(IUnknown* inSurface) //1668-1670
{
	return ((LPDIRECT3DSURFACE8)inSurface)->UnlockRect();
}
HRESULT D3D8Interface::InternalTextureGetSurfaceLevel(IUnknown* inTexture, uint inLevel, IUnknown** outSurface) //1672-1674
{
	return ((LPDIRECT3DTEXTURE8)inTexture)->GetSurfaceLevel(inLevel, (LPDIRECT3DSURFACE8*)outSurface);
}
HRESULT D3D8Interface::InternalTextureMakeDirty(IUnknown* inTexture) //1676-1678
{
	return ((LPDIRECT3DTEXTURE8)inTexture)->AddDirtyRect(NULL);
}
HRESULT D3D8Interface::InternalTextureLockRect(IUnknown* inTexture, int& outPitch, void*& outBits) //1680-1689
{
	D3DLOCKED_RECT aLockRect;
	HRESULT hr = ((LPDIRECT3DTEXTURE8)inTexture)->LockRect(0, &aLockRect, NULL, 0);
	if (hr >= 0)
		outPitch = aLockRect.Pitch;
	return hr;
}
HRESULT D3D8Interface::InternalTextureUnlockRect(IUnknown* inTexture) //1691-1693
{
	return ((LPDIRECT3DTEXTURE8)inTexture)->UnlockRect(0);
}
HRESULT D3D8Interface::InternalCubeTextureLockRect(IUnknown* inCubeTexture, ulong inFace, int& outPitch, void*& outBits) //1695-1704
{
	D3DLOCKED_RECT aLockRect;
	HRESULT hr = ((LPDIRECT3DCUBETEXTURE8)inCubeTexture)->LockRect((D3DCUBEMAP_FACES)inFace, 0, &aLockRect, NULL, 0);
	if (hr >= 0)
		outPitch = aLockRect.Pitch;
	return hr;
}
HRESULT D3D8Interface::InternalCubeTextureUnlockRect(IUnknown* inCubeTexture, ulong inFace) //1706-1708
{
	return ((LPDIRECT3DCUBETEXTURE8)inCubeTexture)->UnlockRect((D3DCUBEMAP_FACES)inFace, 0);
}
HRESULT D3D8Interface::InternalVolumeTextureLockBox(IUnknown* inVolumeTexture, int& outRowPitch, int& outSlicePitch, void*& outBits) //1710-1720
{
	D3DLOCKED_BOX aLockBox;
	HRESULT hr = ((LPDIRECT3DVOLUMETEXTURE8)inVolumeTexture)->LockBox(0, &aLockBox, NULL, 0);
	if (SUCCEEDED(hr))
		outRowPitch = aLockBox.RowPitch;
	return hr;
}
HRESULT D3D8Interface::InternalVolumeTextureUnlockBox(IUnknown* inVolumeTexture) //1722-1724
{
	return ((LPDIRECT3DVOLUMETEXTURE8)inVolumeTexture)->UnlockBox(0);
}
HRESULT D3D8Interface::InternalSetRenderTarget(void* inRenderTargetSurface) //1726-1741
{
	if (mD3DDevice == NULL)
		return D3DERR_INVALIDCALL;

	LPDIRECT3DSURFACE8 aDepthStencilSurface = NULL;
	HRESULT aResult = mD3DDevice->GetDepthStencilSurface(&aDepthStencilSurface);
	if (aResult < 0)
		return aResult;
	aResult = mD3DDevice->SetRenderTarget((LPDIRECT3DSURFACE8)inRenderTargetSurface, aDepthStencilSurface);
	if (aDepthStencilSurface)
		aDepthStencilSurface->Release();
	return aResult;
}
HRESULT D3D8Interface::InternalBeginScene() //1743-1745
{
	return mD3DDevice->BeginScene();
}
HRESULT D3D8Interface::InternalCreateVertexBuffer(uint inLength, bool inIsDynamic, ulong inFVF, ulong inPool, IUnknown** outVertexBuffer) //1747-1749
{
	return mD3DDevice->CreateVertexBuffer(inLength, inIsDynamic ? 520 : 0, inFVF, (D3DPOOL)inPool, (LPDIRECT3DVERTEXBUFFER8*)outVertexBuffer);
}
HRESULT D3D8Interface::InternalCreateIndexBuffer(uint inLength, ulong inPool, IUnknown** outIndexBuffer) //1751-1753
{
	return mD3DDevice->CreateIndexBuffer(inLength, 0, D3DFMT_INDEX16, (D3DPOOL)inPool, (LPDIRECT3DINDEXBUFFER8*)outIndexBuffer);
}
HRESULT D3D8Interface::InternalVertexBufferLock(IUnknown* inVertexBuffer, uint inOffset, uint inSize, void** outData, ulong inLockFlags) //1755-1757
{
	return((LPDIRECT3DVERTEXBUFFER8)inVertexBuffer)->Lock(inOffset, inSize, (BYTE**)outData, inLockFlags);
}
HRESULT D3D8Interface::InternalVertexBufferUnlock(IUnknown* inVertexBuffer) //1759-1761
{
	return ((LPDIRECT3DVERTEXBUFFER8)inVertexBuffer)->Unlock();
}
HRESULT D3D8Interface::InternalIndexBufferLock(IUnknown* inIndexBuffer, uint inOffset, uint inSize, void** outData, ulong inLockFlags) //1763-1765
{
	return ((LPDIRECT3DINDEXBUFFER8)inIndexBuffer)->Lock(inOffset, inSize, (BYTE**)outData, inLockFlags);
}
HRESULT D3D8Interface::InternalIndexBufferUnlock(IUnknown* inIndexBuffer) //1767-1769
{
	return ((LPDIRECT3DINDEXBUFFER8)inIndexBuffer)->Unlock();
}
HRESULT D3D8Interface::InternalDrawIndexedPrimitive(ulong inPrimType, uint inMinIndex, uint inNumVertices, uint inStartIndex, uint inPrimCount) //1771-1773
{
	return mD3DDevice->DrawIndexedPrimitive((D3DPRIMITIVETYPE)inPrimType, inMinIndex, inNumVertices, inStartIndex, inPrimCount);
}
HRESULT D3D8Interface::InternalSetStreamSource(uint inStreamNumber, IUnknown* inVertexBuffer, uint inStride) //1775-1777
{
	return mD3DDevice->SetStreamSource(inStreamNumber, (LPDIRECT3DVERTEXBUFFER8)inVertexBuffer, inStride);
}
