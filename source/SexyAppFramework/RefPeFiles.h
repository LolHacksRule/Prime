#ifndef __REFPEFILES_H__
#define __REFPEFILES_H__

#include "Reflection.h"

namespace Reflection
{
	struct SPeDebugTable
	{
		ulong charDebug;
		ulong timeStamp;
		ushort majorVersion;
		ushort minorVersion;
		ulong debugType;
		ulong sizeBytes;
		ulong rvaRawData;
		ulong ptrRawData;
	};
	struct SPeExportTable
	{
		ulong charDebug;
		ulong timeStamp;
		ushort majorVersion;
		ushort minorVersion;
		ulong rvaName;
		ulong baseOrdinal;
		ulong numExportItems;
		ulong numExportNames;
		ulong rvaItems;
		ulong rvaNames;
		ulong rvaOrdinals;
	};
	struct SPeHeaderDirEntry
	{
		ulong rva;
		ulong size;
	};
	struct SPeHeader
	{
		ulong magic;
		ushort machineType;
		ushort numSections;
		ulong timeStamp;
		ulong ptrSymTable;
		ulong numSymTableSyms;
		ushort sizeOfOptHeader;
		ushort charFlags;
		ushort optMagic;
		uchar majorLinkerVersion;
		uchar minorLinkerVersion;
		ulong sizeOfCode;
		ulong sizeOfInitData;
		ulong sizeOfUninitData;
		ulong rvaEntryPoint;
		ulong rvaBaseOfCode;
		ulong rvaBaseOfData;
		ulong preferredLoadAddress;
		ulong sectionAlignMemory;
		ulong sectionAlignFile;
		ushort majorOSVersion;
		ushort minorOSVersion;
		ushort majorImageVersion;
		ushort minorImageVersion;
		ushort majorSubsystemVersion;
		ushort minorSubsystemVersion;
		ulong versionWin32;
		ulong sizeOfImage;
		ulong sizeOfHeaders;
		ulong checkSum;
		ushort subsystemType;
		ushort charDll;
		ulong memReserveStack;
		ulong memCommitStack;
		ulong memReserveHeap;
		ulong memCommitHeap;
		ulong loaderFlags;
		ulong numDirEntries;
		SPeHeaderDirEntry dirEntries[1];
	};
	struct SPeImportByName
	{
		ushort hint;
		char name[1];
	};
	struct SPeImportDescriptor
	{
		ulong rvaFirstThunkOriginal;
		ulong timeStamp;
		ulong forwarderChain;
		ulong rvaExporterName;
		ulong rvaFirstThunk;
	};
	struct SPeImportThunkData
	{
		ulong rvaImportByName;
	};
	struct SPeSectionHeader
	{
		uchar name[8];
		ulong objAddressBinSize;
		ulong rvaData;
		ulong sizeRawData;
		ulong ptrRawData;
		ulong ptrRelocations;
		ulong ptrLineNumbers;
		ushort numRelocations;
		ushort numLineNumbers;
		ulong charSection;
	};
	struct SPeInfo
	{
		void* fileImageData;
		SPeHeader* peHdr;
		SPeSectionHeader* peSectionHeaders;
		bool isFromFile;
	};
	bool PE_GetInfo(void* inModuleBaseAddr, SPeInfo* outInfo, bool inIsFromFile);
	ulong PE_UnDecorateSymbolName(const char* inDecoratedName, char* outUndecName, ulong inUndecNameLen, ushort inFlags);
	void* PE_ResolveRVA(SPeInfo* inInfo, ulong inRVA);
	void* PE_ResolveAVA(SPeInfo* inInfo, ulong inAVA);
	void* PE_PatchImport(SPeInfo* inInfo, const char* inLibraryName, const char* inImportName, void* inFunction);
	const char* PE_GetPdbFileName(SPeInfo* inInfo);
	void PE_DumpInfo(SPeInfo* inInfo);
	static void PE_DumpImageHeader(SPeHeader* inHdr);
	static bool PE_DumpImageOptHeader(SPeHeader* inHdr);
	static void PE_DumpSectionHeader(SPeSectionHeader* inHdr);
	static void PE_DumpExports(SPeInfo* inInfo, bool inNamesOnly);
	static void PE_DumpImports(SPeInfo* inInfo, bool inNamesOnly);
	static void PE_DumpDebugInfo(SPeInfo* inInfo);
	static void PE_DumpSectionLabel(char* inSection);
}
#endif