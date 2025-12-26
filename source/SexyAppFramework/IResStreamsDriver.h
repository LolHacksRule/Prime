#ifndef __IRESSTREAMSDRIVER_H__
#define __IRESSTREAMSDRIVER_H__

#include "SexyAppBase.h"
#include "ResStreamsFormat.h"
#include "Common.h"

namespace Sexy
{
	class IGraphicsDriver;
	class SexyAppBase;
	class DeviceImage;
	class ResStreamsPool;
	class ResStreamTextureDescriptor;
	class ResStreamFileGPULocationInfo;
	class IResStreamsDriver //Empty on XNA.
	{
#ifdef _SEXYDECOMP_USE_LATEST_CODE
		class Task
		{
		public:
			GetAuxPtr();
			SetAuxData();
			SetAuxPtr();
			GetAuxData();
			Task();
			GetInBuffer();
			GetOutBuffer();
			DidFinishDecompression();
			GetRemainingBytesToDecompress();
			GetNumBytesRead();
			~Task();
		};
		class TaskResource
		{
		public:
			TaskResource();
			~TaskResource();
		};
#endif
	public:
		static IResStreamsDriver*	CreateResStreamsDriver();
		virtual						~IResStreamsDriver() = 0;
		virtual bool				InitWithApp(SexyAppBase* theApp) = 0;
		virtual bool				AttachGraphicsDriver(IGraphicsDriver*) = 0;
		virtual bool				AllocatePool(ResStreamsPool*) = 0;
		virtual bool				IsPoolInstanceBusy(ResStreamsPool*) = 0;
		virtual bool				CanReadGPUResourcesDirectly() = 0;
		virtual ulong				GetGPUDataSize(ResStreamTextureDescriptor*) = 0;
		virtual bool				BeginGPUDataCopy(ResStreamTextureDescriptor*) = 0;
		virtual bool				EndGPUDataCopy() = 0;
		virtual bool				CopyDataToTexture(void*, uchar, ulong) = 0;
		virtual DeviceImage*		GetImageFromResStream(const std::string, void*, ResStreamFileGPULocationInfo*) = 0;
	};
}
#endif //__IRESSTREAMSDRIVER_H__