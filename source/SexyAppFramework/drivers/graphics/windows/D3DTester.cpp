#include "D3D8Helper.h"
#include "D3DTester.h"
#include "DirectXErrorString.h"
#include "../../../Common.h"
#include "D3DInterface.h"
#include "D3D8Interface.h"
#include "D3D9Interface.h"
#include "../../../SexyAppBase.h"
#include <d3d10.h> //included



using namespace Sexy;
static const int gD3DTestTextureWidth = 64;
static const int gD3DTestTextureHeight = 64;
static bool gD3DTestHas32BitTexture = false;

HMODULE hDll = NULL;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DTestImage::D3DTestImage() //31-35
{
	mBits = NULL;
	mWidth = 0;
	mHeight = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DTestImage::D3DTestImage(int theWidth, int theHeight) //40-46
{
	mBits = NULL;
	mWidth = 0;
	mHeight = 0;	

	Create(theWidth,theHeight);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DTestImage::~D3DTestImage() //52-54
{
	FreeImage();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTestImage::Create(int theWidth, int theHeight) //59-67
{
	FreeImage();
	if(theWidth>0 && theHeight>0)
	{
		mBits = new DWORD[theWidth*theHeight];
		mWidth = theWidth;
		mHeight = theHeight;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTestImage::FreeImage() //72-76
{
	delete [] mBits;
	mWidth = 0;
	mHeight = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
const D3DTestImage& D3DTestImage::operator=(const D3DTestImage &theImage) //81-89
{
	if (&theImage==this)
		return *this;

	Create(theImage.GetWidth(), theImage.GetHeight());
	memcpy(mBits, theImage.GetBits(), mWidth*mHeight*4);

	return *this;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTestImage::CompareEqual(const D3DTestImage &theImage) const //94-102
{
	if (theImage.GetWidth() != GetWidth())
		return false;

	if (theImage.GetHeight() != GetHeight())
		return false;

	return memcmp(theImage.GetBits(),GetBits(),mWidth*mHeight*4)==0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTestImage::FillRect(int x, int y, int theWidth, int theHeight, DWORD theColor) //107-118
{
	DWORD *aRow = mBits + y*mWidth + x;
	for(int j=0; j<theHeight; j++)
	{
		DWORD *aPixel = aRow;
		for(int i=0; i<theWidth; i++)
			*aPixel++ = theColor;

		aRow += mWidth;
	}

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTestImage::MakeVerticalBands() //123-126
{
	for(int i=0; i<mWidth; i++)
		FillRect(i,0,1,mHeight,i&1?0xFFFFFFFF:0xFF000000);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTestImage::CopyToTexture8888(LPDIRECTDRAWSURFACE7 theTexture, int offx, int offy, int texWidth, int texHeight) //131-160
{
	DDSURFACEDESC2 aDesc;
	aDesc.dwSize = sizeof(aDesc);
	D3DTester::CheckDXError(theTexture->Lock(NULL,&aDesc,DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_WRITEONLY,NULL),"Lock Texture");

	int aWidth = min(texWidth,(GetWidth()-offx));
	int aHeight = min(texHeight,(GetHeight()-offy));

	if(aWidth < texWidth || aHeight < texHeight)
		memset(aDesc.lpSurface, 0, aDesc.lPitch*aDesc.dwHeight);

	if(aWidth>0 && aHeight>0)
	{
		DWORD *srcRow = GetBits() + offy * GetWidth() + offx;
		char *dstRow = (char*)aDesc.lpSurface;

		for(int y=0; y<aHeight; y++)
		{
			DWORD *src = srcRow;
			DWORD *dst = (DWORD*)dstRow;
			for(int x=0; x<aWidth; x++)
				*dst++ = *src++;

			srcRow += GetWidth();
			dstRow += aDesc.lPitch;
		}
	}

	D3DTester::CheckDXError(theTexture->Unlock(NULL),"Texture Unlock");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTestImage::CopyToTexture4444(LPDIRECTDRAWSURFACE7 theTexture, int offx, int offy, int texWidth, int texHeight) //165-198
{

	DDSURFACEDESC2 aDesc;
	aDesc.dwSize = sizeof(aDesc);
	D3DTester::CheckDXError(theTexture->Lock(NULL,&aDesc,DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_WRITEONLY,NULL),"Lock Texture");

	int aWidth = min(texWidth,(GetWidth()-offx));
	int aHeight = min(texHeight,(GetHeight()-offy));

	if(aWidth < texWidth || aHeight < texHeight)
		memset(aDesc.lpSurface, 0, aDesc.lPitch*aDesc.dwHeight);

	if(aWidth>0 && aHeight>0)
	{
		DWORD *srcRow = GetBits() + offy * GetWidth() + offx;
		char *dstRow = (char*)aDesc.lpSurface;

		for(int y=0; y<aHeight; y++)
		{
			DWORD *src = srcRow;
			ushort *dst = (ushort*)dstRow;
			for(int x=0; x<aWidth; x++)
			{
				DWORD aPixel = *src++;
				*dst++ = ((aPixel>>16)&0xF000) | ((aPixel>>12)&0x0F00) | ((aPixel>>8)&0x00F0) | ((aPixel>>4)&0x000F);
			}

			srcRow += GetWidth();
			dstRow += aDesc.lPitch;
		}
	}

	D3DTester::CheckDXError(theTexture->Unlock(NULL),"Texture Unlock");
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTestImage::DrawPieceToDevice(LPDIRECT3DDEVICE7 theDevice, LPDIRECTDRAWSURFACE7 theTexture, float x, float y, int offx, int offy, int texWidth, int texHeight, DWORD theColor) //204-227
{
	float maxU = (float)texWidth/gD3DTestTextureWidth;
	float maxV = (float)texHeight/gD3DTestTextureHeight;

	if (gD3DTestHas32BitTexture)
		CopyToTexture8888(theTexture, offx, offy, texWidth, texHeight);
	else
		CopyToTexture4444(theTexture, offx, offy, texWidth, texHeight);

	x -= 0.5f;
	y -= 0.5f;

	D3DTLVERTEX aVertex[4] = 
	{
		{ x,				y,					0,	1,	theColor,	0,	0,		0 },
		{ x,				y+texHeight,		0,	1,	theColor,	0,	0,		maxV },
		{ x+texWidth,		y,					0,	1,	theColor,	0,	maxU,	0 },
		{ x+texWidth,		y+texHeight,		0,	1,	theColor,	0,	maxU,	maxV }
	};
		
	D3DTester::CheckDXError(theDevice->SetTexture(0, theTexture),"SetTexture theTexture");
	D3DTester::CheckDXError(theDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, D3DFVF_TLVERTEX, aVertex, 4, D3DDP_WAIT),"DrawPrimitive");
	D3DTester::CheckDXError(theDevice->SetTexture(0, NULL),"SetTexture NULL");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTestImage::DrawToDevice(LPDIRECT3DDEVICE7 theDevice, LPDIRECTDRAWSURFACE7 theTexture, int x, int y, DWORD theColor) //232-246
{
	int aWidth = GetWidth();
	int aHeight = GetHeight();

	int aTexWidth = min(64,gD3DTestTextureWidth);
	int aTexHeight = min(64,gD3DTestTextureHeight);

	for(int j=0; j<aHeight; j+=aTexHeight)
	{
		for(int i=0; i<aWidth; i+=aTexWidth)
		{
			DrawPieceToDevice(theDevice, theTexture, (float)x+i,(float)y+j,i,j,aTexWidth,aTexHeight,theColor);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int D3DTestImage::ColorDistance(DWORD c1, DWORD c2) //251-261
{
	int r1 = (c1&0xff0000)>>16;
	int g1 = (c1&0x00ff00)>>8;
	int b1 = (c1&0xff);

	int r2 = (c2&0xff0000)>>16;
	int g2 = (c2&0x00ff00)>>8;
	int b2 = (c2&0xff);

	return (r1-r2)*(r1-r2) + (g1-g2)*(g1-g2) + (b1-b2)*(b1-b2);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTestImage::IsUniformColor(DWORD theColor, int &theNumMistakes, int testWidth, int testHeight) const //266-288
{
	theNumMistakes = 0;
	const DWORD *aRow = GetBits();
	DWORD aLastPixel = *aRow;
	bool isUniform = true;
	for(int i=0; i<testHeight; i++)
	{
		const DWORD *aSrc = aRow;
		for(int j=0; j<testWidth; j++)
		{
			DWORD aPixel = *aSrc++;
			if(aLastPixel!=aPixel)
				isUniform = false;

			if(ColorDistance(aPixel,theColor)>COLOR_TOLERANCE)
				theNumMistakes++;
		}

		aRow += GetWidth();
	}

	return isUniform;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int D3DTestImage::CheckUniformBands(int testWidth, int testHeight, int xoff, int yoff) //293-315
{
	int aNumMistakes = 0;
	const DWORD *aRow = GetBits() + yoff*GetWidth() + xoff;
	DWORD aLastPixel = *aRow;
	bool isUniform = true;
	for(int i=0; i<testHeight; i++)
	{
		const DWORD *aSrc = aRow;
		for(int j=0; j<testWidth; j++)
		{
			DWORD aPixel = *aSrc++;
			if(aLastPixel!=aPixel)
				isUniform = false;

			if(ColorDistance(aPixel,(j&1)?0xFFFFFF:0x000000)>COLOR_TOLERANCE)
				aNumMistakes++;
		}

		aRow += GetWidth();
	}

	return aNumMistakes;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static int D3DTestHighBit(DWORD theMask) //320-331
{
	int aBit = 31;
	while(aBit>0)
	{
		if((1<<aBit) & theMask)
			break;

		aBit--;
	}

	return aBit;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <class PixelType> 
static void D3DTestPixelConvert(D3DTestImage &theImage, DDSURFACEDESC2 &theDesc, PixelType) //337-369
{
	int rMask = theDesc.ddpfPixelFormat.dwRBitMask;
	int gMask = theDesc.ddpfPixelFormat.dwGBitMask;
	int bMask = theDesc.ddpfPixelFormat.dwBBitMask;

	int redShift = 23-D3DTestHighBit(rMask);
	int greenShift = 15-D3DTestHighBit(gMask);
	int blueShift = 7-D3DTestHighBit(bMask);

	char *srcRow = (char*)theDesc.lpSurface;
	DWORD *dstRow = theImage.GetBits();
	for(int j=0; j<theImage.GetHeight(); j++)
	{
		PixelType *src = (PixelType*)srcRow;
		DWORD *dst = dstRow;
		for(int i=0; i<theImage.GetWidth(); i++)
		{
			
			PixelType aPixel = *src++;
			int r = aPixel & rMask;
			int g = aPixel & gMask;
			int b = aPixel & bMask;

			if(redShift>0) r<<=redShift; else r>>=-redShift;
			if(greenShift>0) g<<=greenShift; else g>>=-greenShift;
			if(blueShift>0) b<<=blueShift; else b>>=-blueShift;
			*dst++ = 0xFF000000 | r | g | b;
		}

		srcRow += theDesc.lPitch;
		dstRow += theImage.GetWidth();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void D3DTestPixelConvert24(D3DTestImage &theImage, DDSURFACEDESC2 &theDesc)
{
	int rMask = theDesc.ddpfPixelFormat.dwRBitMask;
	int gMask = theDesc.ddpfPixelFormat.dwGBitMask;
	int bMask = theDesc.ddpfPixelFormat.dwBBitMask;

	int redShift = 23-D3DTestHighBit(rMask);
	int greenShift = 15-D3DTestHighBit(gMask);
	int blueShift = 7-D3DTestHighBit(bMask);

	char *srcRow = (char*)theDesc.lpSurface;
	DWORD *dstRow = theImage.GetBits();
	for(int j=0; j<theImage.GetHeight(); j++)
	{
		char *src = srcRow;
		DWORD *dst = dstRow;
		for(int i=0; i<theImage.GetWidth(); i++)
		{
			
			DWORD aPixel = *((DWORD*)src)&0xFFFFFF;
			src += 3;

			int r = aPixel & rMask;
			int g = aPixel & gMask;
			int b = aPixel & bMask;

			if(redShift>0) r<<=redShift; else r>>=-redShift;
			if(greenShift>0) g<<=greenShift; else g>>=-greenShift;
			if(blueShift>0) b<<=blueShift; else b>>=-blueShift;
			*dst++ = 0xFF000000 | r | g | b;
		}

		srcRow += theDesc.lPitch;
		dstRow += theImage.GetWidth();
	}
}

#define SafeSetRenderState(x,y)\
	CheckDXError(mD3DDevice7->SetRenderState(x,y),#x ", " #y)

#define SafeSetTextureStageState(i,x,y)\
	CheckDXError(mD3DDevice7->SetTextureStageState(i,x,y),#x ", " #y)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DTester::D3DTester() //419-435
{
	mDisplayDriver = "";
	mDisplayDescription = "";
	D3DTestImage mTestImage;
	mFailureReason = "";
	mWarning = "";

	mDD7 = NULL;
	mPrimarySurface = NULL;
	mTextureSurface = NULL;
	mTextureSurface2 = NULL;
	mD3D7 = NULL;
	mD3DDevice7 = NULL;
	mRegKey = NULL;
	memset(&mDisplayGUID, 0, sizeof(GUID));

	mMinVidMemory = 0;
	mRecommendedVidMemory = 0;
	mDriverYear = 0;

	mCheckRegistry = true;
	mResultsChanged = false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DTester::~D3DTester() //440-442
{
	Cleanup();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::CheckRegistry() //427-527 (Probably commented code)
{
	mResultsChanged = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTester::WriteToRegistry() //532-570
{
	DWORD aSize, aType;

	if (mRegKey==NULL)
		return;

	// Write Test Version
	DWORD aVersion = TEST_VERSION;
	aSize = sizeof(aVersion);
	aType = REG_DWORD;
	RegSetValueExA(mRegKey, "Version", 0, aType, (uchar*) &aVersion, aSize);

	// Write Min Vid Memory
	DWORD aMinVidMemory = mMinVidMemory;
	aSize = sizeof(aMinVidMemory);
	aType = REG_DWORD;
	RegSetValueExA(mRegKey, "MinVidMemory", 0, aType, (uchar*) &aMinVidMemory, aSize);

	// Write Recommended Vid Memory
	DWORD aRecVidMemory = mRecommendedVidMemory;
	aSize = sizeof(aRecVidMemory);
	aType = REG_DWORD;
	RegSetValueExA(mRegKey, "RecVidMemory", 0, aType, (uchar*) &aRecVidMemory, aSize);

	// Write GUID
	aSize = sizeof(mDisplayGUID);
	aType = REG_BINARY;
	RegSetValueExA(mRegKey, "DisplayGUID", 0, aType, (uchar*) &mDisplayGUID, aSize);

	// Write failure reason
	aType = REG_SZ;
	aSize = mFailureReason.length()+1;
	RegSetValueExA(mRegKey, "FailureReason", 0, aType, (uchar*) mFailureReason.c_str(), aSize);

	// Write warining
	aType = REG_SZ;
	aSize = mWarning.length()+1;
	RegSetValueExA(mRegKey, "Warning", 0, aType, (uchar*) mWarning.c_str(), aSize);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::FileContains(FILE* theFile, const char* theString) //575-592
{
	bool found = false;
	char aBuf[4096];
	while (!feof(theFile))
	{
		if (fgets(aBuf,4000,theFile)==NULL)
			break;

		std::string aStr = Trim(aBuf);
		if (!aStr.empty() && StrFindNoCase(theString,aStr.c_str()) >= 0)
		{
			found = true;
			break;
		}
	}
	
	return found;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::IsSupportedCard(const char *theDisplayDesc) //597-630
{
	// Look for 'bad' exception list
	FILE* aFile = fopen("vhwb.dat","r");
	if (aFile != NULL)
	{
		bool found = FileContains(aFile, theDisplayDesc);
		fclose(aFile);
		if (found)
			return false;
	}

	// Look for 'good' supported list
	aFile = fopen("vhw.dat","r");
	if (aFile==NULL) // default checks
	{
		if (mDriverYear>=2002)
			return true;

		if (StrFindNoCase(theDisplayDesc,"nvidia") >= 0)
			return true;

		if (StrFindNoCase(theDisplayDesc,"radeon") >= 0)
			return true;

		if (StrFindNoCase(theDisplayDesc,"ati ") >= 0)
			return true;
	
		return false;
	}

	bool found = FileContains(aFile, theDisplayDesc);
	fclose(aFile);
	return found;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::Init(HWND theHWND, LPDIRECTDRAW7 theDDraw) //635-811
{
	Cleanup();
	mDriverYear = 0;

	if (mCheckRegistry)
	{
		std::string aKey = RemoveTrailingSlash("SOFTWARE\\" + gSexyAppBase->mRegKey) + "\\Test3D";
		RegCreateKeyExA(HKEY_CURRENT_USER, aKey.c_str(),0,"",REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&mRegKey,NULL);
	}

	try
	{

		if (theDDraw==NULL)
		{
			extern HMODULE gDDrawDLL;

			typedef HRESULT (WINAPI *DirectDrawCreateExFunc)(GUID FAR *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkOuter);
			DirectDrawCreateExFunc aDirectDrawCreateExFunc = (DirectDrawCreateExFunc)GetProcAddress(gDDrawDLL,"DirectDrawCreateEx");
			if (aDirectDrawCreateExFunc == NULL)							
				return Fail("No DirectDrawCreateEx"); 			

			CheckDXError(aDirectDrawCreateExFunc(NULL, (LPVOID*)&mDD7, IID_IDirectDraw7, NULL),"DirectDrawCreateEx");
		}
		else
		{
			theDDraw->AddRef();
			mDD7 = theDDraw;
		}

		if (!GetD3D8AdapterInfo(mDisplayGUID,mDisplayDriver,mDisplayDescription))
		{
			// Get Device GUID
			DDDEVICEIDENTIFIER2 aDeviceInfo;
			CheckDXError(mDD7->GetDeviceIdentifier(&aDeviceInfo,0), "GetDeviceIdentifier");
			mDisplayGUID = aDeviceInfo.guidDeviceIdentifier;
			mDisplayDriver = aDeviceInfo.szDriver;
			mDisplayDescription = aDeviceInfo.szDescription;
		}

		// Test Video Memory
		DWORD dwTotal, dwFree; 
		DDSCAPS2 ddsCaps;
		ZeroMemory(&ddsCaps,sizeof(ddsCaps));
		ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
	 
		HDC aDC = GetDC(NULL);
		int aWidth = GetDeviceCaps(aDC, HORZRES);
		int aHeight = GetDeviceCaps(aDC, VERTRES);
		int aBPP = GetDeviceCaps( aDC, BITSPIXEL );
		ReleaseDC(NULL, aDC);

		HRESULT aResult =  mDD7->GetAvailableVidMem(&ddsCaps, &dwTotal, &dwFree);
		if (!SUCCEEDED(aResult))
		{
			Warn(StrFormat("GetAvailableVidMem failed: %s",GetDirectXErrorString(aResult).c_str()));
		}
		else
		{
			dwTotal += (aBPP/8)*aWidth*aHeight;
			dwTotal /= (1024*1024);
			if (dwTotal < mMinVidMemory)
				return Fail("Not enough video memory.");
			else if (dwTotal < mRecommendedVidMemory)
				Warn("Low video memory.");
		}

		// Check registry to see if we've already done the test
		if (mCheckRegistry)
		{
			if (CheckRegistry())
				return false;

			mShouldWriteToRegistry = true;
		}

		mFailureReason = "";
		mWarning = "";

		// Get date on driver dll
		std::string aPath = mDisplayDriver;
		if (aPath.find_first_of("/\\")==std::string::npos)
		{
			char aBuf[_MAX_PATH+1];
			if (GetSystemDirectoryA(aBuf,sizeof(aBuf)-1))
				aPath = AddTrailingSlash(aBuf,true) + aPath;
		}

		FILETIME aFileTime;
		memset(&aFileTime, 0, sizeof(aFileTime));
		HANDLE aFileHandle = CreateFileA(aPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (aFileHandle != INVALID_HANDLE_VALUE)
		{				
			SYSTEMTIME aSystemTime;
			if (GetFileTime(aFileHandle, NULL, NULL, &aFileTime) && FileTimeToSystemTime(&aFileTime,&aSystemTime))
				mDriverYear = aSystemTime.wYear;

			CloseHandle(aFileHandle);
		}

		// Check supported cards
		if (!IsSupportedCard(mDisplayDescription.c_str()))
			Warn(StrFormat("Unsupported video card: %s",mDisplayDescription.c_str()));

		// Get Direct3D7 to test 3d capabilities
		CheckDXError(mDD7->QueryInterface(IID_IDirect3D7, (LPVOID*)&mD3D7),"QueryInterface IID_IDirect3D7"); 

		CheckDXError(mDD7->SetCooperativeLevel(theHWND, DDSCL_NORMAL),"SetCooperativeLevel");

		// Create Primary Surface for test rendering
		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags  = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
		ddsd.dwWidth = 100;
		ddsd.dwHeight = 100;
		CheckDXError(mDD7->CreateSurface(&ddsd, &mPrimarySurface, NULL),"CreateSurface (Primary)");
		mTestImage.Create(ddsd.dwWidth,ddsd.dwHeight);


		CheckDXError(mD3D7->CreateDevice(IID_IDirect3DHALDevice, mPrimarySurface, &mD3DDevice7),"CreateDevice");

		DWORD aFormat = 0;
		CheckDXError(mD3DDevice7->EnumTextureFormats(PixelFormatsCallback,&aFormat),"EnumTextureFormats");
		if (!(aFormat & PixelFormat_A8R8G8B8))
		{
			Warn("A8R8G8B8 texture format not supported.");
			if (!(aFormat & PixelFormat_A4R4G4B4))
				return Fail("A4R4G4B4 and A8R8G8B8 texture formats not supported.");

			gD3DTestHas32BitTexture = false;
		}
		else
			gD3DTestHas32BitTexture = true;

		// Create Texture Surface
		DDSURFACEDESC2 aDesc;
		ZeroMemory(&aDesc, sizeof(aDesc));
		aDesc.dwSize = sizeof(aDesc);	
		aDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
		aDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;	
		aDesc.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;

		aDesc.dwWidth = 64;
		aDesc.dwHeight = 64;	

		aDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		aDesc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;

		if (gD3DTestHas32BitTexture)
		{
			aDesc.ddpfPixelFormat.dwRGBBitCount = 32;
			aDesc.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
			aDesc.ddpfPixelFormat.dwRBitMask		= 0x00FF0000;
			aDesc.ddpfPixelFormat.dwGBitMask		= 0x0000FF00;
			aDesc.ddpfPixelFormat.dwBBitMask		= 0x000000FF;
		}
		else
		{
			aDesc.ddpfPixelFormat.dwRGBBitCount = 16;
			aDesc.ddpfPixelFormat.dwRGBAlphaBitMask = 0xF000;
			aDesc.ddpfPixelFormat.dwRBitMask		= 0x0F00;
			aDesc.ddpfPixelFormat.dwGBitMask		= 0x00F0;
			aDesc.ddpfPixelFormat.dwBBitMask		= 0x000F;
		}

		CheckDXError(mDD7->CreateSurface(&aDesc, &mTextureSurface, NULL), "CreateSurface (TextureSurface1)");
		CheckDXError(mDD7->CreateSurface(&aDesc, &mTextureSurface2, NULL), "CreateSurface (TextureSurfacd2)"); //Lol
	}
	catch(TestException &ex)
	{
		return Fail(ex.mMsg);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
HRESULT CALLBACK D3DTester::PixelFormatsCallback(LPDDPIXELFORMAT theFormat, LPVOID lpContext) //TODO changed | 816-865
{
	struct Local
	{
		static PixelFormat GetDDPixelFormat(LPDDPIXELFORMAT theFormat) //820-858
		{
			if (theFormat->dwFlags == (DDPF_ALPHAPIXELS | DDPF_RGB) &&
				theFormat->dwRGBBitCount == 32 &&
				theFormat->dwRGBAlphaBitMask == 0xFF000000 &&
				theFormat->dwRBitMask == 0x00FF0000 &&
				theFormat->dwGBitMask == 0x0000FF00 &&
				theFormat->dwBBitMask == 0x000000FF)
			{
				return PixelFormat_A8R8G8B8;
			}

			if (theFormat->dwFlags == (DDPF_ALPHAPIXELS | DDPF_RGB) &&
				theFormat->dwRGBBitCount == 16 &&
				theFormat->dwRGBAlphaBitMask == 0xF000 &&
				theFormat->dwRBitMask == 0x0F00 &&
				theFormat->dwGBitMask == 0x00F0 &&
				theFormat->dwBBitMask == 0x000F)
			{
				return PixelFormat_A4R4G4B4;
			}

			if (theFormat->dwFlags == DDPF_RGB &&
				theFormat->dwRGBBitCount == 16 &&
				theFormat->dwRGBAlphaBitMask == 0x0000 &&
				theFormat->dwRBitMask == 0xF800 &&
				theFormat->dwGBitMask == 0x07E0 &&
				theFormat->dwBBitMask == 0x001F)
			{
				return PixelFormat_R5G6B5;
			}

			if (theFormat->dwFlags == (DDPF_RGB | DDPF_PALETTEINDEXED8) &&
				theFormat->dwRGBBitCount == 8)
			{
				return PixelFormat_Palette8;
			}

			return PixelFormat_Unknown;
		};
	};

	*((DWORD*)lpContext) |= Local::GetDDPixelFormat(theFormat);
	
	return D3DENUMRET_OK; 
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTester::Cleanup() //870-913
{	
	if (mDD7)
	{
		mDD7->Release();
		mDD7 = NULL;
	}

	if (mD3D7)
	{
		mD3D7->Release();
		mD3D7 = NULL;
	}

	if (mD3DDevice7)
	{
		mD3DDevice7->Release();
		mD3DDevice7 = NULL;
	}

	if (mPrimarySurface)
	{
		mPrimarySurface->Release();
		mPrimarySurface = NULL;
	}

	if (mTextureSurface)
	{
		mTextureSurface->Release();
		mTextureSurface = NULL;
	}

	if (mTextureSurface2)
	{
		mTextureSurface2->Release();
		mTextureSurface2 = NULL;
	}

	if (mRegKey)
	{
		RegCloseKey(mRegKey);
		mRegKey = NULL;
	}

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::Fail(const std::string &theStr) //918-921
{
	mFailureReason = theStr;
	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::Warn(const std::string &theStr) //926-929
{
	mWarning = theStr;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTester::CheckDXError(HRESULT theResult, const char *theMsg) //934-944
{
	if (FAILED(theResult))
	{
		std::string aMsg = "DXError - ";
		aMsg += theMsg;
		aMsg += ": ";
		aMsg += GetDirectXErrorString(theResult);

		throw TestException(aMsg);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTester::CopyPrimaryToTestImage() //949-974
{
	DDBLTFX aBltFX;
	ZeroMemory(&aBltFX, sizeof(aBltFX));
	aBltFX.dwSize = sizeof(aBltFX);    	

	DDSURFACEDESC2 aDesc;
	memset(&aDesc, 0, sizeof(aDesc));
	aDesc.dwSize = sizeof(aDesc);

	D3DTester::CheckDXError(mPrimarySurface->Lock(NULL,&aDesc,DDLOCK_WAIT,NULL),"CopyPrimary Lock");

	if(aDesc.ddpfPixelFormat.dwRGBBitCount==32)
		D3DTestPixelConvert<DWORD>(mTestImage,aDesc,0);
	else if(aDesc.ddpfPixelFormat.dwRGBBitCount==16)
		D3DTestPixelConvert<unsigned short>(mTestImage,aDesc,0);
	else if(aDesc.ddpfPixelFormat.dwRGBBitCount==24)
		throw TestException("Can't test 24-bit mode.");
//		D3DTestPixelConvert24(mTestImage,aDesc);
	else
	{
		mPrimarySurface->Unlock(NULL);
		throw TestException("Invalid Color Depth");
	}

	mPrimarySurface->Unlock(NULL);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::TestAlphaBlend() //980-1020
{
	try
	{
		CheckDXError(mD3DDevice7->BeginScene());
		CheckDXError(mD3DDevice7->Clear(0, NULL, D3DCLEAR_TARGET ,0xff000000, 1.0f, 0L),"Clear");

		SafeSetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
		SafeSetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA);
		SafeSetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
		SafeSetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);

		D3DTestImage anImage(64,64);

		anImage.FillRect(0,0,64,64,0xFF0000FF);
		anImage.DrawToDevice(mD3DDevice7, mTextureSurface, 0, 0); 

		anImage.FillRect(0,0,64,64,0x80FF0000);
		anImage.DrawToDevice(mD3DDevice7, mTextureSurface, 0, 0); 

		mD3DDevice7->EndScene();

		CopyPrimaryToTestImage();
	}
	catch(TestException &ex)
	{
		mD3DDevice7->EndScene();
		return Fail(ex.mMsg);
	}

	int aNumErrors = 0;
	bool isUniform = mTestImage.IsUniformColor(0x7f007f,aNumErrors,10,10);
	if (aNumErrors==0)
	{
		if (isUniform)
			return true;
		else
			return Warn("Alpha blend test not uniform.");
	}
	else
		return Fail("Alpha blend Not Supported");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::TestAdditiveBlend() //1025-1065
{
	try
	{
		CheckDXError(mD3DDevice7->BeginScene());
		CheckDXError(mD3DDevice7->Clear(0, NULL, D3DCLEAR_TARGET ,0xff000000, 1.0f, 0L),"Clear");

		SafeSetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
		SafeSetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_ONE);
		SafeSetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
		SafeSetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);

		D3DTestImage anImage(64,64);

		anImage.FillRect(0,0,64,64,0xFF404040);
		anImage.DrawToDevice(mD3DDevice7, mTextureSurface, 0, 0); 

		anImage.FillRect(0,0,64,64,0xFF404040);
		anImage.DrawToDevice(mD3DDevice7, mTextureSurface, 0, 0); 

		mD3DDevice7->EndScene();

		CopyPrimaryToTestImage();
	}
	catch(TestException &ex)
	{
		mD3DDevice7->EndScene();
		return Fail(ex.mMsg);
	}

	int aNumErrors = 0;
	bool isUniform = mTestImage.IsUniformColor(0x808080,aNumErrors,10,10);
	if(aNumErrors==0)
	{
		if (isUniform)
			return true;
		else
			return Warn("Additive blend test not uniform.");
	}
	else
		return Fail("Additive blend not supported.");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::TestAlphaAddBlend() //1070-1110
{
	try
	{
		CheckDXError(mD3DDevice7->BeginScene());
		CheckDXError(mD3DDevice7->Clear(0, NULL, D3DCLEAR_TARGET ,0xff000000, 1.0f, 0L),"Clear");

		SafeSetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
		SafeSetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA);
		SafeSetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
		SafeSetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);

		D3DTestImage anImage(64,64);

		anImage.FillRect(0,0,64,64,0xFF404040);
		anImage.DrawToDevice(mD3DDevice7, mTextureSurface, 0, 0); 

		anImage.FillRect(0,0,64,64,0x80808080);
		anImage.DrawToDevice(mD3DDevice7, mTextureSurface, 0, 0); 

		mD3DDevice7->EndScene();

		CopyPrimaryToTestImage();
	}
	catch(TestException &ex)
	{
		mD3DDevice7->EndScene();
		return Fail(ex.mMsg);
	}

	int aNumErrors = 0;
	bool isUniform = mTestImage.IsUniformColor(0x808080,aNumErrors,10,10);
	if(aNumErrors==0)
	{
		if (isUniform)
			return true;
		else
			return Warn("AlphaAdd blend test not uniform.");
	}
	else
		return Fail("AlphaAdd blend not supported.");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::TestAlphaModulate() //1115-1157
{
	try
	{
		CheckDXError(mD3DDevice7->BeginScene());
		CheckDXError(mD3DDevice7->Clear(0, NULL, D3DCLEAR_TARGET ,0xff000000, 1.0f, 0L),"Clear");

		SafeSetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
		SafeSetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA);
		SafeSetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
		SafeSetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);

		SafeSetTextureStageState(0,D3DTSS_ALPHAOP, D3DTOP_MODULATE);

		D3DTestImage anImage(64,64);

		anImage.FillRect(0,0,64,64,0xFF0000FF);
		anImage.DrawToDevice(mD3DDevice7, mTextureSurface, 0, 0); 

		anImage.FillRect(0,0,64,64,0xFFFF0000);
		anImage.DrawToDevice(mD3DDevice7, mTextureSurface, 0, 0, 0x80FFFFFF); 

		mD3DDevice7->EndScene();

		CopyPrimaryToTestImage();
	}
	catch(TestException &ex)
	{
		mD3DDevice7->EndScene();
		return Fail(ex.mMsg);
	}

	int aNumErrors = 0;
	bool isUniform = mTestImage.IsUniformColor(0x7f007f,aNumErrors,10,10);
	if(aNumErrors==0)
	{
		if (isUniform)
			return true;
		else
			return Warn("AlphaModulated blend test not uniform.");
	}
	else
		return Fail("AlphaModulated blend not supported.");
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::TestClipProblem() //1161-1191
{
	try
	{
		CheckDXError(mD3DDevice7->BeginScene());
		CheckDXError(mD3DDevice7->Clear(0, NULL, D3DCLEAR_TARGET ,0xff000000, 1.0f, 0L),"Clear");

		SafeSetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
		SafeSetRenderState(D3DRENDERSTATE_SRCBLEND,  D3DBLEND_SRCALPHA);
		SafeSetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
		SafeSetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);

		D3DTestImage anImage(64,64);
		anImage.MakeVerticalBands();
		anImage.DrawToDevice(mD3DDevice7, mTextureSurface, -2, -2); 

		mD3DDevice7->EndScene();

		CopyPrimaryToTestImage();
	}
	catch(TestException &ex)
	{
		mD3DDevice7->EndScene();
		return Fail(ex.mMsg);
	}

	int aNumErrors = mTestImage.CheckUniformBands(62,62);
	if (aNumErrors==0)
		return true;
	else 
		return Warn("Clip problem detected.");
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::TestD3DInterfaces(WindowsGraphicsDriver* theDriver) //Correct? | 1195-1230
{
	D3DInterface* aD3DInterface = CreateD3D9Interface();
	bool aIsRecommended = true;
	bool aPassedPreTest = aD3DInterface->InitFromGraphicsDriver(theDriver, 0, 1, &aIsRecommended);
	delete aD3DInterface;
	try
	{
		if (aPassedPreTest)
		{
			bool is3D = false;
			bool is3DOptionSet = gSexyAppBase->RegistryReadBoolean("Is3D", &is3D);
			if (is3DOptionSet && is3D || gSexyAppBase->sAttemptingNonRecommended3D)
				aIsRecommended = true;
			if (aIsRecommended)
				return true;
		}
		aD3DInterface = CreateD3D8Interface();
		bool aIsRecommended = true;
		bool aPassedPreTest = aD3DInterface->InitFromGraphicsDriver(theDriver, 0, 1, &aIsRecommended);
		delete aD3DInterface;
		if (aPassedPreTest)
		{
			bool is3D = false;
			bool is3DOptionSet = gSexyAppBase->RegistryReadBoolean("Is3D", &is3D);
			if (is3DOptionSet && is3D ||SexyAppBase::sAttemptingNonRecommended3D) //So they used gSexyAppBase here but why not now
				aIsRecommended = true;
			if (aIsRecommended)
				return true;
		}
	}
	catch (TestException& ex)
	{
		if (!aIsRecommended)
			return Warn("Direct3D not recommended");
		else
			return Fail("Direct3D 8 and 9 unavailable");
	}
}

#undef SafeSetRenderState
#undef SafeSetTextureStageState
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DTester::DoTest(WindowsGraphicsDriver *theDriver) //Changed | 1237-1257
{
	if (!TestAlphaBlend())
		return false;

	if (!TestAdditiveBlend())
		return false;

	if (!TestAlphaAddBlend())
		return false;

	if (!TestAlphaModulate())
		return false;

	if (!TestClipProblem())
		return TestD3DInterfaces(theDriver);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTester::SetVidMemoryConstraints(DWORD theMin, DWORD theRecommended) //1262-1265
{
	mMinVidMemory = theMin;
	mRecommendedVidMemory = theRecommended;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTester::TestD3D(HWND theHWND, LPDIRECTDRAW7 theDDraw, WindowsGraphicsDriver *theDriver) //1270-1289 (Demo code is gone)
{
	mShouldWriteToRegistry = false;
	if (Init(theHWND, theDDraw))
	{
		DoTest(theDriver);
	}
	else
	{
		if ((mCheckRegistry) && (!mShouldWriteToRegistry))
		{
			mResultsChanged = RegQueryValueExA(mRegKey, "FailureReason", 0, NULL, NULL, NULL) != ERROR_SUCCESS;
			mShouldWriteToRegistry = true;
		}
	}

	if (mShouldWriteToRegistry)
		WriteToRegistry();

	Cleanup();
}

int D3DTester::StaticGetVideoMemoryMB() //Correct? | 1294-1417
{
	hDll = LoadLibraryA("d3d9.dll");
	if (hDll == NULL)
		return 0;
	typedef HRESULT (WINAPI* Direct3DCreate9ExFunc)(UINT SDKVersion, LPDIRECT3D9EX*); //?
	Direct3DCreate9ExFunc aDirect3DCreate9ExFunc = (Direct3DCreate9ExFunc)GetProcAddress(hDll, "Direct3DCreate9Ex");
	if (aDirect3DCreate9ExFunc == NULL)
		return 0;
	hDll = LoadLibraryA("d3d10.dll"); //Huh
	if (hDll == NULL)
		return 0;
	typedef HRESULT(WINAPI* D3D10CreateDeviceFunc)(IDXGIAdapter* pAdapter, D3D10_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, UINT SDKVersion, ID3D10Device** ppDevice); //?
	D3D10CreateDeviceFunc aD3D10CreateDeviceFunc = (D3D10CreateDeviceFunc)GetProcAddress(hDll, "D3D10CreateDevice");
	if (aD3D10CreateDeviceFunc == NULL)
	{
		FreeLibrary(hDll);
		return 0;
	}
	ID3D10Device* pD3D10Device = NULL;
	if (!SUCCEEDED(aD3D10CreateDeviceFunc(0, D3D10_DRIVER_TYPE_HARDWARE, 0, 0, 29, &pD3D10Device)))
	{
		FreeLibrary(hDll);
		return 0;
	}
	IDXGIDevice* pDXGIDevice = NULL;
	if (!SUCCEEDED(pD3D10Device->QueryInterface(__uuidof(IDXGIDevice), (void**) &pDXGIDevice)))
	{
		pD3D10Device->Release();
		FreeLibrary(hDll);
		return 0;
	}
	IDXGIAdapter* pDXGIAdapter = NULL;
	if (!SUCCEEDED(pDXGIDevice->GetAdapter(&pDXGIAdapter)))
	{
		pDXGIDevice->Release();
		pD3D10Device->Release();
		FreeLibrary(hDll);
		return 0;
	}
	DXGI_ADAPTER_DESC aAdapterDesc;
	HRESULT hr = pDXGIAdapter->GetDesc(&aAdapterDesc);
	pDXGIAdapter->Release();
	pDXGIDevice->Release();
	pD3D10Device->Release();
	if (hr >= 0)
		return aAdapterDesc.DedicatedVideoMemory >> 20;
	else
		return 0;
}