#ifndef __SAFESTRFNTRCD_H
#define __SAFESTRFNTRCD_H

#include "Common.h"

namespace Sexy
{
	class SafeStrFntRcd //Is this part of Prime or PL only? No idea...
	{
		public:
			SafeStrFntRcd();
			~SafeStrFntRcd();
			void AddRecord(const std::wstring& str, const std::string& fntFile);
			void WriteRecordToFile();
		private:
			WStringStringMap mStrFntRcd;
			CRITICAL_SECTION mCSStrFntRcd;
	};
}
#endif