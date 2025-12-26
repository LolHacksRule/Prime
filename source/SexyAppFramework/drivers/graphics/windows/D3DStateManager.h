#ifndef __D3DSTATEMANAGER_H__
#define __D3DSTATEMANAGER_H__

#include "../../../Graphics.h"
#include "../../../RenderStateManager.h"

namespace Sexy
{
	class D3DStateManager : public RenderStateManager
	{
	public:
		enum EStateGroup
		{
			SG_RS,
			SG_TSS,
			SG_SS,
			SG_LIGHT,
			SG_MATERIAL,
			SG_STREAM,
			SG_TRANSFORM,
			SG_VIEWPORT,
			SG_MISC,
			SG_COUNT,
		};
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		enum ED3DStateGroup //Custom stategroup enum from XNA
		{
			SG_BLEND,
			SG_Raster,
			SG_Depth,
			SG_Sampler,
			SG_Project,
			SG_View,
			SG_World,
			SG_ViewPort,
			SG_Num
		};
#endif
		enum //ERenderStateConst in XNA
		{
			ST_COUNT_RS = 256,
			ST_COUNT_TSS = 48,
			ST_COUNT_SS = 16,
			ST_COUNT_TRANSFORM = 512
		};
		enum ELightState
		{
			ST_LIGHT_ENABLED,
			ST_LIGHT_TYPE,
			ST_LIGHT_DIFFUSE,
			ST_LIGHT_SPECULAR,
			ST_LIGHT_AMBIENT,
			ST_LIGHT_POSITION,
			ST_LIGHT_DIRECTION,
			ST_LIGHT_RANGE,
			ST_LIGHT_FALLOFF,
			ST_LIGHT_ATTENUATION,
			ST_LIGHT_ANGLES,
			ST_COUNT_LIGHT,
		};
		enum EMaterialState
		{
			ST_MAT_DIFFUSE,
			ST_MAT_AMBIENT,
			ST_MAT_SPECULAR,
			ST_MAT_EMISSIVE,
			ST_MAT_POWER,
			ST_COUNT_MAT,
		};
		enum EStreamState
		{
			ST_STREAM_DATA,
			ST_STREAM_OFFSET,
			ST_STREAM_STRIDE,
			ST_STREAM_FREQ,
			ST_COUNT_STREAM,
		};
		enum EViewportState
		{
			ST_VIEWPORT_X,
			ST_VIEWPORT_Y,
			ST_VIEWPORT_WIDTH,
			ST_VIEWPORT_HEIGHT,
			ST_VIEWPORT_MINZ,
			ST_VIEWPORT_MAXZ,
			ST_COUNT_VIEWPORT,
		};
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		enum EScissorState
		{
			ST_SCISSOR_ENABLE,
			ST_SCISSOR_X,
			ST_SCISSOR_Y,
			ST_SCISSOR_WIDTH,
			ST_SCISSOR_HEIGHT,
			ST_COUNT_SCISSOR
		};
#endif
		enum EMiscState
		{
			ST_MISC_FVF,
			ST_MISC_FVFSIZE,
			ST_MISC_INDICES,
			ST_MISC_PIXELSHADER,
			ST_MISC_VERTEXSHADER,
			ST_MISC_TEXTUREPALETTE,
			ST_MISC_SCISSORRECT,
			ST_MISC_NPATCHMODE,
			ST_MISC_SRCBLENDOVERRIDE,
			ST_MISC_DESTBLENDOVERRIDE,
			ST_MISC_BLTDEPTH,
			ST_MISC_TEXTURE,
			ST_MISC_PIXELSHADERCONST,
			ST_MISC_VERTEXSHADERCONST,
			ST_MISC_CLIPPLANE,
			ST_MISC_TEXTUREREMAP,
			ST_COUNT_MISC,
			ST_COUNT_MISC_SINGLE = 0x000b,
		};
		protected:
			typedef std::vector<State> StateVector;
			StateVector mRenderStates;
			std::vector<StateVector> mTextureStageStates;
			std::vector<StateVector> mSamplerStates;
			std::vector<StateVector> mLightStates;
			StateVector mMaterialStates;
			std::vector<StateVector> mStreamStates;
			std::vector<StateVector> mTransformStates;
			StateVector mViewportStates;
			std::vector<StateVector> mMiscStates;
			void InitRenderState(ulong inIndex, const std::string& inStateName, ulong inDefaultValue, bool inHasContextDefault, ulong inContextDefaultValue, const char* inValueEnumName  = "");
			void InitRenderStateFloat(ulong inIndex, const std::string& inStateName, float inDefaultValue);
			void InitTextureStageState(ulong inFirstStage, ulong inLastStage, ulong inIndex, const std::string& inStateName, ulong inDefaultValue, bool inHasContextDefault, ulong inContextDefaultValue, const char* inValueEnumName = "");
			void InitTextureStageStateFloat(ulong inFirstStage, ulong inLastStage, ulong inIndex, const std::string& inStateName, float inDefaultValue);
			void InitSamplerState(ulong inFirstStage, ulong inLastStage, ulong inIndex, const std::string& inStateName, ulong inDefaultValue, bool inHasContextDefault, ulong inContextDefaultValue, const char* inValueEnumName = "");
			void InitStates();
			void ResetStates();
			static StateValue MakeLightColorStateValue(const Color& inColor, float inAutoScale);
			static Color MakeStateValueLightColor(const State& inState);
			static ulong MakeFVFSize(ulong inFVF);
			D3DStateManager() {} //159
			~D3DStateManager();
			void Init() //163-165
			{
				InitStates();
			}
			void Reset() //167-169
			{
				ResetStates();
			}
		public:
			void SetRenderState(ulong inRS, ulong inValue);
			void SetTextureStageState(ulong inStage, ulong inTSS, ulong inValue);
			void SetSamplerState(ulong inSampler, ulong inSS, ulong inValue);
			void SetLightEnabled(ulong inLightIndex, bool inEnabled);
			void SetPointLight(int inLightIndex, const SexyVector3& inPos, const Graphics3D::LightColors& inColors, float inRange, const SexyVector3& inAttenuation);
			void SetDirectionalLight(int inLightIndex, const SexyVector3& inDir, const Graphics3D::LightColors& inColors);
			void SetMaterialAmbient(const Color& inColor, int inVertexColorComponent);
			void SetMaterialDiffuse(const Color& inColor, int inVertexColorComponent);
			void SetMaterialSpecular(const Color& inColor, int inVertexColorComponent, float inPower);
			void SetMaterialEmissive(const Color& inColor, int inVertexColorComponent);
			void SetStreamSource(ulong inStreamIndex, void* inVertexBuffer, ulong inOffset, ulong inStride, ulong inFreq);
			void SetTransform(ulong inTS, const SexyMatrix4* inMatrix);
			void SetViewport(ulong inX, ulong inY, ulong inWidth, ulong inHeight, float inMinZ, float inMaxZ);
			void SetFVF(ulong inFVF);
			void SetIndices(void* inIndexBuffer);
			void SetPixelShader(void* inShader);
			void SetVertexShader(void* inShader);
			void SetCurrentTexturePalette(ulong inPaletteIndex);
			void SetScissorRect(const RECT inRect);
			void SetNPatchMode(float inSegments);
			void SetTexture(ulong inSampler, void* inTexture);
			void SetTextureRemap(ulong inLogicalSampler, ulong inPhysicalSampler);
			void SetPixelShaderConstantF(ulong inStartRegister, const float* inConstantData, ulong inVector4fCount);
			void SetVertexShaderConstantF(ulong inStartRegister, const float* inConstantData, ulong inVector4fCount);
			void SetClipPlane(ulong inIndex, const float* inPlane);
			void SetBlendOverride(Graphics3D::EBlendMode inSrcBlend, Graphics3D::EBlendMode inDestBlend);
			void SetBltDepth(float inDepth);
			ulong GetRenderState(ulong inRS) const;
			ulong GetTextureStageState(ulong inStage, ulong inTSS) const;
			ulong GetSamplerState(ulong inSampler, ulong inSS) const;
			void GetTransform(ulong inTS, SexyMatrix4* inMatrix) const;
			ulong GetFVF() const;
			ulong GetFVFSize() const;
			void* GetTexture(ulong inSampler) const;
			ulong GetTextureRemap(ulong inLogicalSampler) const;
			void GetLightInfo(int inLightIndex, Graphics3D::LightColors& outColors, SexyVector3& outPos, SexyVector3& outDir, SexyVector3& outAttenuation, float& outRange);
			void GetMaterialInfo(Color& outAmbient, Color& outDiffuse, Color&, Color& outSpecular, Color& outEmissive, float& outSpecularPower);
			void GetBlendOverride(Graphics3D::EBlendMode& outSrcBlend, Graphics3D::EBlendMode& outDestBlend);
			float GetBltDepth();
	};
}
#endif