#include "D3D9Interface.h"
#include "WindowsGraphicsDriver.h"
#include "../../app/windows/WindowsAppDriver.h"
#include "../../../Debug.h"
#include "../../../PixelTracer.h"
#include "../../../ResourceManager.h"
#include "../../../CfgMachine.h"
//#include <d3dx9Shader.h>

using namespace Sexy;

D3DInterface* Sexy::CreateD3D9Interface() //53-55
{
	return new D3D9Interface();
}

bool D3DStateManager9::DoCommitRenderState(State* inState) //68-77
{
	assert(inState->mContext[0] == SG_RS); //69
	D3DStateManager9* m = (D3DStateManager9*)inState->mManager;

	bool result = D3DInterface::CheckDXError(m->mDevice->SetRenderState(SG_RS, inState->GetDword()), "SetRenderState");
	inState->ClearDirty();
	return result;
}
bool D3DStateManager9::DoCommitTextureStageState(State* inState) //79-88
{
	assert(inState->mContext[0] == SG_TSS); //80
	D3DStateManager9* m = (D3DStateManager9*)inState->mManager;



	bool result = D3DInterface::CheckDXError(m->mDevice->SetRenderState(SG_TSS, inState->GetDword()), "SetTextureStageState");
	inState->ClearDirty();
	return;
}
bool D3DStateManager9::DoCommitSamplerState(State* inState) //90-99
{
	assert(inState->mContext[0] == SG_SS); //91
	D3DStateManager9* m = (D3DStateManager9*)inState->mManager;



	bool result = D3DInterface::CheckDXError(m->mDevice->SetRenderState(SG_SS, inState->GetDword()), "SetSamplerStageState");
	inState->ClearDirty();
	return;
}
bool D3DStateManager9::DoCommitLightState(State* inState) //101-140
{
	assert(inState->mContext[0] == SG_LIGHT); //102
	D3DStateManager9* m = (D3DStateManager9*)inState->mManager;
	ulong stateId = inState->mContext[1];
	ulong lightIndex = inState->mContext[2];
	float unused;
	bool result;
	if (stateId != NULL)
	{
		State* s;
		D3DLIGHT9 light;
		s = &m->mLightStates[ST_LIGHT_TYPE][lightIndex]; light.Type = (_D3DLIGHTTYPE)s->GetDword(); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_DIFFUSE][lightIndex]; s->GetVector(light.Diffuse.r, light.Diffuse.g, light.Diffuse.b, light.Diffuse.a); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_SPECULAR][lightIndex]; s->GetVector(light.Specular.r, light.Specular.g, light.Specular.b, light.Specular.a); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_AMBIENT][lightIndex]; s->GetVector(light.Ambient.r, light.Ambient.g, light.Ambient.b, light.Ambient.a); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_POSITION][lightIndex]; s->GetVector(light.Position.x, light.Position.y, light.Position.z, unused); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_DIRECTION][lightIndex]; s->GetVector(light.Direction.x, light.Direction.y, light.Direction.z, unused); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_RANGE][lightIndex]; light.Range = s->GetFloat(); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_FALLOFF][lightIndex]; light.Falloff = s->GetFloat(); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_ATTENUATION][lightIndex]; s->GetVector(light.Attenuation0, light.Attenuation1, light.Attenuation2, unused); s->ClearDirty();
		s = &m->mLightStates[ST_LIGHT_ANGLES][lightIndex]; s->GetVector(light.Theta, light.Phi, unused, unused); s->ClearDirty();
		result = !D3DInterface::CheckDXError(m->mDevice->SetLight(lightIndex, &light), "SetLight");
	}
	else
	{
		result = !D3DInterface::CheckDXError(m->mDevice->LightEnable(lightIndex, inState->GetDword() != 0), "LightEnable");
		inState->ClearDirty(false);
	}
	return result;
}
bool D3DStateManager9::DoCommitMaterialState(State* inState) //142-160
{
	assert(inState->mContext[0] == SG_MATERIAL); //143
	D3DStateManager9* m = (D3DStateManager9*)inState->mManager;
	State* s = &m->mMaterialStates[0]; //?
	D3DMATERIAL9 mat;	
	s = &m->mMaterialStates[ST_MAT_DIFFUSE]; s->GetVector(mat.Diffuse.r, mat.Diffuse.g, mat.Diffuse.b, mat.Diffuse.a); s->ClearDirty();
	s = &m->mMaterialStates[ST_MAT_AMBIENT]; s->GetVector(mat.Ambient.r, mat.Ambient.g, mat.Ambient.b, mat.Ambient.a); s->ClearDirty();
	s = &m->mMaterialStates[ST_MAT_SPECULAR]; s->GetVector(mat.Specular.r, mat.Specular.g, mat.Specular.b, mat.Specular.a); s->ClearDirty();
	s = &m->mMaterialStates[ST_MAT_EMISSIVE]; s->GetVector(mat.Emissive.r, mat.Emissive.g, mat.Emissive.b, mat.Emissive.a); s->ClearDirty();
	s = &m->mMaterialStates[ST_MAT_POWER]; mat.Power = s->GetFloat(); s->ClearDirty();
	bool result = !D3DInterface::CheckDXError(m->mDevice->SetMaterial(&mat), "SetMaterial");
	return result;
}
bool D3DStateManager9::DoCommitStreamState(State* inState) //162-194
{
	assert(inState->mContext[0] == SG_STREAM); //163
	D3DStateManager9* m = (D3DStateManager9*)inState->mManager;
	bool result;
	ulong streamIndex = inState->mContext[2];
	State* s;
	if (inState->mContext[1] == SG_LIGHT)
	{
		bool result = !D3DInterface::CheckDXError(m->mDevice->SetStreamSourceFreq(streamIndex, inState->GetDword()));
		inState->ClearDirty(false);
		return result;
	}
	else
	{
		s = &m->mStreamStates[ST_STREAM_DATA][streamIndex]; LPDIRECT3DVERTEXBUFFER9 data = (LPDIRECT3DVERTEXBUFFER9)s->GetPtr(); s->ClearDirty();
		s = &m->mStreamStates[ST_STREAM_OFFSET][streamIndex]; ulong offset = s->GetDword(); s->ClearDirty();
		s = &m->mStreamStates[ST_STREAM_STRIDE][streamIndex]; ulong stride = s->GetDword(); s->ClearDirty();
		inState->ClearDirty();
		bool result = m->mDevice->SetStreamSource(streamIndex, data, offset, stride);
		return !D3DInterface::CheckDXError(result, "SetStreamSource");
	}
}
bool D3DStateManager9::DoCommitTransformState(State* inState) //196-213
{
	assert(inState->mContext[0] == SG_TRANSFORM); //197
	D3DStateManager9* m = (D3DStateManager9*)inState->mManager;
	D3DTRANSFORMSTATETYPE stateId = (D3DTRANSFORMSTATETYPE)inState->mContext[1];
	State* s;
	D3DMATRIX mat;
	s = &m->mTransformStates[stateId][0]; s->GetVector(mat._11, mat._12, mat._13, mat._14); s->ClearDirty();
	s = &m->mTransformStates[stateId][1]; s->GetVector(mat._21, mat._22, mat._23, mat._24); s->ClearDirty();
	s = &m->mTransformStates[stateId][2]; s->GetVector(mat._31, mat._32, mat._33, mat._34); s->ClearDirty();
	s = &m->mTransformStates[stateId][3]; s->GetVector(mat._41, mat._42, mat._43, mat._44); s->ClearDirty();
	bool result = m->mDevice->SetTransform(stateId, &mat);
	return D3DInterface::CheckDXError(result, "SetTransform");
}
bool D3DStateManager9::DoCommitViewportState(State* inState) //215-234
{
	assert(inState->mContext[0] == SG_VIEWPORT); //216
	D3DStateManager9* m = (D3DStateManager9*)inState->mManager;
	State* s;
	D3DVIEWPORT9 vp;
	s = &m->mViewportStates[ST_VIEWPORT_X]; s->GetDword(); s->ClearDirty();
	s = &m->mViewportStates[ST_VIEWPORT_Y]; s->GetDword(); s->ClearDirty();
	s = &m->mViewportStates[ST_VIEWPORT_WIDTH]; s->GetDword(); s->ClearDirty();
	s = &m->mViewportStates[ST_VIEWPORT_HEIGHT]; s->GetDword(); s->ClearDirty();
	s = &m->mViewportStates[ST_VIEWPORT_X]; s->GetFloat(); s->ClearDirty();
	s = &m->mViewportStates[ST_VIEWPORT_Y]; s->GetFloat(); s->ClearDirty();
	bool result = m->mDevice->SetViewport(&vp);
	return D3DInterface::CheckDXError(result, "SetViewport");
}
bool D3DStateManager9::DoCommitMiscState(State* inState) //236-419
{
	assert(inState->mContext[0] == SG_MISC); //274
	D3DStateManager9* m = (D3DStateManager9*)inState->mManager;
	ulong stateId = inState->mContext[2];
	ulong subIndex = inState->mContext[2];
	switch (stateId)
	{
	case ST_MISC_FVF: bool result = !D3DInterface::CheckDXError(m->mDevice->SetFVF(inState->GetDword()), "SetFVF"); inState->ClearDirty(false); break;
	case ST_MISC_FVFSIZE:
	case ST_MISC_PIXELSHADER: bool result = !D3DInterface::CheckDXError(m->mDevice->SetPixelShader((LPDIRECT3DPIXELSHADER9)inState->GetPtr()), "SetPixelShader"); inState->ClearDirty(false); break;
	case ST_MISC_VERTEXSHADER: bool result = !D3DInterface::CheckDXError(m->mDevice->SetVertexShader((LPDIRECT3DVERTEXSHADER9)inState->GetPtr()), "SetVertexShader"); inState->ClearDirty(false); break;
	case ST_MISC_SCISSORRECT: RECT rc; bool result = !D3DInterface::CheckDXError(m->mDevice->SetScissorRect(&rc), "SetVertexShader"); inState->ClearDirty(false); break; //?
	case ST_MISC_NPATCHMODE: bool result = !D3DInterface::CheckDXError(m->mDevice->SetNPatchMode(inState->GetFloat()), "SetNPatchMode"); inState->ClearDirty(false); break; //?
	case ST_MISC_SRCBLENDOVERRIDE:
	case ST_MISC_DESTBLENDOVERRIDE:
	case ST_MISC_BLTDEPTH:
	case ST_MISC_PIXELSHADERCONST: bool result = !D3DInterface::CheckDXError(m->mDevice->SetPixelShaderConstantF(subIndex, &inState->mValue.mFloat, 1), "SetPixelShaderConstantF"); inState->ClearDirty(false); break; //?
	case ST_MISC_VERTEXSHADERCONST: bool result = !D3DInterface::CheckDXError(m->mDevice->SetVertexShaderConstantF(subIndex, &inState->mValue.mFloat, 1), "SetVertexShaderConstantF"); inState->ClearDirty(false); break; //?
	case ST_MISC_TEXTUREREMAP: inState->ClearDirty(false); bool result = true; break;
	case ST_MISC_INDICES: bool result = !D3DInterface::CheckDXError(m->mDevice->SetIndices((LPDIRECT3DINDEXBUFFER9)inState->GetPtr()), "SetIndices"); inState->ClearDirty(false); break;
	case ST_MISC_TEXTUREPALETTE: bool result = !D3DInterface::CheckDXError(m->mDevice->SetCurrentTexturePalette(inState->GetDword()), "SetCurrentTexturePalette"); inState->ClearDirty(false); break;
	case ST_MISC_TEXTURE: State* remapState = &m->mMiscStates[ST_MISC_TEXTUREREMAP][subIndex]; bool result = !D3DInterface::CheckDXError(m->mDevice->SetTexture(remapState->GetDword(), (LPDIRECT3DBASETEXTURE9)inState->GetPtr()), "SetTexture"); inState->ClearDirty(false); break;
	case ST_MISC_CLIPPLANE: bool result = !D3DInterface::CheckDXError(m->mDevice->SetClipPlane(subIndex, &inState->mValue.mFloat), "SetClipPlane"); inState->ClearDirty(false); break;
	default: assert(false && "Bad Misc State Id"); break; //414
	return result; 
	}
}

RenderStateManager::State::FCommitFunc D3DStateManager9::GetCommitFunc(State* inState) //422-436, looks roughly identical
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

D3DStateManager9::D3DStateManager9(D3D9Interface* inInterface, LPDIRECT3DDEVICE9 inDevice) { mInterface = inInterface; mDevice = inDevice; } //442


bool D3D9Interface::CheckRequiredCaps(const D3DCAPS9& theCaps) //448-498
{
	sTextureSizeMustBePow2 = theCaps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL && (theCaps.TextureCaps & D3DPTEXTURECAPS_POW2);
	if (sTextureSizeMustBePow2)
		return false;
	sMinTextureWidth = 8;
	sMinTextureHeight = 8;
	sMaxTextureWidth = theCaps.MaxTextureWidth;
	sMaxTextureHeight = theCaps.MaxTextureHeight;
	sMaxTextureAspectRatio = theCaps.MaxTextureAspectRatio;
	sCanStretchRectFromTextures = theCaps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES;
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

bool D3D9Interface::InitD3D(IGraphicsDriver::EResult* outErrorResult, bool preTestOnly, bool* outIsRecommended) //500-869
{
	static HMODULE sD3D9Lib;
	D3DFORMAT aFormat;
	if (outErrorResult)
		*outErrorResult = IGraphicsDriver::RESULT_OK;
	if (outIsRecommended)
		*outIsRecommended = true;
	if (sD3D9Lib == NULL)
		LoadLibrary("d3d9.dll");
	if (!sD3D9Lib == NULL)
		return false;

	mD3DProductVersionString = ((WindowsAppDriver*)(gSexyAppBase->mAppDriver))->GetProductVersion("d3d9.dll");
	typedef LPDIRECT3D9(WINAPI* Direct3DCreate9Func)(UINT SDKVersion);
	Direct3DCreate9Func aDirect3DCreate9Func = (Direct3DCreate9Func)GetProcAddress(sD3D9Lib, "Direct3DCreate9");
	if (aDirect3DCreate9Func == NULL)
		return false;

	mD3D = aDirect3DCreate9Func(-1); //?
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
			if (aBitsPerPixel == 16 && sSupportedScreenFormats & PixelFormat_R5G6B5)
				aFormat = D3DFMT_R5G6B5;
			else
				aFormat = D3DFMT_X8R8G8B8;
		}
		else if (sSupportedScreenFormats & PixelFormat_R5G6B5)
			aFormat = D3DFMT_R5G6B5;
		else
			aFormat = D3DFMT_X8R8G8B8;
	}
	else if ((mFullscreenBits == 16 && (sSupportedScreenFormats & PixelFormat_R5G6B5) || sSupportedScreenFormats & PixelFormat_A8R8G8B8))
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
	D3DCAPS9 aCaps;// = RtlZeroMemory(aCaps, sizeof aCaps); //?
	if (CheckDXError(mD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &aCaps), "GetDeviceCaps failed") || !CheckRequiredCaps(aCaps))
	{
		Cleanup();
		return false;
	}
	bool needMatchDepthBufferBits = false;
	if (gSexyAppBase->mCompatCfgMachine)
	{
		D3DADAPTER_IDENTIFIER9 aIdent; // = memset(&aIdent, 0, 1068);
		if ((mD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &aIdent) < 0))
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
			gSexyAppBase->SetString("compat_D3DDriverName", StringToWString(aIdent.Driver));
		}
		float psVersion = (double)(aCaps.PixelShaderVersion & 0xFFFF) / 256;
		float vsVersion = (double)(aCaps.VertexShaderVersion & 0xFFFF) / 256;
		gSexyAppBase->SetDouble("compat_D3DVSVersion", vsVersion);
		gSexyAppBase->SetDouble("compat_D3DPSVersion", psVersion);
		gSexyAppBase->SetInteger("compat_D3DInterface", 9); //d3d9
		gSexyAppBase->SetBoolean("IsVista", CheckForVista());
		gSexyAppBase->SetBoolean("IsWin7", CheckForWin7());

		CfgMachineValue aFuncResult(false);
		if (gSexyAppBase->mCompatCfgMachine->MachineExecuteFunction("Is3DSupported", &aFuncResult) && !aFuncResult.GetBoolean())
		{
			D3D9Interface::Cleanup();
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
	D3DPRESENT_PARAMETERS d3dpp; // = ZeroMemory(&d3dpp, 0); //?
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
			ulong aModeCount = mD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, aFormat);
			for (ulong iMode = 0; iMode < aModeCount; ++iMode)
			{
				D3DDISPLAYMODE aMode; // = memset(&aMode, 0, sizeof(aMode));
				if (mD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, aFormat, iMode, &aMode) >= 0 && aMode.Width <= mGraphicsDriver->mDesktopWidth && aMode.Height <= mGraphicsDriver->mDesktopHeight && aMode.Width == mWidth && aMode.Height == mHeight && aMode.Format == aFormat)
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
	}
	ulong aCreateFlags = 256;
	if (mD3D->CreateDevice(0, D3DDEVTYPE_HAL, mHWnd, 320, &d3dpp, &mD3DDevice) >= 0)
	{
		HRESULT aResult = mD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, mHWnd, aCreateFlags | 32, &d3dpp, &mD3DDevice);
		if (aResult == D3DERR_DEVICELOST)
		{
			if (outErrorResult)
				*outErrorResult = IGraphicsDriver::RESULT_3D_NOTREADY;
			mD3D->Release();
			mD3D = NULL;
			sErrorString = "Create Device - Device Lost";
			return false;
		}
		if (CheckDXError(aResult, "Create Device"))
		{
			D3DInterface::Cleanup();
			return false;
		}
		if (mD3DDevice)
		{
			mD3D->Release();
			mD3D = NULL;
			sErrorString = "Create Device";
			return false;
		}
	}
	mStateMgr = new D3DStateManager9(this, mD3DDevice);
	mStateMgr->Init();
	gPixelTracerStateManager = mStateMgr;
	if (d3dpp.EnableAutoDepthStencil)
	{
		int aDepthWidth = mWidth < d3dpp.BackBufferWidth ? d3dpp.BackBufferWidth : mWidth;
		int aDepthHeight = mHeight < d3dpp.BackBufferHeight ? d3dpp.BackBufferHeight : mHeight;
		if (CheckDXError(mD3DDevice->CreateDepthStencilSurface(aDepthWidth, aDepthHeight, d3dpp.AutoDepthStencilFormat, D3DMULTISAMPLE_NONE, 0, false, &mFullscreenZBuffer, NULL), "CreateDepthStencilSurface (InitD3D D3D9)"))
		{
			D3DInterface::Cleanup();
			return false;
		}
		if (CheckDXError(mD3DDevice->SetDepthStencilSurface(mFullscreenZBuffer), "SetDepthStencilSurface (InitD3D D3D9)"))
		{
			D3DInterface::Cleanup();
			return false;
		}
	}
	//?
	if (CheckDXError(mD3DDevice->GetDeviceCaps(&aCaps), "GetDeviceCaps failed"))
	{
		D3DInterface::Cleanup();
		return false;
	}
	CopyMemory(mDeviceCaps, &aCaps, sizeof mDeviceCaps);
	if (CheckRequiredCaps == false)
	{
		D3DInterface::Cleanup();
		return false;
	}
	SetDefaultState(NULL, false);
	if (mStateMgr->CommitState() == false)
	{
		D3DInterface::Cleanup();
		return false;
	}
	mDisplayFormat = (SEXY3DFORMAT)aFormat;
	if (CheckDXError(mD3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, (LPDIRECT3DSURFACE9*)&mBackBufferSurface), "GetBackBuffer"))
	{
		D3DInterface::Cleanup();
		return false;
	}
	mCurRenderTargetImage = NULL;
	mCurRenderTargetSurface = mBackBufferSurface;
	if (gSexyAppBase->mWantsDialogCompatibility && !mIsWindowed)
		mD3DDevice->SetDialogBoxMode(true);
	mD3DDevice->Clear(0, NULL, 1, 0xFF000000, 1, 0);
	CopyMemory(mD3DPresentParams, d3dpp, sizeof mD3DPresentParams);
	Mesh::MeshPieceList::iterator aMeshItr = mGraphicsDriver.mMeshSet.begin();
	while (aMeshItr != mGraphicsDriver.mMeshSet.end())
	{
		Mesh* aMesh = aMeshItr;
		LoadMesh(aMesh);
		aMeshItr++;
	}
	mTexMemResQuery = NULL;
	if ((mD3DDevice->CreateQuery(D3DQUERYTYPE_RESOURCEMANAGER, NULL) >= 0 && mD3DDevice->CreateQuery(D3DQUERYTYPE_RESOURCEMANAGER, &mTexMemResQuery) < 0))
		mTexMemResQuery = NULL;
	return true;
}

void D3D9Interface::SetupSupportedRenderTargetFormats() //874-890
{
	sSupportedScreenFormats = 0;
	static int D3DFMT_PixelFormat_Map[4] = { 16 };
	for (int i = 0; i < 4; i += 2)
	{
		if (CheckDXError(mD3D->CheckDeviceType(0, D3DDEVTYPE_HAL, (D3DFORMAT)D3DFMT_PixelFormat_Map[i], (D3DFORMAT)D3DFMT_PixelFormat_Map[i], mIsWindowed)))
			sSupportedScreenFormats |= 1, 23, 4, 0; //?
	}
}

void D3D9Interface::SetupSupportedTextureFormats(D3DFORMAT theDisplayFormat) //895-916
{
	sSupportedTextureFormats = 0;
	static int D3DFMT_PixelFormat_Map[6] = { 15 };
	for (int i = 0; i < 6; i += 2)
	{
		if (mD3D->CheckDeviceFormat(0, D3DDEVTYPE_HAL, theDisplayFormat, 0, D3DRTYPE_TEXTURE, (D3DFORMAT)D3DFMT_PixelFormat_Map[i]) >= 0)
			sSupportedTextureFormats |= 1, 26, 2, 23, 4, 0; //?
	}
}

void D3D9Interface::DrawPrimitiveInternal(ulong inPrimType, ulong inPrimCount, const void* inVertData, ulong inVertStride, ulong inVertFormat) //921-1023
{
	DBG_ASSERTE(inPrimCount > 0); //922
	mStateMgr->CommitState();
	bool noDynVB = mGraphicsDriver->mRenderModeFlags & IGraphicsDriver::RENDERMODEF_NoDynVB;
	IGraphicsDriver::ERenderMode aRenderMode = GetEffectiveRenderMode();
	switch (aRenderMode)
	{
	case IGraphicsDriver::RENDERMODE_BatchSize: ulong color = gSexyAppBase->HSLToRGB(inPrimCount >= 150 ? 150 : inPrimCount, 255, 128); mD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, color & 0xFFFFFF | 0x20000000); break;
	case IGraphicsDriver::RENDERMODE_WastedOverdraw:
		mD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCCOLOR);
		mD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_EQUAL);
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAREF, 0);
		break;
	case IGraphicsDriver::RENDERMODE_TextureHash:
		void* ptr = mStateMgr->GetTexture(0);
		uint h = 0x5BD1E995 * ((0xBEEFCAFE * ((uint)ptr ^ 0xBEEFCAFE)) ^ ((0x5BD1E995 * ((uint)ptr ^ 0xBEEFCAFE)) >> 13));
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
	CheckDXError(mD3DDevice->DrawPrimitiveUP((D3DPRIMITIVETYPE)inPrimType, inPrimCount, inVertData, inVertStride), "DrawPrimitiveInternal DrawPrimitiveUP");
	switch (aRenderMode)
	{
	case IGraphicsDriver::RENDERMODE_BatchSize: mD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, mStateMgr->GetRenderState(60)); break;
	case IGraphicsDriver::RENDERMODE_WastedOverdraw:
		mD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, mStateMgr->GetRenderState(19)); //Sexy3DRS?
		mD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, mStateMgr->GetRenderState(20));
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, mStateMgr->GetRenderState(15));
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, mStateMgr->GetRenderState(25));
		mD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAREF, mStateMgr->GetRenderState(26));
		break;
	case IGraphicsDriver::RENDERMODE_TextureHash: mD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, mStateMgr->GetRenderState(60)); break;
	}
}


CD3DEffectStateManager::CD3DEffectStateManager(LPDIRECT3DDEVICE9 inDevice, Buffer* inBuffer) { mDevice = inDevice; mBuffer = inBuffer; mErrorStr = ""; } //1048

void CD3DEffectStateManager::Reset() //1051-1053
{
	mErrorStr.clear();
}
void CD3DEffectStateManager::Log(const std::string &inStr) //1055-1057
{
	//Wow nothing
}
void CD3DEffectStateManager::Error(const std::string& inStr) //1059-1062
{
	if (mErrorStr.empty())
		mErrorStr = inStr;
}

HRESULT CD3DEffectStateManager::QueryInterface(const GUID& iid, void** ppvObject) //1068-1071
{
	std::string inStr = "QueryInterface";
	Log(inStr);
	return 0x80004002;
}
ULONG CD3DEffectStateManager::AddRef() //1073-1077
{
	std::string inStr = "AddRef";
	Log(inStr);
	return mRefCount++;
}
ULONG CD3DEffectStateManager::Release() //1079-1091
{
	std::string inStr = "Release";
	Log(inStr);
	if (mRefCount)
		return --mRefCount;
	else
		delete this;
	return 0;
}

HRESULT CD3DEffectStateManager::LightEnable(DWORD Index, BOOL Enable) //1097-1101
{
	Log(StrFormat("LightEnable: %d, %d", Index, Enable));
	Error("Fixed-function lighting parameters should not be set directly in fx files");
	return NULL;
}
HRESULT CD3DEffectStateManager::SetFVF(DWORD FVF) //1103-1107
{
	Log(StrFormat("SetFVF: %d", FVF));
	Error("Vertex format should not be set directly in fx files");
	return NULL;
}
HRESULT CD3DEffectStateManager::SetLight(DWORD Index, const D3DLIGHT9* pLight) //1109-1113
{
	Log(StrFormat("SetLight: %d", Index));
	Error("Fixed-function lighting parameters should not be set directly in fx files");
	return NULL;
}
HRESULT CD3DEffectStateManager::SetMaterial(const D3DMATERIAL9* pMaterial) //1115-1119
{
	Log("SetMaterial:");
	Error("Fixed-function lighting parameters should not be set directly in fx files");
	return NULL;
}
HRESULT CD3DEffectStateManager::SetNPatchMode(float nSegments) //1121-1125
{
	Log(StrFormat("SetNPatchMode: %f", nSegments));
	Error("NPatchMode is not supported in our framework"); //Awww
	return NULL;
}
HRESULT CD3DEffectStateManager::SetPixelShader(void* inShader) //1127-1131
{
	Log("SetPixelShader:");

	return NULL;
}
HRESULT CD3DEffectStateManager::SetPixelShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT RegisterCount) //1133-1137
{
	Log(StrFormat("SetPixelShaderConstantB: %d, %d", StartRegister, RegisterCount));

	return NULL;
}
HRESULT CD3DEffectStateManager::SetPixelShaderConstantF(UINT StartRegister, const FLOAT* pConstantData, UINT RegisterCount) //1139-1143
{
	Log(StrFormat("SetPixelShaderConstantF: %d, %d", StartRegister, RegisterCount));
	return NULL;
}
HRESULT CD3DEffectStateManager::SetPixelShaderConstantI(UINT StartRegister, const BOOL* pConstantData, UINT RegisterCount) //1145-1149
{
	Log(StrFormat("SetPixelShaderConstantI: %d, %d", StartRegister, RegisterCount));
	return NULL;
}
HRESULT CD3DEffectStateManager::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) //1151-1159
{
	Log(StrFormat("SetRenderState: %d = %d", State, Value));
	mBuffer->WriteLong(D3DRenderEffectDefInfo::StateCommand::CMD_SetRenderState);
	mBuffer->WriteLong(State);
	mBuffer->WriteLong(Value);
	return NULL;
}
HRESULT CD3DEffectStateManager::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) //1161-1170
{
	Log(StrFormat("SetSamplerState: %d[%d]", Type, Value));
	mBuffer->WriteLong(D3DRenderEffectDefInfo::StateCommand::CMD_SetSamplerState);
	mBuffer->WriteLong(Sampler);
	mBuffer->WriteLong(Type);
	mBuffer->WriteLong(Value);
	return NULL;
}
HRESULT CD3DEffectStateManager::SetTexture(DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture) //1172-1176
{
	Log("SetTexture:");
	Error("Textures should not be set directly in fx files");
	return NULL;
}
HRESULT CD3DEffectStateManager::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) //1178-1187
{
	Log(StrFormat("SetTextureStageState: %d[%d] = %d", Type, Stage, Value));
	mBuffer->WriteLong(D3DRenderEffectDefInfo::StateCommand::CMD_SetTextureStageState);
	mBuffer->WriteLong(Stage);
	mBuffer->WriteLong(Type);
	mBuffer->WriteLong(Value);
	return NULL;
}
HRESULT CD3DEffectStateManager::SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) //1189-1193
{
	Log(StrFormat("SetTransform: %d", State));
	Error("Fixed-function transform parameters should not be set directly in fx files");
	return NULL;
}
HRESULT CD3DEffectStateManager::SetVertexShader(LPDIRECT3DVERTEXSHADER9 pShader) //1195-1199
{
	Log("SetVertexShader:");

	return NULL;
}
HRESULT CD3DEffectStateManager::SetVertexShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT RegisterCount) //1201-1205
{
	Log(StrFormat("SetVertexShaderConstantB: %d, %d", StartRegister, RegisterCount));

	return NULL;
}
HRESULT CD3DEffectStateManager::SetVertexShaderConstantF(UINT StartRegister, const FLOAT *pConstantData, UINT RegisterCount) //1207-1211
{
	Log(StrFormat("SetVertexShaderConstantF: %d, %d", StartRegister, RegisterCount));

	return NULL;
}
HRESULT CD3DEffectStateManager::SetVertexShaderConstantI(UINT StartRegister, const BOOL* pConstantData, UINT RegisterCount) //1213-1217
{
	Log(StrFormat("SetVertexShaderConstantI: %d, %d", StartRegister, RegisterCount));

	return NULL;
}

static HMODULE Sexy::GetD3D9XModule() //1221-1224
{
	static HMODULE aModule = LoadLibrary(StrFormat("d3dx9_%d.dll", 35).c_str());
	return aModule;
}

bool D3D9Interface::CompileEffect(const char* inSrcFile, Buffer& outBuffer) //1240-1645
{
	HMODULE aModule = GetD3D9XModule();
	if (!aModule)
		return false;

	typedef HRESULT(WINAPI* FD3DXCreateEffectFromFileW)(LPDIRECT3DDEVICE9 pDevice, LPCWSTR pSrcFile, CONST D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude, DWORD Flags, LPD3DXEFFECTPOOL pPool, LPD3DXEFFECT* ppEffect, LPD3DXBUFFER* ppCompilationErrors);
	static FD3DXCreateEffectFromFileW aFuncD3DXCreateEffectFromFileW = (FD3DXCreateEffectFromFileW)GetProcAddress(aModule, "D3DXCreateEffectFromFileW"); //?

	typedef UINT(WINAPI* FD3DXGetShaderSize)(CONST DWORD* pFunction); //?
	static FD3DXGetShaderSize aFuncD3DXGetShaderSize = (FD3DXGetShaderSize)GetProcAddress(aModule, "D3DXGetShaderSize"); //?

	typedef HRESULT(WINAPI * FD3DXGetShaderConstantTable)(CONST DWORD* pFunction, LPD3DXCONSTANTTABLE* ppConstantTable);
	static FD3DXGetShaderConstantTable aFuncD3DXGetShaderConstantTable = (FD3DXGetShaderConstantTable)GetProcAddress(aModule, "D3DXGetShaderConstantTable"); //?

	ulong aFlags = 65792;
	LPD3DXEFFECT aEffect = NULL;
	LPD3DXBUFFER anError = NULL;
	LPDIRECT3DDEVICE9 aDevice = mD3DDevice;
	HRESULT aResult = aFuncD3DXCreateEffectFromFileW(aDevice, ToWString(inSrcFile).c_str(), NULL, NULL, aFlags, NULL, aEffect, anError);
	if (anError)
		gSexyAppBase->Popup(ToWString(anError->GetBufferPointer()));
	else if (!aResult && aEffect)
	{
		Buffer& aBuffer = outBuffer;
		CD3DEffectStateManager aStateMan(aDevice, outBuffer);
		aEffect->SetStateManager(&aStateMan);
		bool failed = false;
		aBuffer.WriteLong(0x1234FFFF);
		aBuffer.WriteShort(1);
		D3DXEFFECT_DESC aEffectDesc;
		if (aEffect->GetDesc(&aEffectDesc) >= 0)
		{
			aBuffer.WriteShort(aEffectDesc.Techniques);
			for (ulong iTechnique = 0; iTechnique < aEffectDesc.Techniques; iTechnique++)
			{
				const char* aTech = aEffect->GetTechnique(iTechnique);
				if (!aTech)
				{
					failed = true;
					break;
				}
				D3DXTECHNIQUE_DESC aTechDesc;
				if (!aEffect->GetTechniqueDesc(aTech, &aTechDesc) < 0)
				{
					failed = true;
					break;
				}
				aEffect->SetTechnique(aTech);
				uint aActualPasses = 0;
				if (!aEffect->Begin(&aActualPasses, 7) < 0)
				{
					failed = true;
					break;
				}
				aBuffer.WriteString(aTechDesc.Name);
				struct Local
				{
					static bool ProcessConstantTable(LPD3DXCONSTANTTABLE theTable, LPD3D9EFFECT theEffect, Buffer* theBuffer) //1280-1379
					{
						D3DXCONSTANTTABLE_DESC aTableDesc = theTable->GetDesc();
						if (aTableDesc < 0)
							return false;
						theBuffer->WriteShort(aTableDesc.Constants);
						for (ulong iConstant = 0; iConstant < aTableDesc.Constants; iConstant++)
						{
							const char* aConstant = theTable->GetConstant(theTable, 0, iConstant);
							if (!aConstant)
								return false;
							uint aCount = 16;
							D3DXCONSTANT_DESC aConstantDesc[16] = theTable->GetConstantDesc(aConstant, aCount);
							if (aConstantDesc < 0)
								return false;
							for (uint iDesc = 0; iDesc < aCount; ++iDesc)
							{
								D3DXCONSTANT_DESC aDesc = aConstantDesc[iDesc];
								if (aDesc->Class == D3DXPC_STRUCT || aDesc->StructMembers)
								{
									theBuffer->WriteByte(0);
								}
								else
								{
									if (aDesc->RegisterSet == D3DXRS_FLOAT4)
									{
										if (aDesc->Type != D3DXPT_FLOAT)
										{
											gSexyAppBase->Popup("Float registers must only use float data types");
											return false;
										}
										theBuffer->WriteByte(1);
									}
									else
									{
										if (aDesc->RegisterSet != D3DXRS_SAMPLER)
										{
											gSexyAppBase->Popup("Int and Bool constant registers are not permitted in our framework,\nplease use Floats and Samplers only");
											return false;
										}
										if (aDesc->Type != D3DXPT_SAMPLER
											&& aDesc->Type != D3DXPT_SAMPLER1D
											&& aDesc->Type != D3DXPT_SAMPLER2D
											&& aDesc->Type != D3DXPT_SAMPLER3D
											&& aDesc->Type != D3DXPT_SAMPLERCUBE)
										{
											gSexyAppBase->Popup("Sampler registers must only use sampler data types");
											return false;
										}
										if (aDesc->Class != D3DXPC_OBJECT)
										{
											gSexyAppBase->Popup("Sampler registers must only be of the object class");
											return false;
										}
										theBuffer->WriteByte(2);
									}
									std::string aSemantic;
									const char* aParam = theEffect->GetParameterByName(0, aDesc->Name);
									if (aParam != NULL)
									{
										D3DXPARAMETER_DESC aParamDesc = theEffect->GetParameterDesc(aParam, aParamDesc);
										if (aParamDesc < 0)
											return false;
										if (aParamDesc.Semantic)
											aSemantic = aParamDesc.Semantic;
									}
									theBuffer->WriteString(aDesc.Name);
									theBuffer->WriteString(aSemantic);
									theBuffer->WriteShort(aDesc.RegisterIndex);
									theBuffer->WriteByte(aDesc.RegisterCount);
								}
							}
						}
					};
					static bool ProcessAnnotations(const char* theObject, ulong theAnnotCount, LPD3DXEFFECT theEffect, Buffer* theBuffer) //1381-1503
					{
						StringVector aAnnotStringKeys;
						StringVector aAnnotStringValues;
						theBuffer->WriteByte(theAnnotCount);
						for (ulong i = 0; i < theAnnotCount; ++i)
						{
							const char* aAnnot = theEffect->GetAnnotation(theObject, i);
							if (!aAnnot)
								return false;
							D3DXPARAMETER_DESC aDesc = theEffect->GetParameterDesc(theEffect, aAnnot);
							if (!aDesc)
								return false;
							if (aDesc.Elements)
								theBuffer->WriteByte(0);
							else
							{
								switch (aDesc.Type)
								{
								case D3DXPT_BOOL:
								{
									if (aDesc.Class)
										theBuffer->WriteByte(0);
									int aAnnotValue = theEffect->GetBool(aAnnot, aAnnotValue);
									if (!aAnnotValue)
										return false;
									theBuffer->WriteByte(1);
									theBuffer->WriteString(aDesc.Name);
									theBuffer->WriteByte(aAnnotValue);
								}
								case D3DXPT_INT:
								{
									if (aDesc.Class)
										theBuffer->WriteByte(0);
									int aAnnotValue = theEffect->GetInt(aAnnot, aAnnotValue);
									if (!aAnnotValue)
										return false;
									theBuffer->WriteByte(2);
									theBuffer->WriteString(aDesc.Name);
									theBuffer->WriteLong(aAnnotValue);
								}
								case D3DXPT_FLOAT:
								{
									if (aDesc.Class)
									{
										if (aDesc.Class != D3DXPC_VECTOR)
											theBuffer->WriteByte(0);
										D3DXVECTOR4 aAnnotValue = theEffect->GetVector(aAnnot, aAnnotValue);
										if (!aAnnotValue)
											return false;
										theBuffer->WriteByte(4);
										theBuffer->WriteString(aDesc.Name);
										theBuffer->WriteBytes(aAnnotValue, 16);
									}
									else
									{
										float aAnnotValue = theEffect->GetFloat(aAnnot, aAnnotValue);
										if (!aAnnotValue)
											return false;
										theBuffer->WriteByte(3);
										theBuffer->WriteString(aDesc.Name);
										theBuffer->WriteBytes(aAnnotValue, 4);
									}
								}
								case D3DXPT_INT:
								{
									if (aDesc.Class != D3DXPC_OBJECT)
										theBuffer->WriteByte(0);
									const char* aAnnotValue = theEffect->GetString(aAnnot, aAnnotValue);
									if (!aAnnotValue)
										return false;
									theBuffer->WriteByte(5);
									theBuffer->WriteString(aDesc.Name);
									theBuffer->WriteString(aAnnotValue);
									break;
								}
								default: theBuffer->WriteByte(0);
								}
							}
						}
						return true;
					};
				};
				if (!Local::ProcessAnnotations(aTech, aTechDesc.Annotations, aEffect, &aBuffer))
				{
					failed = true;
					break;
				}
				aBuffer.WriteShort(aTechDesc.Passes);
				for (ulong iPass = 0; iPass < aTechDesc.Passes; ++iPass)
				{
					const char* aPass = aEffect->GetPass(aTech, iPass);
					if (!aPass)
					{
						failed = true;
						break;
					}
					D3DXPASS_DESC aPassDesc;
					if (!aEffect->GetTechniqueDesc(aPass, &aPassDesc) < 0)
					{
						failed = true;
						break;
					}
					ulong aVertexShaderSize = aPassDesc.pVertexShaderFunction ? aFuncD3DXGetShaderSize(aPassDesc.pVertexShaderFunction) : 0;
					ulong aPixelShaderSize = aPassDesc.pPixelShaderFunction ? aFuncD3DXGetShaderSize(aPassDesc.pPixelShaderFunction) : 0;
					aBuffer.WriteString(aPassDesc.Name);
					if (!Local::ProcessAnnotations(aPass, aPassDesc.Annotations, aEffect, &aBuffer))
					{
						failed = true;
						break;
					}
					if (iPass < aActualPasses)
					{
						aStateMan.Reset();
						if (aEffect->BeginPass(iPass) < 0)
						{
							failed = true;
							break;
						}
						if (aEffect->CommitChanges() < 0)
						{
							failed = true;
							break;
						}
						if (aEffect->EndPass() < 0)
						{
							failed = true;
							break;
						}
						if (!aStateMan.mErrorStr.empty())
						{
							gSexyAppBase->Popup(StrFormat("Effect Error: %S", &aStateMan.mErrorStr.c_str()));
							failed = true;
							break;
						}
					}
					aBuffer.WriteByte(0);
					aBuffer.WriteShort(aVertexShaderSize);
					if (aVertexShaderSize > 0)
					{
						aBuffer.WriteBytes(aPassDesc.pVertexShaderFunction, aVertexShaderSize);
						LPD3DXCONSTANTTABLE aContextTable = NULL;
						if (aFuncD3DXGetShaderConstantTable(aPassDesc.pVertexShaderFunction, &aConstantTable) < 0)
						{
							failed = true;
							break;
						}
						bool result = ProcessConstantTable(aConstantTable, aEffect, aBuffer);
						aConstantTable->Release();
						if (!result)
						{
							failed = true;
							break;
						}
					}
					aBuffer.WriteShort(aPixelShaderSize);
					if (aPixelShaderSize > 0)
					{
						aBuffer.WriteBytes(aPassDesc.pPixelShaderFunction, aPixelShaderSize);
						LPD3DXCONSTANTTABLE aContextTable = NULL;
						if (aFuncD3DXGetShaderConstantTable(aPassDesc.pPixelShaderFunction, &aConstantTable) < 0)
						{
							failed = true;
							break;
						}
						bool result = ProcessConstantTable(aConstantTable, aEffect, aBuffer);
						aConstantTable->Release();
						if (!result)
						{
							failed = true;
							break;
						}
					}
				}
				if (aEffect->End() < 0)
				{
					failed = false;
					break;
				}
				if (failed)
					break;
			}
			aBuffer.WriteLong(0xFFFF5678);
			while ((aBuffer.GetDataLen() & 3) != 0)
				aBuffer.WriteByte(0);
		}
		else
		{
			failed = true;
		}
		aEffect->SetStateManager(0);
		if (failed)
		{
			gSexyAppBase->Popup("Effect processing failed.");
			delete aStateMan;
		}
		else
		{
			delete aStateMan;
			if (anError)
				anError->Release();
			if (anEffect)
				anEffect->Release();
			return true;
		}
	}
	else
	{
		gSexyAppBase->Popup("Effect compilation failed.");
		return false;
	}
	return false;
}

D3D9Interface::D3D9Interface() //1650-1664
{
	mD3D = NULL;
	mD3DDevice = NULL;
	mFullscreenZBuffer = NULL;

	RtlZeroMemory(&mDeviceCaps, sizeof mDeviceCaps); //?
	mPingPongRenderTarget[0] = 0;
	mPingPongRenderTarget[1] = 0;
	mPingPongMemSurface = 0;
	mPingPongToggle = false;
	mTexMemResQuery = 0;
	RtlZeroMemory(&mTexMemResQueryStats, sizeof mTexMemResQueryStats); //?
}

D3D9Interface::~D3D9Interface() //1669-1671
{
	Cleanup();
}

bool D3D9Interface::InitFromGraphicsDriver(WindowsGraphicsDriver* theDriver, IGraphicsDriver::EResult* outErrorResult, bool preTestOnly, bool* outIsRecommended) //1676-1721
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
	ZeroMemory(mDrawPrimMtx, sizeof mDrawPrimMtx);
	mIsWindowed = mGraphicsDriver->mIsWindowed;
	if (InitD3D(outErrorResult, preTestOnly, outIsRecommended) == NULL)
		return false;
	if (preTestOnly)
		return true;
	D3DDISPLAYMODE d3ddm;
	ZeroMemory(&d3ddm, sizeof d3ddm);
	if (mD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm) >= 0)
	{
		mGraphicsDriver->mIsD3D9 = true;
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

void D3D9Interface::Cleanup() //1726-1774
{
	Cleanup();

	if (mPingPongRenderTarget[0])
	{
		mPingPongRenderTarget[0]->Release();
		mPingPongRenderTarget[0] = NULL;
	}

	if (mPingPongRenderTarget[1])
	{
		mPingPongRenderTarget[1]->Release();
		mPingPongRenderTarget[1] = NULL;
	}

	if (mPingPongMemSurface)
	{
		mPingPongMemSurface->Release();
		mPingPongMemSurface = NULL;
	}

	mPingPongToggle = false;

	if (mTexMemResQuery)
	{
		mTexMemResQuery->Release();
		mTexMemResQuery = 0;
	}

	ZeroMemory(&mTexMemResQueryStats, sizeof mTexMemResQueryStats);

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

bool D3D9Interface::Flush(ulong inFlushFlags) //1779-1893
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
			if (mGraphicsDriver->mRenderModeFlags & IGraphicsDriver::RENDERMODEF_PreventLag)
			{
				bool doPingPong = true;
				if (mPingPongMemSurface != NULL)
				{
					doPingPong = CheckDXError(mD3DDevice->CreateOffscreenPlainSurface(32, 32, mD3DPresentParams.BackBufferFormat, D3DPOOL_SYSTEMMEM, &mPingPongMemSurface, NULL));
				}
				if (!mPingPongRenderTarget[0])
				{
					if (CheckDXError(mD3DDevice->CreateRenderTarget(32, 32, mD3DPresentParams.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, false, &mPingPongRenderTarget[0], NULL)))
						doPingPong = false;
				}
				if (!mPingPongRenderTarget[1])
				{
					if (CheckDXError(mD3DDevice->CreateRenderTarget(32, 32, mD3DPresentParams.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, false, &mPingPongRenderTarget[1], NULL)))
						doPingPong = false;
				}
				if (doPingPong)
				{
					LPDIRECT3DSURFACE9 oldTarget = NULL;
					mD3DDevice->GetRenderTarget(0, &oldTarget);
					mD3DDevice->SetRenderTarget(0, mPingPongRenderTarget[mPingPongToggle]);
					mD3DDevice->Clear(0, NULL, 0, 0, 0.0, 0);
					mD3DDevice->GetRenderTargetData(mPingPongRenderTarget[mPingPongToggle], mPingPongMemSurface);
					mD3DDevice->SetRenderTarget(0, oldTarget);
					oldTarget->Release();
					mPingPongToggle != mPingPongToggle;
				}
			}
			mStateMgr->PopState();
			mD3DDevice->EndScene();
			mSceneBegun = false;
			sErrorString.erase();
			mStateMgr->SetContext(aStateContext);
		}
		mTexMemUsageBytesCurFrame = 0;
		mTexMemUsageFlushRevision++;
		if (mTexMemResQuery)
		{
			D3DDEVINFO_RESOURCEMANAGER aStats;
			mTexMemResQuery->Issue(D3DISSUE_END);
			for (;;)
			{
				HRESULT aResult = mTexMemResQuery->GetData(&aStats, sizeof aStats, D3DGETDATA_FLUSH);
				if (!aResult)
					break;
				if (aResult == D3DERR_DEVICELOST)
				{
					if (mD3DDevice)
						mD3DDevice->EvictManagedResources();
				}
			}
			aStats.stats[3].NumEvicts += this->mTexMemResQueryStats.stats[3].NumEvicts;
			aStats.stats[3].NumVidCreates += this->mTexMemResQueryStats.stats[3].NumVidCreates;
			CopyMemory(&mTexMemResQueryStats, &aStats, sizeof(mTexMemResQueryStats));
		}
	}
	if (inFlushFlags & FLUSHF_ManagedResources_Immediate)
	{
		if (mD3DDevice)
			mD3DDevice->EvictManagedResources();
	}
	else if (inFlushFlags & FLUSHF_ManagedResources_OnPresent)
		mNeedEvictManagedResources = true;
	return false;
}

bool D3D9Interface::Present(const Rect* theSrcRect, const Rect* theDestRect) //1898-1966
{
	HRESULT aResult;
	if (mIsWindowed)
	{
		RECT winSrcRect = theSrcRect->ToRECT();
		RECT winDestRect = theDestRect->ToRECT();
		if ((mGraphicsDriver->mRenderModeFlags & IGraphicsDriver::RENDERMODEF_HalfPresent) && theSrcRect)
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
		if (CheckDXError(mD3DDevice->SetRenderTarget(NULL, NULL), "SetDepthStencilSurface null pre-Reset (Present D3D9)"))
			return false;
		mFullscreenZBuffer->Release();
		mFullscreenZBuffer = false;
	}
	aResult = mD3DDevice->Reset(&mD3DPresentParams); //Prob correct
	mStateMgr->Reset();
	/*if (?) //?
	D3DInterface::Cleanup();
	return false;
	else*/
	if (!mD3DPresentParams.EnableAutoDepthStencil && (mWidth <= mD3DPresentParams.BackBufferWidth ? mWidth = mD3DPresentParams.BackBufferWidth : mWidth = mWidth, mHeight <= mD3DPresentParams.BackBufferHeight ? mHeight = mD3DPresentParams.BackBufferHeight : mHeight = mHeight, CheckDXError(mD3DDevice->CreateDepthStencilSurface(mWidth, mHeight, mD3DPresentParams.AutoDepthStencilFormat, D3DMULTISAMPLE_NONE, 0, false, &mFullscreenZBuffer, NULL))), CheckDXError(mD3DDevice->SetDepthStencilSurface(mFullscreenZBuffer))) //?
	{
		Cleanup();
		return false;
	}
	else
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

ulong D3D9Interface::GetCapsFlags() //1971-1985
{
	ulong aFlags = CAPF_AutoWindowedVSync | CAPF_ImageRenderTargets | CAPF_SingleImageTexture;
	if (mDeviceCaps.PixelShaderVersion / 256.0 >= 2.0)
		aFlags = CAPF_AutoWindowedVSync | CAPF_ImageRenderTargets | CAPF_PixelShaders | CAPF_SingleImageTexture;
	if (mDeviceCaps.VertexShaderVersion / 256.0 >= 2.0)
		aFlags |= CAPF_VertexShaders;
	if ((mDeviceCaps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP) != 0)
		aFlags |= CAPF_CubeMaps;
	if ((mDeviceCaps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP) != 0)
		aFlags |= CAPF_VolumeMaps;
	return aFlags;
}

ulong D3D9Interface::GetMaxTextureStages() //1988-1990
{
	return mDeviceCaps.MaxTextureBlendStages;
}

std::string D3D9Interface::GetInfoString(EInfoString theInfoString) //1995-2133
{
	std::string aFormatStr;
	switch (theInfoString)
	{
	case INFOSTRING_Adapter:
		if (mD3D && mAdapterInfoString.empty())
		{
			D3DADAPTER_IDENTIFIER9 aIdent;
			ZeroMemory(&aIdent, sizeof aIdent);
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
			aFormatStr = "";
			int aBitDepth;
			switch (aMode.Format)
			{
			case D3DFMT_A8R8G8B8: aFormatStr = "A8R8G8B8"; aBitDepth = 32; break;
			case D3DFMT_X8R8G8B8: aFormatStr = "X8R8G8B8"; aBitDepth = 32; break;
			case D3DFMT_R5G6B5: aFormatStr = "R5G6B5"; aBitDepth = 32; break;
			case D3DFMT_X1R5G5B5: aFormatStr = "X1R5G5B5"; aBitDepth = 32; break;
			case D3DFMT_A1R5G5B5: aFormatStr = "A1R5G5B5"; aBitDepth = 32; break;
			case D3DFMT_A2R10G10B10: aFormatStr = "A2R10G10B10"; aBitDepth = 32; break;
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
		aFormatStr = "";
		int aBitDepth;
		switch (mD3DPresentParams.BackBufferFormat)
		{
		case D3DFMT_A8R8G8B8: aFormatStr = "A8R8G8B8"; aBitDepth = 32; break;
		case D3DFMT_X8R8G8B8: aFormatStr = "X8R8G8B8"; aBitDepth = 32; break;
		case D3DFMT_R5G6B5: aFormatStr = "R5G6B5"; aBitDepth = 32; break;
		case D3DFMT_X1R5G5B5: aFormatStr = "X1R5G5B5"; aBitDepth = 32; break;
		case D3DFMT_A1R5G5B5: aFormatStr = "A1R5G5B5"; aBitDepth = 32; break;
		case D3DFMT_A2R10G10B10: aFormatStr = "A2R10G10B10"; aBitDepth = 32; break;
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
	case INFOSTRING_DrvResourceManager:
		if (!mD3DDevice && !mTexMemResQuery)
		{
			return "";
			break;
		}
		D3DRESOURCESTATS* aStats = &mTexMemResQueryStats.stats[3];
		int aWorkingK = mTexMemResQueryStats.stats[3].WorkingSetBytes >> 10;
		if (mTexMemResQueryStats.stats[3].WorkingSetBytes % 1024)
			aWorkingK++;
		int aTotalK = aStats->TotalBytes >> 10;
		if (aStats->TotalBytes % 1024)
			aTotalK++;
		return StrFormat("%s%dkB (%d objects) Working / %dkB (%d objects) Total, VidCreates=%d Evicts=%d", (aStats->bThrashing ? "THRASH " : ""), aWorkingK, aStats->WorkingSet, aTotalK, aStats->TotalManaged, aStats->NumVidCreates, aStats->NumEvicts);
	default:
		return "";
		break;
	}
}

void D3D9Interface::GetBackBufferDimensions(ulong& outWidth, ulong& outHeight) //2138-2141
{
	outWidth = mD3DPresentParams.BackBufferWidth;
	outHeight = mD3DPresentParams.BackBufferHeight;
}

IUnknown* D3D9Interface::CreateSurface(int inWidth, int inHeight, bool inRenderTarget, bool inTexture) //2146-2189
{
	LPDIRECT3DSURFACE9 aSurface = NULL;
	if (inRenderTarget)
		Flush(FLUSHF_ManagedResources_Immediate);
	if (inTexture)
	{
		HRESULT hr = mD3DDevice->CreateTexture(inWidth, inHeight, 1, inRenderTarget, (D3DFORMAT)(mDisplayFormat == SEXY3DFMT_X8R8G8B8 ? D3DFMT_A8R8G8B8 : mDisplayFormat), (D3DPOOL)!inRenderTarget, (LPDIRECT3DTEXTURE9*)aSurface, NULL);
		if (hr < 0)
		{
			Flush(FLUSHF_ManagedResources_Immediate);
			hr = mD3DDevice->CreateTexture(inWidth, inHeight, 1, inRenderTarget, (D3DFORMAT)(mDisplayFormat == SEXY3DFMT_X8R8G8B8 ? D3DFMT_A8R8G8B8 : mDisplayFormat), (D3DPOOL)!inRenderTarget, (LPDIRECT3DTEXTURE9*)aSurface, NULL);
		}
		return hr < 0 ? NULL : aSurface;
	}
	else if (inRenderTarget)
	{
		HRESULT hr = mD3DDevice->CreateRenderTarget(inWidth, inHeight, (D3DFORMAT)mDisplayFormat, D3DMULTISAMPLE_NONE, 0, true, &aSurface, NULL);
		if (hr < 0)
		{
			Flush(FLUSHF_ManagedResources_Immediate);
			hr = mD3DDevice->CreateRenderTarget(inWidth, inHeight, (D3DFORMAT)mDisplayFormat, D3DMULTISAMPLE_NONE, 0, true, &aSurface, NULL);
		}
		return hr < 0 ? NULL : aSurface;
	}
	else
	{
		HRESULT hr = mD3DDevice->CreateOffscreenPlainSurface(inWidth, inHeight, (D3DFORMAT)mDisplayFormat, D3DPOOL_SYSTEMMEM, &aSurface, NULL);
		return hr < 0 ? NULL : aSurface;
	}
}

bool D3D9Interface::CanBltSurface(bool srcSurfaceIsTexture) //2192-2195
{
	return true; //So a useless bool, gotcha
}

void D3D9Interface::BltSurface(IUnknown* theSurface, const Rect& theDest, const Rect& theSrc) //2198-2328
{
	if (!PreDraw())
		return;

	RECT aSrcRect = theSrc.ToRECT();
	RECT aDestRect = theDest.ToRECT();
	D3DSURFACE_DESC aSrcDesc;
	LPDIRECT3DSURFACE9 aSrcSurface = (LPDIRECT3DSURFACE9)theSurface;
	aSrcSurface->GetDesc(&aSrcDesc);
	if (aSrcDesc.Usage & D3DUSAGE_RENDERTARGET)
	{
		LPDIRECT3DSURFACE9 aDestSurface = (LPDIRECT3DSURFACE9)&mCurRenderTargetSurface;
		bool doStretchRect = false;
		LPDIRECT3DTEXTURE9 aSrcTexture = NULL;
		HRESULT aResult = aSrcSurface->GetContainer(IID_IDirect3DTexture9, (void**)&aSrcTexture);
		if (aResult >= 0 && aSrcTexture)
			doStretchRect = false;
		else
		{
			doStretchRect = true;
			aSrcTexture = NULL;
		}
		if (doStretchRect)
			CheckDXError(mD3DDevice->StretchRect(aSrcSurface, &aSrcRect, aDestSurface, &aDestRect, D3DTEXF_LINEAR), "StretchRect");
		else
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

void D3D9Interface::ClearColorBuffer(const Color& inColor) //2333-2348
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

void D3D9Interface::ClearDepthBuffer() //2351-2357
{
	FlushBufferedTriangles();
	mStateMgr->CommitState();

	mD3DDevice->Clear(0, NULL, 2, 0xFF000000, 1.0, 0);
	mNeedClearZBuffer = false;
}

bool D3D9Interface::ReloadEffects() //2362-2403
{
	bool reloaded = false;
	if (!gSexyAppBase->mResourceManager)
		return false;
	ResourceManager::ResMap::iterator it = gSexyAppBase->mResourceManager->mResMaps[ResourceManager::ResType_RenderEffect].begin();
	while (it != gSexyAppBase->mResourceManager->mResMaps[ResourceManager::ResType_RenderEffect].end())
	{
		ResourceManager::RenderEffectRes* aRes = (ResourceManager::RenderEffectRes*)it->second;
		RenderEffectDefinition* aDef = aRes->mRenderEffectDefinition;
		std::string aSrcFile = aDef->mSrcFileName;
		if (!aSrcFile.empty())
		{
			Buffer aBuffer;
			if (CompileEffect(aSrcFile.c_str(), aBuffer))
			{
				aBuffer.SeekFront();
				aDef->LoadFromMem(aBuffer.GetDataLen(), aBuffer.GetDataPtr(), aSrcFile.c_str(), "d3dfx");
				//? it2 = mRenderEffectDefInfo.find(aDef);
				if (it2 != it2.end())
					mRenderEffectDefInfo.erase();

				//? it3 = mRenderEffects.find(aDef);
				if (it3 != it3.end())
					GetEffect(aDef);
				reloaded = true;
			}
		}
		it++;
	}
	return reloaded;
}

//LPDIRECT3DDEVICE9 D3D9Interface::GetRawDevice() { return mD3DDevice; } //Unknown line number. Does this even exist?

static D3DFORMAT Sexy::MakeD3DFORMAT(PixelFormat theFormat) //2408-2420
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

HRESULT D3D9Interface::InternalValidateDevice(ulong* outNumPasses) //2425-2431
{
	if (outNumPasses)
		*outNumPasses = 0;
	if (mD3DDevice)
		mD3DDevice->ValidateDevice(outNumPasses);
	return D3DERR_INVALIDCALL;
}
HRESULT D3D9Interface::InternalCreateVertexShader(const ulong* inFunction, IUnknown** outShader) //2433-2437 (Would replace)
{
	if (mD3DDevice)
		return mD3DDevice->CreateVertexShader(inFunction, (LPDIRECT3DVERTEXSHADER9*)outShader);
	return D3DERR_INVALIDCALL;
}
HRESULT D3D9Interface::InternalCreatePixelShader(const ulong* inFunction, IUnknown** outShader) //2439-2443
{
	return mD3DDevice ? mD3DDevice->CreatePixelShader(inFunction, (LPDIRECT3DPIXELSHADER9*)outShader) : D3DERR_INVALIDCALL;
}
HRESULT D3D9Interface::InternalSetPaletteEntries(uint inPaletteNumber, const PALETTEENTRY* inEntries) //2445-2447
{
	return mD3DDevice->SetPaletteEntries(inPaletteNumber, inEntries);
}
HRESULT D3D9Interface::InternalGetPaletteEntries(uint inPaletteNumber, PALETTEENTRY* inEntries) //2449-2451
{
	return mD3DDevice->SetPaletteEntries(inPaletteNumber, inEntries);
}
HRESULT D3D9Interface::InternalCreateTexture(uint inWidth, uint inHeight, uint inLevels, bool inRenderTarget, PixelFormat inFormat, ulong inPool, IUnknown** outTexture) //2453-2455
{
	return mD3DDevice->CreateTexture(inWidth, inHeight, inLevels, inRenderTarget, MakeD3DFORMAT(inFormat), (D3DPOOL)inPool, (LPDIRECT3DTEXTURE9*)outTexture, NULL);
}
HRESULT D3D9Interface::InternalCreateCubeTexture(uint inEdgeLength, uint inLevels, ulong inUsage, PixelFormat inFormat, ulong inPool, IUnknown** outCubeTexture) //2457-2459
{
	return mD3DDevice->CreateCubeTexture(inEdgeLength, inLevels, inUsage, MakeD3DFORMAT(inFormat), (D3DPOOL)inPool, (LPDIRECT3DCUBETEXTURE9*)outCubeTexture, NULL);
}
HRESULT D3D9Interface::InternalCreateVolumeTexture(uint inWidth, uint inHeight, uint inDepth, uint inLevels, ulong inUsage, PixelFormat inFormat, ulong inPool, IUnknown** outVolumeTexture) //2461-2463
{
	return mD3DDevice->CreateVolumeTexture(inWidth, inHeight, inDepth, inLevels, inUsage, MakeD3DFORMAT(inFormat), (D3DPOOL)inPool, (LPDIRECT3DVOLUMETEXTURE9*)outVolumeTexture, NULL);
}
HRESULT D3D9Interface::InternalUpdateTexture(IUnknown* inSourceTexture, IUnknown* inDestTexture) //2465-2467
{
	return mD3DDevice->UpdateTexture((LPDIRECT3DBASETEXTURE9)inSourceTexture, (LPDIRECT3DBASETEXTURE9)inDestTexture);
}
HRESULT D3D9Interface::InternalCreateImageSurface(uint inWidth, uint inHeight, PixelFormat inFormat, IUnknown** outSurface) //2469-2471
{
	return mD3DDevice->CreateOffscreenPlainSurface(inWidth, inHeight, MakeD3DFORMAT(inFormat), D3DPOOL_SYSTEMMEM, (LPDIRECT3DSURFACE9*)outSurface, NULL);
}
HRESULT D3D9Interface::InternalGetRenderTargetData(IUnknown* inRenderTarget, IUnknown* inDestSurface) //2473-2475
{
	return mD3DDevice->GetRenderTargetData((LPDIRECT3DSURFACE9)inRenderTarget, (LPDIRECT3DSURFACE9)inDestSurface);
}
HRESULT D3D9Interface::InternalSurfaceLockRect(IUnknown* inSurface, int& outPitch, void*& outBits) //2477-2486 (Accurate?)
{
	D3DLOCKED_RECT aLockRect;
	HRESULT hr = ((LPDIRECT3DSURFACE9)inSurface)->LockRect(&aLockRect, NULL, 0);
	if (SUCCEEDED(hr))
		outPitch = aLockRect.Pitch;
	return hr;
}
HRESULT D3D9Interface::InternalSurfaceUnlockRect(IUnknown* inSurface) //2488-2490
{
	return ((LPDIRECT3DSURFACE9)inSurface)->UnlockRect();
}
HRESULT D3D9Interface::InternalTextureGetSurfaceLevel(IUnknown* inTexture, uint inLevel, IUnknown** outSurface) //2492-2494
{
	return ((LPDIRECT3DTEXTURE9)inTexture)->GetSurfaceLevel(inLevel, (LPDIRECT3DSURFACE9*)outSurface);
}
HRESULT D3D9Interface::InternalTextureMakeDirty(IUnknown* inTexture) //2496-2498
{
	return ((LPDIRECT3DTEXTURE9)inTexture)->AddDirtyRect(NULL);
}
HRESULT D3D9Interface::InternalTextureLockRect(IUnknown* inTexture, int& outPitch, void*& outBits) //2500-2509
{
	D3DLOCKED_RECT aLockRect;
	HRESULT hr = ((LPDIRECT3DTEXTURE9)inTexture)->LockRect(0, &aLockRect, NULL, 0);
	if (SUCCEEDED(hr))
		outPitch = aLockRect.Pitch;
	return hr;
}
HRESULT D3D9Interface::InternalTextureUnlockRect(IUnknown* inTexture) //2511-2513
{
	return ((LPDIRECT3DTEXTURE9)inTexture)->UnlockRect(0);
}
HRESULT D3D9Interface::InternalCubeTextureLockRect(IUnknown* inCubeTexture, ulong inFace, int& outPitch, void*& outBits) //2515-2524
{
	D3DLOCKED_RECT aLockRect;
	HRESULT hr = ((LPDIRECT3DCUBETEXTURE9)inCubeTexture)->LockRect((D3DCUBEMAP_FACES)inFace, 0, &aLockRect, NULL, 0);
	if (SUCCEEDED(hr))
		outPitch = aLockRect.Pitch;
	return hr;
}
HRESULT D3D9Interface::InternalCubeTextureUnlockRect(IUnknown* inCubeTexture, ulong inFace) //2526-2528
{
	return ((LPDIRECT3DCUBETEXTURE9)inCubeTexture)->UnlockRect((D3DCUBEMAP_FACES)inFace, 0);
}
HRESULT D3D9Interface::InternalVolumeTextureLockBox(IUnknown* inVolumeTexture, int& outRowPitch, int& outSlicePitch, void*& outBits) //2530-2540
{
	D3DLOCKED_BOX aLockBox;
	HRESULT hr = ((LPDIRECT3DVOLUMETEXTURE9)inVolumeTexture)->LockBox(0, &aLockBox, NULL, 0);
	if (SUCCEEDED(hr))
		outRowPitch = aLockBox.RowPitch;
	return hr;
}
HRESULT D3D9Interface::InternalVolumeTextureUnlockBox(IUnknown* inVolumeTexture) //2542-2544
{
	return ((LPDIRECT3DVOLUMETEXTURE9)inVolumeTexture)->UnlockBox(0);
}
HRESULT D3D9Interface::InternalSetRenderTarget(void* inRenderTargetSurface) //2546-2550 (Would be simple one liner but matching lines)
{
	if (mD3DDevice)
		return mD3DDevice->SetRenderTarget(0, (LPDIRECT3DSURFACE9)inRenderTargetSurface);
	return D3DERR_INVALIDCALL;
}
HRESULT D3D9Interface::InternalBeginScene() //2552-2554
{
	return mD3DDevice->BeginScene();
}
HRESULT D3D9Interface::InternalCreateVertexBuffer(uint inLength, bool inIsDynamic, ulong inFVF, ulong inPool, IUnknown** outVertexBuffer) //2556-2558
{
	return mD3DDevice->CreateVertexBuffer(inLength, inIsDynamic ? 520 : 0, inFVF, (D3DPOOL)inPool, (LPDIRECT3DVERTEXBUFFER9*)outVertexBuffer, NULL);
}
HRESULT D3D9Interface::InternalCreateIndexBuffer(uint inLength, ulong inPool, IUnknown** outIndexBuffer) //2560-2562
{
	return mD3DDevice->CreateIndexBuffer(inLength, 0, D3DFMT_INDEX16, (D3DPOOL)inPool, (LPDIRECT3DINDEXBUFFER9*)outIndexBuffer, NULL);
}
HRESULT D3D9Interface::InternalVertexBufferLock(IUnknown* inVertexBuffer, uint inOffset, uint inSize, void** outData, ulong inLockFlags) //2564-2566
{
	return ((LPDIRECT3DVERTEXBUFFER9)inVertexBuffer)->Lock(inOffset, inSize, outData, inLockFlags);
}
HRESULT D3D9Interface::InternalVertexBufferUnlock(IUnknown* inVertexBuffer) //2568-2570
{
	return ((LPDIRECT3DVERTEXBUFFER9)inVertexBuffer)->Unlock();
}
HRESULT D3D9Interface::InternalIndexBufferLock(IUnknown* inIndexBuffer, uint inOffset, uint inSize, void** outData, ulong inLockFlags) //2572-2574
{
	return ((LPDIRECT3DINDEXBUFFER9)inIndexBuffer)->Lock(inOffset, inSize, outData, inLockFlags);
}
HRESULT D3D9Interface::InternalIndexBufferUnlock(IUnknown* inIndexBuffer) //2576-2578
{
	return ((LPDIRECT3DINDEXBUFFER9)inIndexBuffer)->Unlock();
}
HRESULT D3D9Interface::InternalDrawIndexedPrimitive(ulong inPrimType, uint inMinIndex, uint inNumVertices, uint inStartIndex, uint inPrimCount) //2580-2582
{
	return mD3DDevice->DrawIndexedPrimitive((D3DPRIMITIVETYPE)inPrimType, 0, inMinIndex, inNumVertices, inStartIndex, inPrimCount);
}
HRESULT D3D9Interface::InternalSetStreamSource(uint inStreamNumber, IUnknown* inVertexBuffer, uint inStride) //2584-2586
{
	return mD3DDevice->SetStreamSource(inStreamNumber, (LPDIRECT3DVERTEXBUFFER9)inVertexBuffer, 0, inStride);
}
