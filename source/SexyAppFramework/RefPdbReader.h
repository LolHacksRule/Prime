#ifndef __REFPDBREADER_H__
#define __REFPDBREADER_H__

#include "dia2.h"

namespace Reflection
{
	char* STR_WideToAnsi(const wchar_t* inSrcStr, char* inDestBuf, ulong inMaxLen);
	wchar_t* STR_AnsiToWide(const char* inSrcStr, wchar_t* inDestBuf, ulong inMaxLen);
	const char* STR_StripNamespace(const char* inName, char* outBuffer, int inRecurseCount);
	class IPdbTypeCollection
	{
	public:
		virtual ulong TypeCollGetTypeCount() = 0;
		virtual CPdbType* TypeCollGetTypeIndexed(ulong inIndex) = 0;
		//IPdbTypeCollection(); //?
	};
	class IPdbReader : public IPdbTypeCollection
	{
	public:
		virtual void ReaderDestroy() = 0;
		virtual ulong ReaderGetStructCount() = 0;
		virtual CPdbStruct* ReaderGetStructIndexed(ulong inIndex, bool inEnsureLoaded) = 0;
		virtual CPdbStruct* ReaderGetStructNamed(const char* inStructName, bool inEnsureLoaded) = 0;
		virtual ulong ReaderGetEnumCount() = 0;
		virtual CPdbEnum* ReaderGetEnumIndexed(ulong inIndex, bool inEnsureLoaded) = 0;
		virtual CPdbEnum* ReaderGetEnumNamed(const char* inEnumName, bool inEnsureLoaded) = 0;
		//IPdbReader(); //?
	};
	IPdbReader* PDB_CreateReader(const char* inPdbFileName, bool inForceFullLoad);
	class HPdbType
	{
	protected:
		ulong mIndex;
	public:
		HPdbType() { mIndex = 0; } //60
		HPdbType(const HPdbType& inType) { mIndex = inType.mIndex; } //61
		HPdbType(ulong inIndex) { mIndex = inIndex; } //62
		bool IsValid();

		ulong GetIndex() { return mIndex; } //65

		CPdbType* Resolve(IPdbTypeCollection* inTypeColl) const //67-69
		{
			return mIndex ? inTypeColl->TypeCollGetTypeIndexed(mIndex - 1) : NULL;
		}
		operator bool();
		bool operator!();
		bool Equals(const HPdbType& inType, IPdbTypeCollection* inTypeColl, bool inCheckConst);
	};
	class CPdbType
	{
	public:
		enum ETypeFlags
		{
			TYPEF_Const = 0x00001
		};
		enum EType
		{
			TYPE_Unknown,
			TYPE_Simple_MASK = 16,
			TYPE_Simple_Void,
			TYPE_Simple_Bool,
			TYPE_Simple_AChar,
			TYPE_Simple_WChar,
			TYPE_Simple_SInt,
			TYPE_Simple_UInt,
			TYPE_Simple_FloatDouble,
			TYPE_Simple_Ellipsis,
			TYPE_Simple_HResult,
			TYPE_Reference_MASK = 32,
			TYPE_Reference_Ampersand,
			TYPE_Reference_Pointer,
			TYPE_Reference_Array,
			TYPE_Function_MASK = 64,
			TYPE_Function_UnkCall,
			TYPE_Function_ThisCall,
			TYPE_Function_Cdecl,
			TYPE_Function_StdCall,
			TYPE_Function_FastCall,
			TYPE_Function_SysCall,
			TYPE_Named_MASK = 129,
			TYPE_Named_Generic,
		};
		HPdbType mHandle;
		ulong mType;
		ulong mTypeFlags;
		ulong mSize;
		ulong mThisAdjust;
		HPdbType mRefTypeInnerType;
		HPdbType mFuncTypeThisType;
		HPdbType mFuncTypeReturnType;
		std::vector<HPdbType> mFuncTypeArgTypes;
		std::string mNamedTypeName;
		IPdbTypeCollection* mTypeColl;
		CPdbType() { mType = 0, mTypeFlags, mSize, mThisAdjust, mTypeColl = 0; } //153
		//~CPdbType(); //?
		std::string ToString();
		bool Equals(CPdbType* inType, bool inCheckConst);
	};
	class CPdbStructForm
	{
	public:
		enum EStructFlags
		{
			STRUCTF_Union = 0x0001,
			STRUCTF_Dispute_MASK = 0xfff0,
			STRUCTF_Dispute_Flags = 0x0010,
			STRUCTF_Dispute_SizeInstance = 0x0020,
			STRUCTF_Dispute_IntroVtblSize = 0x0040,
			STRUCTF_Dispute_Bases = 0x0080,
			STRUCTF_Dispute_Fields = 0x0100,
			STRUCTF_Dispute_Methods = 0x0200
		};
		enum EAccess
		{
			ACCESS_Unknown,
			ACCESS_Private,
			ACCESS_Protected,
			ACCESS_Public
		};
		class CAttribute
		{
		public:
			std::string mKey;
			std::string mValueMethod;
			//CAttribute(); //?
		};
		class CBase
		{
		public:
			enum EBaseFlags
			{
				BASEF_Disputed = 0x0010,
				BASEF_Reserved = 0x8000
			};
			std::string mName;
			ulong mBaseFlags;
			ulong mOffset;
			CBase() { mBaseFlags, mOffset = 0; } //213
			bool Equals(CBase* inBase);
			//~CBase(); //?
		};
		class CField
		{
		public:
			enum EFieldFlags
			{
				FIELDF_Static = 0x0001,
				FIELDF_Disputed = 0x0010,
				FIELDF_Reserved = 0x8000,
			};
		public:
			std::string mName;
			HPdbType mType;
			ulong mFieldFlags;
			ulong mOffset;
			EAccess mAccess;
			std::vector<CAttribute> mAttributes;
			CField() { mFieldFlags, mOffset = 0; mAccess = ACCESS_Unknown; } //240
			bool Equals(CField* inField, IPdbTypeCollection* inTypeColl);
			//~CField(); //?
		};
		class CMethod
		{
		public:
			enum EMethodFlags
			{
				METHODF_Virtual = 0x0001,
				METHODF_IntroVirtual,
				METHODF_PureVirtual = 0x00004,
				METHODF_Disputed = 0x0010,
				METHODF_Reserved = 0x8000,
			};
		public:
			std::string mName;
			HPdbType mType;
			ulong mMethodFlags;
			ulong mRVA;
			ulong mVtblOffset;
			EAccess mAccess;
			std::vector<CAttribute> mAttributes;
			CMethod() { mMethodFlags, mRVA, mVtblOffset = 0; mAccess = ACCESS_Unknown; } //271
			bool Equals(CMethod* inMethod, IPdbTypeCollection* inTypeColl);
			//~CMethod(); //?
		};
		std::string mStructName;
		ulong mStructFlags;
		ulong mSizeInstance;
		ulong mIntroVtblSize;
		std::vector<CBase> mBases;
		std::vector<CField> mFields;
		std::vector<CMethod> mMethods;
		std::vector<CAttribute> mAttributes;
		CPdbStructForm() { mStructFlags, mSizeInstance, mIntroVtblSize = 0; } //290
		bool Equals(CPdbStructForm* inStruct, IPdbTypeCollection* inTypeColl);
		//~CPdbStructForm(); //?
	};
	class CPdbStruct
	{
	public:
		std::string mStructName;
		std::vector<ulong> mSymIds;
		std::vector<CPdbStructForm> mForms;
		CPdbStruct() { } //305
		//~CPdbStruct(); //?
	};
	class CPdbEnumForm
	{
	public:
		enum EEnumFlags
		{
			ENUMF_Dispute_MASK = 0xfff0,
			ENUMF_Dispute_Flags = 0x0010,
			ENUMF_Dispute_Members = 0x0020,
		};
		class CMember
		{
		public:
			enum
			{
				MEMBERF_Disputed = 16,
				MEMBERF_Reserved = 0xFFFF8000,
			};
			std::string mName;
			int mValue;
			ulong mMemberFlags;
			bool Equals(CMember* inMember);
			CMember() { mValue, mMemberFlags = 0; } //341
			//~CMember(); //?
		};
		std::string mEnumName;
		ulong mEnumFlags;
		std::vector<CMember> mMembers;
		CPdbEnumForm() { mEnumFlags = 0; } //353
		//~CPdbEnumForm(); //?
		bool Equals(CPdbEnumForm* inEnum);
	};
	class CPdbEnum
	{
	public:
		std::string mEnumName;
		std::vector<ulong> mSymIds;
		std::vector<CPdbEnumForm> mForms;
		CPdbEnum() {} //368
		//~CPdbEnum(); //?
	};
	class CPdbDiaChildSymbolColl
	{
	public:
		std::vector<IDiaSymbol*> mChildSymbols;
		//CPdbDiaChildSymbolColl(); //?
		CPdbDiaChildSymbolColl(IDiaSymbol* inParentSymbol, enum SymTagEnum inTag, const char* inMaskStr, ulong inCompareFlags);
		~CPdbDiaChildSymbolColl();
	};
	class CPdbReader : public IPdbReader
	{
	public:
		struct SStringLess { bool operator()(const std::string& x, const std::string& y) const; };
		typedef std::map<std::string, int, SStringLess> DStructNameToIndexMap;
		typedef std::map<std::string, int, SStringLess> DEnumNameToIndexMap; //It's the same as the top. This is stupid, whatever
	protected:
		IDiaDataSource* mDataSource;
		IDiaSession* mSession;
		std::vector<CPdbType> mTypes;
		std::vector<CPdbStruct> mStructs;
		DStructNameToIndexMap mStructMap;
		std::vector<CPdbEnum> mEnums;
		DEnumNameToIndexMap mEnumMap;
	public:
		CPdbReader();
		~CPdbReader();
	protected:
		CPdbStruct* GetStruct(const char* inStructName, bool inAllowAdd);
		CPdbEnum* GetEnum(const char* inEnumName, bool inAllowAdd);
		HPdbType DiaSymGetInnerType(IDiaSymbol* inSym);
		bool HasStructTag(IDiaSymbol*, const char*);
		bool AttemptGetTimeStamp(IDiaSymbol*, ulong*);
		HPdbType BuildType(IDiaSymbol* inSym);
		bool BuildField(CPdbStructForm* inStruct, IDiaSymbol* inStructSym, IDiaSymbol* inFieldSym, bool inNeedsTag);
		bool BuildMethod(CPdbStructForm* inStruct, IDiaSymbol* inStructSym, IDiaSymbol* inMethodSym, bool inNeedsTag);
		void BuildStructFormInternal(IDiaSymbol* inStructSym, CPdbStructForm* inStruct);
		bool PrePassStructForm(IDiaSymbol* inStructSym);
		bool BuildStruct(CPdbStruct* inStruct);
		bool PrePassEnumForm(IDiaSymbol* inEnumSym);
		bool BuildEnumMember(CPdbEnumForm* inEnum, IDiaSymbol* inMemberSym);
		void BuildEnumFormInternal(IDiaSymbol* inEnumSym, CPdbEnumForm* inEnum);
		bool BuildEnum(CPdbEnum* inEnum);
		bool BuildFromExe(IDiaSymbol* inExeSym, bool inForceFullLoad);
		static DWORD DiaSymGetTag(IDiaSymbol* inSym);
		static const char* DiaSymGetName(IDiaSymbol* inSym);
		static ulong DiaSymGetSize(IDiaSymbol* inSym);
		static CPdbStructForm::EAccess DiaSymGetAccess(IDiaSymbol* inSym);
	public:
		void Cleanup();
		bool ReadPdb(const char* inPdbFileName, bool inForceFullLoad);
		virtual void ReaderDestroy();
		virtual ulong ReaderGetStructCount();
		virtual CPdbStruct* ReaderGetStructIndexed(ulong inIndex, bool inEnsureLoaded);
		virtual CPdbStruct* ReaderGetStructNamed(const char* inStructName, bool inEnsureLoaded);
		virtual ulong ReaderGetEnumCount();
		virtual CPdbEnum* ReaderGetEnumIndexed(ulong inIndex, bool inEnsureLoaded);
		virtual CPdbEnum* ReaderGetEnumNamed(const char* inEnumName, bool inEnsureLoaded);
		virtual ulong TypeCollGetTypeCount();
		virtual CPdbType* TypeCollGetTypeIndexed(ulong inIndex);
	};
}
#endif