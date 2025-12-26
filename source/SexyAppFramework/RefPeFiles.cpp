#include "RefPeFiles.h"
#include <memoryapi.h>
#include <ctime>

static char* pe_DirEntryNames[16] = { "Exports", "Imports", "Resources", "Exception", "Security", "Base Relocation", "Debug", "Copyright", "GlobalPtr", "Thread" "Local Storage", "Load Configuration", "Bound Imports", "Import Address Table", "", "", "" };
using namespace Reflection;

void Reflection::PE_DumpImageHeader(SPeHeader* inHdr) //66-115
{
	printf("Machine Type: \n");
	switch (inHdr->machineType)
	{
	case IMAGE_FILE_MACHINE_I386: printf("Intel 386\n"); break;
	case IMAGE_SYM_CLASS_FAR_EXTERNAL | IMAGE_SYM_CLASS_ARGUMENT | IMAGE_SCN_LNK_OTHER: printf("Intel 486\n"); break;
	case IMAGE_SYM_CLASS_FAR_EXTERNAL | IMAGE_SYM_CLASS_STRUCT_TAG | IMAGE_SCN_LNK_OTHER: printf("Intel Pentium\n"); break;
	case IMAGE_SCN_LNK_OTHER | IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_CNT_CODE: printf("MIPS R3000 BigEndian\n"); break;
	case IMAGE_FILE_MACHINE_R3000: printf("MIPS R3000 LittleEndian\n"); break;
	case IMAGE_FILE_MACHINE_R4000: printf("MIPS R4000\n"); break;
	case IMAGE_FILE_MACHINE_R10000: printf("MIPS R10000\n"); break;
	case IMAGE_FILE_MACHINE_ALPHA: printf("DEC Alpha AXP\n"); break;
	case IMAGE_FILE_MACHINE_POWERPC: printf("IBM PowerPC\n"); break;
	default: printf("Unknown 0x%08X\n", inHdr->machineType); break;
	}
	printf("Number of Sections: %d\n", inHdr->numSections);
	printf("TimeStamp: %s\n", ctime((const time_t*)inHdr->timeStamp));
	printf("Debug Symbols Address: 0x%08X\n", inHdr->ptrSymTable);
	printf("Characteristics:\n");
	if ((inHdr->charFlags & IMAGE_FILE_RELOCS_STRIPPED) != 0) printf("\tRelocations stripped\n");
	if ((inHdr->charFlags & IMAGE_FILE_EXECUTABLE_IMAGE) != 0) printf("\tExecutable\n");
	if ((inHdr->charFlags & IMAGE_FILE_LINE_NUMS_STRIPPED) != 0) printf("\tLine Numbers stripped\n");
	if ((inHdr->charFlags & IMAGE_FILE_LOCAL_SYMS_STRIPPED) != 0) printf("\tLocal Symbols Stripped\n");
	if ((inHdr->charFlags & IMAGE_FILE_AGGRESIVE_WS_TRIM) != 0) printf("\tAggressive Working Set Trimming\n");
	if ((inHdr->charFlags & IMAGE_FILE_BYTES_REVERSED_LO) != 0) printf("\tBytes Reversed (Low)\n");
	if ((inHdr->charFlags & IMAGE_FILE_32BIT_MACHINE) != 0) printf("\t32-bit Machine\n");
	if ((inHdr->charFlags & IMAGE_FILE_DEBUG_STRIPPED) != 0) printf("\tDebug Info Stripped\n");
	if ((inHdr->charFlags & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP) != 0) printf("\tDon't Run From Removable Media\n");
	if ((inHdr->charFlags & IMAGE_FILE_NET_RUN_FROM_SWAP) != 0) printf("\tDon't Run From Network Media\n");
	if ((inHdr->charFlags & IMAGE_FILE_SYSTEM) != 0) printf("\tSystem/Driver File\n");
	if ((inHdr->charFlags & IMAGE_FILE_DLL) != 0) printf("\tDLL File\n");
	if ((inHdr->charFlags & IMAGE_FILE_UP_SYSTEM_ONLY) != 0) printf("\tSingle-processor System Only\n");
	if ((inHdr->charFlags & IMAGE_FILE_BYTES_REVERSED_HI) != 0) printf("\tBytes Reversed (High)\n");
	printf("\n");
}

bool Reflection::PE_DumpImageOptHeader(SPeHeader* inHdr) //118-176
{
	if (inHdr->optMagic == IMAGE_NT_OPTIONAL_HDR_MAGIC)
	{
		printf("Linker Version: %d.%d\n", inHdr->majorLinkerVersion, inHdr->minorLinkerVersion);
		printf("Code Size: %d\n", inHdr->sizeOfCode);
		printf("Initialized Data Size: %d\n", inHdr->sizeOfInitData);
		printf("Uninitialized Data Size: %d\n", inHdr->sizeOfUninitData);
		printf("Entry Point: 0x%08X\n", inHdr->rvaEntryPoint);
		printf("Code Offset: 0x%08X\n", inHdr->rvaBaseOfCode);
		printf("Initialized Data Offset: 0x%08X\n", inHdr->rvaBaseOfData);
		printf("Preferred Load Address: 0x%08X\n", inHdr->preferredLoadAddress);
		printf("Memory Section Alignment: 0x%08X\n", inHdr->sectionAlignMemory);
		printf("File Section Alignment: 0x%08X\n", inHdr->sectionAlignFile);
		printf("OS Version: %d.%d\n", inHdr->majorOSVersion, inHdr->minorOSVersion);
		printf("Image Version: %d.%d\n", inHdr->majorImageVersion, inHdr->minorImageVersion);
		printf("Subsystem Version: %d.%d\n", inHdr->majorSubsystemVersion, inHdr->minorSubsystemVersion);
		printf("Win32 Version: %d\n", inHdr->versionWin32);
		printf("Image Memory Size: %d\n", inHdr->sizeOfImage);
		printf("Total Header Size: %d\n", inHdr->sizeOfHeaders);
		printf("Checksum: %d\n", inHdr->checkSum);
		printf("Subsystem Type: \n");
		switch (inHdr->subsystemType)
		{
		case IMAGE_SUBSYSTEM_NATIVE: printf("\tNative Driver\n"); break;
		case IMAGE_DIRECTORY_ENTRY_RESOURCE: printf("\tWin32 GUI\n"); break;
		case IMAGE_SYM_TYPE_SHORT: printf("\tWin32 Console\n"); break;
		case IMAGE_SYM_TYPE_LONG: printf("\tOS/2 Console\n"); break;
		case IMAGE_SYM_TYPE_DOUBLE: printf("\tPOSIX Console\n"); break;
		default: printf("\tUnknown 0x%08X\n", inHdr->subsystemType); break;
		}
		printf("DLL Characteristics:\n");
		if ((inHdr->charDll & 1) != 0) printf("\tDLL Load\n");
		if ((inHdr->charDll & 8) != 0) printf("\tDLL Unload\n");
		if ((inHdr->charDll & 4) != 0) printf("\tThread Attach\n");
		if ((inHdr->charDll & 2) != 0) printf("\tThread Detach\n");
		printf("Stack Memory Reserved: 0x%08X\n", inHdr->memReserveStack);
		printf("Stack Memory Committed: 0x%08X\n", inHdr->memCommitStack);
		printf("Heap Memory Reserved: 0x%08X\n", inHdr->memReserveHeap);
		printf("Heap Memory Committed: 0x%08X\n", inHdr->memCommitHeap);
		printf("Loader Flags: 0x%08X\n", inHdr->loaderFlags);
		printf("Number of Directory Entries: %d\n", inHdr->numDirEntries);
		printf("Directory Entries:\n");
		for (ulong i = 0; i < inHdr->numDirEntries; i++)
		{
			if (inHdr->dirEntries[i].size)
				printf("\t%s: Address 0x%08X, Size %d\n", pe_DirEntryNames[i], inHdr->dirEntries[i].rva, inHdr->dirEntries[i].size);
		}
		printf("\n");
		return true;
	}
	else
	{
		printf("File has invalid optional header signature\n");
		return false;
	}
}

void Reflection::PE_DumpSectionHeader(SPeSectionHeader* inHdr) //179-231
{
	char tempName[9] = "";
	memcpy(tempName, inHdr, 8);
	printf("Name: %s\n", tempName);
	printf("Obj Address / Bin Size: %d\n", inHdr->objAddressBinSize);
	printf("Memory Address: 0x%08X\n", inHdr->rvaData);
	printf("Data Size: %d\n", inHdr->sizeRawData);
	printf("Data Offset: 0x%08X\n", inHdr->ptrRawData);
	printf("Relocations: Count %d, Offset 0x%08X\n", inHdr->numRelocations, inHdr->ptrRelocations);
	printf("Line Numbers: Count %d, Offset 0x%08X\n", inHdr->numLineNumbers, inHdr->ptrLineNumbers);
	printf("Characteristics:\n");
	if ((inHdr->charSection & IMAGE_SCN_CNT_CODE) != 0) printf("\tContains Code\n");
	if ((inHdr->charSection & IMAGE_SCN_CNT_INITIALIZED_DATA) != 0) printf("\tContains Initialized Data\n");
	if ((inHdr->charSection & IMAGE_SCN_CNT_UNINITIALIZED_DATA) != 0) printf("\tContains Uninitialized Data\n");
	if ((inHdr->charSection & IMAGE_SCN_LNK_INFO) != 0) printf("\tLinker Info\n");
	if ((inHdr->charSection & IMAGE_SCN_LNK_REMOVE) != 0) printf("\tLinker Removable\n");
	if ((inHdr->charSection & IMAGE_SCN_LNK_COMDAT) != 0) printf("\tLinker Common Block Data\n");
	if ((inHdr->charSection & IMAGE_SCN_MEM_FARDATA) != 0) printf("\tMemory Far Data\n");
	if ((inHdr->charSection & IMAGE_SCN_MEM_PURGEABLE) != 0) printf("\tMemory Purgeable\n");
	if ((inHdr->charSection & IMAGE_SCN_MEM_LOCKED) != 0) printf("\tMemory Locked\n");
	if ((inHdr->charSection & IMAGE_SCN_MEM_PRELOAD) != 0) printf("\tMemory Preload\n");
	ulong align = (inHdr->charSection >> 20) & 0xF;
	if (align) printf("\tAlignment %d\n", align);
	if ((inHdr->charSection & IMAGE_SCN_LNK_NRELOC_OVFL) != 0) printf("\tLinker Extended\n");
	if ((inHdr->charSection & IMAGE_SCN_MEM_DISCARDABLE) != 0) printf("\tMemory Discardable\n");
	if ((inHdr->charSection & IMAGE_SCN_MEM_NOT_CACHED) != 0) printf("\tMemory No Caching\n");
	if ((inHdr->charSection & IMAGE_SCN_MEM_NOT_PAGED) != 0) printf("\tMemory No Paging\n");
	if ((inHdr->charSection & IMAGE_SCN_MEM_SHARED) != 0) printf("\tMemory Share All Instances\n");
	if ((inHdr->charSection & IMAGE_SCN_MEM_EXECUTE) != 0) printf("\tExecute Access\n");
	if ((inHdr->charSection & IMAGE_SCN_MEM_READ) != 0) printf("\tRead Access\n");
	if ((inHdr->charSection & IMAGE_SCN_MEM_WRITE) != 0) printf("\tWrite Access\n");
	printf("\n");
}

void Reflection::PE_DumpExports(SPeInfo* inInfo, bool inNamesOnly) //234-272
{
	SPeExportTable* exports = NULL;
	if (inInfo->peHdr->dirEntries[0].size)
		exports = (SPeExportTable*)PE_ResolveRVA(inInfo, inInfo->peHdr->dirEntries[0].rva);
	if (exports)
	{
		printf("TimeStamp: %s\n", ctime((const time_t*)exports->timeStamp));
		printf("Binary Name: %s\n", PE_ResolveRVA(inInfo, exports->rvaName));
		printf("Base Ordinal: %d\n", exports->baseOrdinal);
		printf("Number of Exports: %d\n", exports->numExportItems);
		printf("Number of Names: %d\n", exports->numExportNames);
		ulong* names = (ulong*)PE_ResolveRVA(inInfo, exports->rvaNames);
		ulong* items = (ulong*)PE_ResolveRVA(inInfo, exports->rvaItems);
		ushort* ordinals = (ushort*)PE_ResolveRVA(inInfo, exports->rvaOrdinals);
		for (ulong i = 0; i < exports->numExportNames; ++i)
		{
			char* nameStr = (char*)PE_ResolveRVA(inInfo, names[i]);
			if (*nameStr == '?')
			{
				char undecNameStr[1024];
				PE_UnDecorateSymbolName(nameStr, undecNameStr, 0x3FFu, inNamesOnly ? 0x1000 : 0);
				printf("\t@%d (RVA 0x%08X): %s\n", exports->baseOrdinal + ordinals[i], items[i], undecNameStr);
			}
			else
				printf("\t@%d (RVA 0x%08X): %s\n", exports->baseOrdinal + ordinals[i], items[i], nameStr);
		}
	}
	printf("\n");
}

void Reflection::PE_DumpImports(SPeInfo* inInfo, bool inNamesOnly) //275-337
{
	SPeImportDescriptor* imports = NULL;
	char undecNameStr[1024];
	if (inInfo->peHdr->machineType)
		imports = (SPeImportDescriptor*)PE_ResolveRVA(inInfo, inInfo->peHdr[1].magic);
	if (imports)
	{
		ulong libCount = 0;
		while (imports->rvaFirstThunkOriginal || imports->timeStamp || imports->forwarderChain || imports->rvaExporterName || imports->rvaFirstThunk)
		{
			printf("Library: %s  \n", PE_ResolveRVA(inInfo, imports->rvaExporterName));
			if (imports->timeStamp)
				printf("  TimeStamp: %s\n", ctime((const time_t*)imports->timeStamp));
			else
				printf("  TimeStamp: Unbound\n");
			ulong thunkCount = 0;
			SPeImportThunkData* thunks = (SPeImportThunkData*)PE_ResolveRVA(inInfo, imports->rvaFirstThunkOriginal);
			for (thunks; thunks && thunks->rvaImportByName; thunks++)
			{
				if ((thunks->rvaImportByName & IMAGE_SCN_MEM_WRITE) == 0)
				{
					SPeImportByName* impByName = (SPeImportByName*)PE_ResolveRVA(inInfo, thunks->rvaImportByName);
					if (impByName->name[0] == '?')
					{
						PE_UnDecorateSymbolName(impByName->name, undecNameStr, 1023, inNamesOnly ? IMAGE_SCN_LNK_COMDAT : 0);
						printf("  %d: %s\n", thunkCount, undecNameStr[1024]);
					}
					else
						printf("  %d: %s\n", thunkCount, impByName->name);
				}
				else
					printf("  %d: @d\n", thunkCount, thunks->rvaImportByName);
				++thunkCount;
			}
			++libCount;
			++imports;
		}
	}
	printf("\n");
}

void Reflection::PE_DumpDebugInfo(SPeInfo* inInfo) //340-345
{
	const char* pdbFileName = PE_GetPdbFileName(inInfo);
	if (pdbFileName)
		printf("  %s\n", pdbFileName);
	printf("\n");
}

void Reflection::PE_DumpSectionLabel(char* inSection) //348-353
{
	printf("-------------------------------\n");
	printf("t%s\n", inSection);
	printf("-------------------------------\n");
	printf("\n");
}

bool Reflection::PE_GetInfo(void* inModuleBaseAddr, SPeInfo* outInfo, bool inIsFromFile) //359-384
{
	if (!outInfo || (DWORD)inModuleBaseAddr != IMAGE_DOS_SIGNATURE)
		return false;
	SPeHeader *pehdr = (SPeHeader*)((char*)inModuleBaseAddr + (DWORD)inModuleBaseAddr + 15); //?
	if (pehdr->magic != IMAGE_NT_SIGNATURE)
		return false;
	outInfo->fileImageData = inModuleBaseAddr;
	outInfo->isFromFile = inIsFromFile;
	outInfo->peHdr = pehdr;
	outInfo->peSectionHeaders = (SPeSectionHeader*)&pehdr->dirEntries[pehdr->numDirEntries];
	return true;
}

ulong Reflection::PE_UnDecorateSymbolName(const char* inDecoratedName, char* outUndecName, ulong inUndecNameLen, ushort inFlags) //389-397
{
	if (!outUndecName || !inUndecNameLen)
		return 0;

	*outUndecName = 0;
	//unDName(outUndecName, inDecoratedName, inUndecNameLen, malloc, free, inFlags); //?

	return strlen(outUndecName);
}

void* Reflection::PE_ResolveRVA(SPeInfo* inInfo, ulong inRVA) //400-322
{
	if (!inRVA)
		return false;
	if (!inInfo->isFromFile)
		return &inInfo->fileImageData + inRVA;

	SPeSectionHeader *section = NULL;
	for (int i = 0; i < inInfo->peHdr->numSections; i++)
	{
		SPeSectionHeader* curSection = &inInfo->peSectionHeaders[i];
		if (inRVA >= curSection->rvaData && inRVA < curSection->sizeRawData + curSection->rvaData)
		{
			section = &inInfo->peSectionHeaders[i];
			break;
		}
	}
	if (section)
		return &inInfo->fileImageData + section->ptrRawData + inRVA - section->rvaData;
	else
		return 0;
}

void* Reflection::PE_ResolveAVA(SPeInfo* inInfo, ulong inAVA) //425-428
{
	return PE_ResolveRVA(inInfo, inAVA - inInfo->peHdr->preferredLoadAddress);
}

void* Reflection::PE_PatchImport(SPeInfo* inInfo, const char* inLibraryName, const char* inImportName, void* inFunction) //431-479
{
	SPeImportDescriptor *imports = NULL;
	if (&inInfo->peHdr[1].machineType)
		imports = (SPeImportDescriptor*)PE_ResolveRVA(inInfo, inInfo->peHdr[1].magic);
	if (imports != NULL)
	{
		while (imports->rvaFirstThunkOriginal || imports->timeStamp || imports->forwarderChain || imports->rvaExporterName || imports->rvaFirstThunk)
		{
			if (stricmp((const char*)PE_ResolveRVA(inInfo, imports->rvaExporterName), inLibraryName))
			{
				SPeImportThunkData* nameThunks = (SPeImportThunkData*)PE_ResolveRVA(inInfo, imports->rvaFirstThunkOriginal);
				void** iatThunks = (void**)PE_ResolveRVA(inInfo, imports->rvaFirstThunk);
				while (nameThunks && nameThunks->rvaImportByName)
				{
					if ((nameThunks->rvaImportByName & 2147483648) == 0)
					{
						SPeImportByName* impByName = (SPeImportByName*)PE_ResolveRVA(inInfo, nameThunks->rvaImportByName);
						bool isMatch = false;
						if (impByName->name[0] == 63)
						{
							char undecNameStr[1024];
							PE_UnDecorateSymbolName(impByName->name, undecNameStr, 1023, 4096);
							isMatch = stricmp(inImportName, undecNameStr) == 0;
						}
						else
							isMatch = stricmp(inImportName, impByName->name) == 0;
						ulong oldProtection;
						if (isMatch && VirtualProtect(iatThunks, 4u, 4u, &oldProtection))
						{
							void* oldFunc = *iatThunks;
							memcpy(iatThunks, &inFunction, sizeof(void*));
							VirtualProtect(iatThunks, 4u, oldProtection, &oldProtection);
							return oldFunc;
						}
					}
					++nameThunks;
					++iatThunks;
				}
			}
			++imports;
		}
	}
	return NULL;
}

const char* Reflection::PE_GetPdbFileName(SPeInfo *inInfo) //482-541
{
	struct CV_HEADER
	{
		ulong CvSignature;
		int Offset;
	};
	struct CV_INFO_PDB20
	{
		CV_HEADER Header;
		ulong Signature;
		ulong Age;
		char PdbFileName[1];
	};
	struct CV_INFO_PDB70
	{
		ulong CvSignature;
		uchar SignatureGuid[16];
		ulong Age;
		char PdbFileName[1];
	};
	SPeDebugTable* debugTable = NULL;
	if (inInfo->peHdr[1].rvaBaseOfCode)
		debugTable = (SPeDebugTable*) PE_ResolveRVA(inInfo, inInfo->peHdr[1].rvaEntryPoint);
	if (debugTable)
	{
		ulong numEntries = inInfo->peHdr[1].rvaBaseOfCode / 28;
		for (ulong i = 0; i < numEntries; ++i)
		{
			if (debugTable[i].debugType == IMAGE_DEBUG_TYPE_CODEVIEW && debugTable[i].sizeBytes >= 4) //?
			{
				uchar* debugData = (uchar*) PE_ResolveRVA(inInfo, debugTable[i].rvaRawData);
				if (*debugData == 808534606)
					return (char*)(debugData + 16);
				if (*debugData == 1396986706)
					return (char*)(debugData + 24);
			}
		}
	}
	return 0;
}

void Reflection::PE_DumpInfo(SPeInfo* inInfo) //544-572
{
	//
	PE_DumpSectionLabel("HEADER");

	PE_DumpImageHeader(inInfo->peHdr);
	PE_DumpSectionLabel("OPTHEADER");


	if (!PE_DumpImageOptHeader(inInfo->peHdr))
		return;

	PE_DumpSectionLabel("SECTION HEADERS");
	for (ulong i = 0; i < inInfo->peHdr->numSections; ++i)
		PE_DumpSectionHeader(&inInfo->peSectionHeaders[i]);

	PE_DumpSectionLabel("EXPORTS");
	PE_DumpExports(inInfo, true);


	PE_DumpSectionLabel("IMPORTS");
	PE_DumpImports(inInfo, true);


	PE_DumpSectionLabel("DEBUG");
	PE_DumpDebugInfo(inInfo);

	PE_DumpSectionLabel("DONE");
}