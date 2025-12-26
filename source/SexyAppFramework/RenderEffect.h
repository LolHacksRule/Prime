#ifndef __RENDEREFFECT_H__
#define __RENDEREFFECT_H__

#include "RenderDevice.h"

namespace Sexy
{
	class RenderEffectDefinition //Unlimplemented in XNA.
	{
	public:
		ByteVector mData;
		std::string mSrcFileName;
		std::string mDataFormat;
		bool LoadFromMem(ulong inDataLen, const void* inData, const char* inSrcFileName, const char* inDataFormat);
		bool LoadFromFile(const char* inFileName, const char* inSrcFileName);
	};
	class RenderEffect
	{
	protected:
		virtual ~RenderEffect() {}; //63
	public:
		typedef HANDLE HRunHandle;
		virtual RenderDevice3D* GetDevice() = 0;
		virtual RenderEffectDefinition* GetDefinition() = 0;
		virtual void SetParameter(const std::string& inParamName, const float* inFloatData, ulong inFloatCount) = 0;
		void SetFloat(const std::string& inName, float inValue);
		void SetVector4(const std::string& inParamName, const float* inValue);
		void SetVector3(const std::string& inParamName, const float* inValue);
		virtual void SetMatrix(const std::string& inParamName, const float* inValue) { SetParameter(inParamName, inValue, 16); } //88
		virtual void GetParameterBySemantic(ulong inSemantic, float* outFloatData, ulong inMaxFloatCount) = 0;
		virtual void SetCurrentTechnique(const std::string& inName, bool inCheckValid = true) = 0;
		virtual std::string GetCurrentTechniqueName() = 0;
		virtual int Begin(const HRunHandle& outRunHandle, const HRenderContext& inRenderContext) = 0;
		virtual void BeginPass(const HRunHandle& inRunHandle, int inPass) = 0;
		virtual void EndPass(const HRunHandle& inRunHandle, int inPass) = 0;
		virtual void End(const HRunHandle& inRunHandle) = 0;
		virtual bool PassUsesVertexShader(int inPass) = 0;
		virtual bool PassUsesPixelShader(int inPass) = 0;
		//RenderEffect();
	};
	class RenderEffectAutoState //Not in exe, why
	{
	protected:
		RenderEffect* mEffect;
		RenderEffect::HRunHandle mRunHandle;
		int mPassCount;
		int mCurrentPass;
	public:
		RenderEffectAutoState(Graphics* inGraphics, RenderEffect* inEffect, int inDefaultPassCount = 1); //Variables are recovered from XNA, not in PL_D.dll.
		~RenderEffectAutoState();
		void Reset(Graphics* inGraphics, RenderEffect* inEffect, int inDefaultPassCount = 1);
		void NextPass();
		bool IsDone() const;
		bool PassUsesVertexShader();
		bool PassUsesPixelShader();
		operator bool() const; //?
		bool operator!() const;
		RenderEffectAutoState& operator++(int);
		RenderEffectAutoState& operator++();
		
	};
}
#endif