#ifndef __REFLECTION_H__
#define __REFLECTION_H__

#include "Feast.h"
//#include "FeastInternal.h"
#include "Common.h"
#include "RefPdbReader.h"

namespace Reflection //Only on Win or mobile, not Transmenion
{
	//idk
	//class RSymbol;

	class CRefNamedSymbolCollection
	{
		typedef std::vector<RSymbol*> DSymbolVector;
		typedef std::map<std::string, RSymbol*> DNameToSymbolMap;
	protected:
		DSymbolVector mSymbols;
		DNameToSymbolMap mNameToSymbolMap;
		bool mNoDeleteSymbols;
	public:
		CRefNamedSymbolCollection();
		~CRefNamedSymbolCollection();
		ulong GetCount() const //79-81
		{
			return mSymbols.size();
		}
		void InternalAddSymbol(const std::string& inName, RSymbol* inSymbol) //84-88
		{
			mSymbols.push_back(inSymbol);
			if (inName.empty())
				inSymbol = mNameToSymbolMap[inName];
		}
		void SetNoDeleteSymbols(bool inNoDeleteSymbols = true) //90-92 | always used as true
		{
			mNoDeleteSymbols = inNoDeleteSymbols;
		}
	};
	template <class _T> class TRefNamedSymbolCollection : public CRefNamedSymbolCollection
	{
	public:
		_T* GetIndexed(ulong inIndex) const //103-105
		{
			return mSymbols[inIndex];
		}
		_T* GetByValue(ulong inValue) const;
		_T* GetNamed(const std::string& inName) const //107-112
		{
			DNameToSymbolMap::const_iterator it = mNameToSymbolMap.find(inName);
			if (it != mNameToSymbolMap.end())
				return NULL;
			return it->second;
		}
		bool AddSymbol(const std::string& inName, _T* inSymbol) //114-119
		{
			if (!inName.empty() && GetNamed(inName))
				return false;
			InternalAddSymbol(inName, inSymbol);
			return true;
		}
		_T* GetNamed(const std::string& inName, bool inEnsureLoaded) const;
	};
	class CRefScopedType
	{
	public:
		class CScope
		{
		public:
			std::string mName;
			std::vector<CRefScopedType> mTemplateArgTypes;
			CScope();
			~CScope();
		};
	public:
		std::vector<CScope> mScopes;
		std::string ToString(ulong, ulong);
		CRefScopedType();
		~CRefScopedType();
	};
	class CRefTypeNameParser
	{
	public:
		//FEAST is also defined here.
		class FeastException : public FEAST::XLibException
		{
		protected:
			std::string mText;
		public:
			virtual const char* LibExceptionGetText();
		};
		class FeastClient : public FEAST::ILibClient
		{
		public:
			virtual void* LibMalloc(ulong inSize);
			virtual void* LibRealloc(void* inPtr, ulong inNewSize);
			virtual void LibFree(void* inPtr);
			virtual void LibError(const char* inErrorStr);
		};
		enum EToken
		{
			TOKEN_None,
			TOKEN_Identifier,
			TOKEN_Integer
		};
		enum ENode
		{
			NODE_None,
			NODE_TermIdentifier,
			NODE_TermInteger,
			NODE_List,
			NODE_Scope,
			NODE_TemplateArg
		};
	protected:
		FEAST::IPrsParser* mParser;
		void InitGrammar(FEAST::IPrsParser* inParser);
		void BuildTemplateArgs(FEAST::IPrsNode* inNode, CRefScopedType::CScope& outScope);
		void BuildScopedType(FEAST::IPrsNode* inNode, CRefScopedType& outType);
	public:
		CRefTypeNameParser();
		~CRefTypeNameParser();
		bool ParseTypeName(const char* inTypeName, CRefScopedType& outScopedType);
		static CRefTypeNameParser* GetParser();
	};
	class CRefVariant
	{
	public:
		enum EVariantType
		{
			VT_DWord = 0x0000,
			VT_Float = 0x0001,
			VT_QWord = 0x0002,
			VT_Double = 0x0003,
			VT_Ptr = 0x0004,
		};
	public:
		EVariantType mType;
		union
		{
		public:
			DWORD mDWord;
			float mFloat;
			unsigned __int64 mQWord;
			double mDouble;
			void* mPtr;
		};
		CRefVariant() { mType = VT_DWord; mDWord = 0; } //147
		CRefVariant(ulong inValue) { mType = VT_DWord; mDWord = inValue; } //148

		CRefVariant(float inValue) { mType = VT_Float; mFloat = inValue; } //150
		CRefVariant(unsigned __int64 inValue) { mType = VT_QWord; mDWord = inValue; } //151

		CRefVariant(double inValue) { mType = VT_Double; mDouble = inValue; } //153
		CRefVariant(void* inValue) { mType = VT_Ptr; mDWord = (uint)inValue; } //154
		CRefVariant(__int64);
		CRefVariant(int);
		CRefVariant(unsigned int);
	};
	class RSymbol
	{
	public:
		union UTypePtr
		{
			RType* mPtr;
			ulong mHandle;
		};
	protected:
		TRefNamedSymbolCollection<RAttribute> mSymAttributes;
	public:
		RSymbol() {} //173
		virtual ~RSymbol() {} //174
		const TRefNamedSymbolCollection<RAttribute>* GetAttributes() const;
	};
	class RType : public RSymbol
	{
	public:
		enum ETypeCategory
		{
			TC_None,
			TC_Simple,
			TC_Reference,
			TC_Function,
			TC_Named_MASK = 0x01f0,
			TC_Named_Unknown = 0x0010,
			TC_Named_Class = 0x0020,
			TC_Named_ClassRef = 0x0040,
			TC_Named_Enum = 0x0080,
			TC_Named_EnumRef = 0x0100
		};
		enum ETypeFlags
		{
			TF_Const = 0x0001,
			TF_DisputedSize = 0x0100
		};
	protected:
		ulong mTypeFlags;
		ulong mTypeSize;
		ulong mTypeThisAdjust;
		RType() {} //217
	public:
		virtual ~RType(); //?
		virtual ETypeCategory GetTypeCategory() const = 0;
		virtual bool TypeEquals(RType* inType, bool inCheckConst, bool inExactMatch) const = 0;
		virtual std::string TypeToString(bool inCheckConst) const = 0;
		ulong GetSize() const { return mTypeSize; } //224
		ulong GetThisAdjust() const { return mTypeThisAdjust; } //225
		bool GetIsConst() const { return (mTypeFlags & TF_Const) != 0; } //Or TYPEF_Const prob not since that relates to PDBs themselves | 226
		bool GetIsDisputedSize() const;
		virtual std::string InstanceToString(const void* inInstancePtr) const //230-232
		{
			return "?";
		}
	};
	class RSimpleType : public RType
	{
	public:
		enum ESimpleTypeCategory
		{
			STC_None,
			STC_Ellipsis,
			STC_Void,
			STC_Bool,
			STC_AChar,
			STC_WChar,
			STC_SInt,
			STC_UInt,
			STC_Float,
			STC_HResult
		};
	protected:
		ESimpleTypeCategory mSimpleTypeCategory;
	public:
		//RSimpleType(); //?
		//virtual ~RSimpleType(); //?
		virtual bool TypeEquals(RType* inType, bool inCheckConst, bool inExactMatch) const;
		virtual std::string TypeToString(bool inCheckConst) const;
		std::string InstanceToString(const void* inInstancePtr, bool inUseHex) const;
		virtual ETypeCategory GetTypeCategory() const { return TC_Simple; } //265
		ESimpleTypeCategory GetSimpleTypeCategory() const { return mSimpleTypeCategory; } //269
		virtual std::string InstanceToString(const void* inInstancePtr) const { return InstanceToString(inInstancePtr, false); } //274-276
	};
	class RReferenceType : public RType
	{
	public:
		enum EReferenceTypeCategory
		{
			RTC_Ampersand,
			RTC_Pointer,
			RTC_Array,
		};
	protected:
		EReferenceTypeCategory mReferenceTypeCategory;
		UTypePtr mInnerType;
		ulong mArrayItemCount;
	public:
		//RReferenceType(); //?
		//virtual ~RReferenceType(); //?
		virtual ETypeCategory GetTypeCategory() const { return TC_Reference; } //302
		virtual bool TypeEquals(RType* inType, bool inCheckConst, bool inExactMatch) const;
		virtual std::string TypeToString(bool inCheckConst) const;
		EReferenceTypeCategory GetReferenceTypeCategory() const { return mReferenceTypeCategory; } //306
		RType* GetInnerType() const { return mInnerType.mPtr; } //307
		ulong GetArrayItemCount() const { return mArrayItemCount; } //308
		virtual std::string InstanceToString(const void* inInstancePtr) const { return InstanceToString(inInstancePtr, 0, 4); } //315;
		std::string InstanceToString(const void* inInstancePtr, ulong inArrayStart, ulong inArrayCount) const;
	};
	class RFunctionType : public RType
	{
	public:
		enum ECallType
		{
			CT_None,
			CT_ThisCall,
			CT_Cdecl,
			CT_StdCall,
			CT_FastCall,
			CT_SysCall,
		};
	protected:
		ECallType mCallType;
		UTypePtr mThisType;
		UTypePtr mReturnType;
		std::vector<UTypePtr> mArgTypes;
	public:
		RFunctionType();
		virtual ~RFunctionType();
		virtual ETypeCategory GetTypeCategory() const { return TC_Function; } //346
		virtual bool TypeEquals(RType* inType, bool inCheckConst, bool inExactMatch) const;
		virtual std::string TypeToString(bool inCheckConst) const;
		ECallType GetCallType() const { return mCallType; } //350
		RType* GetThisType() const { return mThisType.mPtr; } //351
		RType* GetReturnType() const { return mReturnType.mPtr; } //352
		ulong GetArgTypeCount() const { return mArgTypes.size(); } //353
		RType* GetArgTypeIndexed(ulong inIndex) const { return (RType*)mArgTypes[inIndex].mHandle; } //354
	};
	class RNamedType : public RType
	{
	protected:
		std::string mName;
	protected:
		RNamedType() {} //369
	public:
		virtual ~RNamedType();
		virtual bool TypeEquals(RType* inType, bool inCheckConst, bool inExactMatch) const;
		virtual std::string TypeToString(bool inCheckConst) const;
		const char* GetName() const { return mName.c_str(); } //375
		virtual std::string InstanceToString(const void* inInstancePtr) const;

		virtual RNamedType* GetDereferencedType() const { return; } //378 | Return
	};
	class RUnknownNamedType : public RNamedType
	{
	public:
		virtual ETypeCategory GetTypeCategory() const { return TC_Named_Unknown; } //388
		//RUnknownNamedType();
		//virtual ~RUnknownNamedType();
	};
	class RClass : public RNamedType
	{
	public:
		enum EClassFlags
		{
			CF_Union = 0x0001,
			CF_DisputedFlags = 0x0100,
			CF_DisputedVtblSize = 0x0200,
			CF_DisputedFields = 0x0400,
			CF_DisputedMethods = 0x0800,
			CF_DisputedAncestors = 0x1000,
			CF_DisputedMembers = 0x1c00,
			CF_ResolvedVirtualBases = 0x01000000,
			CF_Loaded = -2147483648
		};
		class CPreAttribute
		{
		public:
			std::string mMethodName;
			ulong mRVA;
			CPreAttribute();
			~CPreAttribute();
		};
	protected:
		CRefSymbolDb* mSymbolDb;
		ulong mClassDbIndex;
		ulong mClassFlags;
		ulong mVtblSize;
		TRefNamedSymbolCollection<RField> mFields;
		TRefNamedSymbolCollection<RMethod> mMethods;
		TRefNamedSymbolCollection<RAncestor> mAncestors;
		std::vector<CPreAttribute> mPreAttributes;
		TRefNamedSymbolCollection<RField> mAllFields;
		TRefNamedSymbolCollection<RMethod> mAllMethods;
		TRefNamedSymbolCollection<RAttribute> mAllAttributes;
		RMethod* FindVirtualBaseMethod(RMethod* inMethod);
		void ResolveVirtualBases();
	public:
		RClass();
		virtual ~RClass();
		virtual ETypeCategory GetTypeCategory() const { return TC_Named_Class; } //443
		bool GetIsUnion() const;
		ulong GetVtblSize() const { return mVtblSize; } //447

		const TRefNamedSymbolCollection<RField>* GetFields(bool inIncludeInherited) const { return inIncludeInherited ? &mAllFields : &mFields; }; //449
		const TRefNamedSymbolCollection<RMethod>* GetMethods(bool inIncludeInherited) const { return inIncludeInherited ? &mAllMethods : &mMethods; }; //450
		const TRefNamedSymbolCollection<RAncestor>* GetAncestors() const { return &mAncestors; } //451
		const TRefNamedSymbolCollection<RAttribute>* GetAttributes(bool inIncludeInherited) const { return inIncludeInherited ? &mAllAttributes : &mSymAttributes; }; //452
		virtual std::string InstanceToString(const void* inInstancePtr) const;
		RClass* GetPrimaryAncestor() const;
		void LoadClass();
	};
	class RClassRef : public RNamedType
	{
	protected:
		RClass* mClass;
	public:
		RClassRef();
		virtual ~RClassRef();
		virtual ETypeCategory GetTypeCategory() const { return TC_Named_ClassRef; } //473

		virtual std::string InstanceToString(const void* inInstancePtr) const //476-479
		{
			RClass* c = GetClass();
			return c ? c->RType::InstanceToString(inInstancePtr) : "?"; //?
		}

		RClass* GetClass(bool inEnsureLoaded = true) const //482-486
		{
			if (mClass && inEnsureLoaded)
				mClass->LoadClass();
			return mClass;
		}
		virtual RNamedType* GetDereferencedType() const { return GetClass(); } //488
		//RClassRef(const RClassRef&);
	};
	class REnumMember : public RSymbol
	{
	public:
		enum EEnumMemberFlags
		{
			EMF_Disputed = 0x0100,
		};
	protected:
		std::string mMemberName;
		ulong mMemberValue;
		REnum* mMemberOuter;
		ulong mEnumMemberFlags;
	public:
		REnumMember();
		virtual ~REnumMember();
		const char* GetName() const { return mMemberName.c_str(); } //512
		ulong GetValue() const { return mMemberValue; } //513
		REnum* GetOuter() const;
		bool GetIsDisputed() const;
	};
	class TRefNamedSymbolCollection<REnumMember> : public CRefNamedSymbolCollection
	{
	protected:
		typedef std::map<ulong, REnumMember*> DValueToSymbolMap;
		DValueToSymbolMap mValueToSymbolMap;
	public:
		REnumMember* GetIndexed(ulong index) const //532-534
		{
			return mSymbols[index];
		}
		REnumMember* GetNamed(const std::string& inName) const //536-541
		{
			DNameToSymbolMap::const_iterator it = mNameToSymbolMap.find(inName);
			if (it != mNameToSymbolMap.end())
				return NULL;
			return (REnumMember*)it->second; //?
		}
		REnumMember* GetByValue(ulong inValue) const //543-548
		{
			DValueToSymbolMap::const_iterator it = mValueToSymbolMap.find(inValue);
			if (it != mValueToSymbolMap.end())
				return NULL;
			return it->second;
		}
		bool AddSymbol(const std::string& inName, REnumMember* inSymbol) const //? | 550-560
		{
			if (!inName.empty() && GetNamed(inName))
				return false;
			InternalAddSymbol(inName, inSymbol);
			//?
			DValueToSymbolMap::const_iterator it = mValueToSymbolMap.find(inSymbol->GetValue());
			if (it == mValueToSymbolMap.end())
				inSymbol = mValueToSymbolMap[inSymbol->GetValue()];
			return true;
		}
	};
	class REnum : public RNamedType
	{
	public:
		enum EEnumFlags
		{
			EF_MembersAreFlags = 0x0001,
			EF_DisputedFlags = 0x0100,
			EF_DisputedMembers = 0x0200,
			EF_Loaded = -2147483648,
		};
	protected:
		CRefSymbolDb* mSymbolDb;
		ulong mEnumDbIndex;
		ulong mEnumFlags;
		TRefNamedSymbolCollection<REnumMember> mMembers;
	public:
		virtual ETypeCategory GetTypeCategory() const { return TC_Named_Enum; } //589

		bool GetIsFlags() const { (mEnumFlags & EF_MembersAreFlags) != 0; } //591

		const TRefNamedSymbolCollection<REnumMember>* GetMembers() const { return &mMembers; } //593
		virtual std::string InstanceToString(const void* inInstancePtr) const;
		void LoadEnum();
		REnum();
		~REnum();
	};
	class REnumRef : public RNamedType
	{
	protected:
		REnum* mEnum;
	public:
		virtual ETypeCategory GetTypeCategory() const { return TC_Named_EnumRef; } //613
		virtual std::string InstanceToString(const void* inInstancePtr) const //616-619
		{
			REnum* e = GetEnum(true);
			return e ? e->RType::InstanceToString(inInstancePtr) : "?";
		}


		REnum* GetEnum(bool inEnsureLoaded) const //622-626
		{
			if (mEnum && inEnsureLoaded)
				mEnum->LoadEnum();
			return mEnum;
		}
		virtual RNamedType* GetDereferencedType() const { return GetEnum(true); } //628
		REnumRef();
		~REnumRef();
	};
	class RClassMember : public RSymbol
	{
	public:
		enum EMemberAccess
		{
			MA_Private,
			MA_Protected,
			MA_Public,
		};
	protected:
		std::string mMemberName;
		EMemberAccess mMemberAccess;
		RClass* mMemberOuter;
	public:
		const char* GetName() const { return mMemberName.c_str(); } //Do we need a cast? Prob not | 654
		EMemberAccess GetAccess() const;
		RClass* GetOuter() const { return mMemberOuter; } //656
		RClassMember();
		~RClassMember();
		//RClassMember& operator=(const RClassMember&);
	};
	class RField : public RClassMember
	{
	public:
		enum EFieldFlags
		{
			FF_Static = 0x0001,
			FF_Disputed = 0x0100,
		};
	protected:
		ulong mFieldFlags;
		ulong mFieldOffset;
		UTypePtr mFieldType;
	public:
		bool GetIsStatic() const;
		bool GetIsDisputed() const { return (mFieldFlags & FF_Disputed) != 0; } //682

		ulong GetFieldOffset() const { return mFieldOffset; } //684

		RType* GetType() const { return mFieldType.mPtr; } //686
		RField();
		~RField();
		//RField& operator=(const RField&);
	};
	class RMethod : public RClassMember
	{
	public:
		enum EMethodFlags
		{
			MF_Virtual,
			MF_IntroVirtual,
			MF_PureVirtual = 0x0004,
			MF_CanInvokeStatic = 0x0010,
			MF_CanInvokeInstance = 0x0020,
			MF_Disputed = 0x0100,
		};
	protected:
		ulong mMethodFlags;
		ulong mRVA;
		ulong mVtblOffset;
		RMethod* mVirtualBase;
		UTypePtr mMethodType;
		void* mMethodInvokePtr;
	public:
		bool GetIsVirtual() const { return (mMethodFlags & MF_Virtual) != 0; } //719
		bool GetIsIntroVirtual() const;
		bool GetIsPureVirtual() const { return (mMethodFlags & MF_PureVirtual) != 0; }
		bool GetIsDisputed() const;
		RMethod* GetVirtualBase() const { return mVirtualBase; } //724
		ulong GetVtblOffset() const { return mVtblOffset; }
		ulong GetRVA() const;

		RType* GetType() const { return mMethodType.mPtr; } //728


		bool CanInvoke(bool inHasThis) const //731-736
		{
			if (inHasThis)
				return (mMethodFlags & (MF_CanInvokeInstance | MF_CanInvokeStatic)) != 0;
			else
				return (mMethodFlags & MF_CanInvokeStatic) != 0;
		}
		bool BuildInvokeArgBuffer(const std::vector<CRefVariant>& inArgs, std::vector<uchar>& outBuffer);
		bool Invoke(CRefVariant* outReturnValue, void* inThis, const void* inArgData, ulong inArgLen);

		bool Invoke(CRefVariant* outReturnValue, void* inThis, const std::vector<CRefVariant>& inArgs) //741-746 (UNMATCHED)
		{
			std::vector<uchar> argBuffer;
			if (BuildInvokeArgBuffer(inArgs, argBuffer))
				return Invoke(outReturnValue, inThis, argBuffer.size() ? &argBuffer[0] : 0, argBuffer.size()); //?
			return false;
		}

		//RMethod(const RMethod&);
		RMethod();
		~RMethod();
		//RMethod& operator=(const RMethod&);
	};
	class RAncestor : public RClassMember
	{
	public:
		enum EAncestorFlags
		{
			AF_Disputed = 0x0100,
		};
	protected:
		ulong mAncestorFlags;
		ulong mOffset;
		RClass* mAncestorClass;
	public:
		bool GetIsDisputed() const;
		ulong GetOffset() const { return mOffset; } //773


		RClass* GetClass(bool inEnsureLoaded) const //776-780 | always true
		{
			if (mAncestorClass && inEnsureLoaded)
				mAncestorClass->LoadClass();
			return mAncestorClass;
		}
		//RAncestor(const RAncestor&);
		RAncestor();
		~RAncestor();
		//RAncestor& operator=(const RAncestor&);
	};
	class RAttribute : public RSymbol
	{
	protected:
		std::string mName;
		RMethod* mMethod;
		CRefVariant mValue;
	public:
		const char* GetName() const { return mName.c_str(); } //798
		const CRefVariant& GetValue() const { return mValue; } //799
		//RAttribute(const RAttribute&);
		RAttribute();
		~RAttribute();
		//RAttribute& operator=(const RAttribute&);
	};
	class TRefNamedSymbolCollection<RClass> : public CRefNamedSymbolCollection
	{
		RClass* GetNamed(const std::string& inName, bool inEnsureLoaded = false) const //817-827 (UNMATCHED)
		{
			DNameToSymbolMap::const_iterator it = mNameToSymbolMap.find(inName);
			if (it != mNameToSymbolMap.end())

				return NULL;
			RClass* c = it->second;
			if (c && inEnsureLoaded)
				c->LoadClass();

			return c;
		}
		bool AddSymbol(const std::string& inName, RClass* inSymbol) const //829-834
		{
			if (!inName.empty() && GetNamed(inName))
				return false;
			InternalAddSymbol(inName, inSymbol);
			return true;
		}
		//_T& operator=(const _T&);
	};
	class TRefNamedSymbolCollection<REnum> : public CRefNamedSymbolCollection
	{
	public:
		REnum* GetNamed(const std::string& inName, bool inEnsureLoaded = true) const //848-858
		{
			DNameToSymbolMap::const_iterator it = mNameToSymbolMap.find(inName);
			if (it != mNameToSymbolMap.end())
				return NULL;
			REnum* e = it->second;
			if (e && inEnsureLoaded)
				e->LoadEnum();

			return e;
		}
		bool AddSymbol(const std::string& inName, REnum* inSymbol) //860-865
		{
			if (!inName.empty())
				return false;
			InternalAddSymbol(inName, inSymbol);
			return true;
		}
	};
	class CRefSymbolDb
	{
	public:
		enum EStringFlags //Quite lonely in here
		{
			STRINGF_NoShowPointers = 1,
		};
	protected:
		static ulong sStringFlags;
		TRefNamedSymbolCollection<RType> mTypes;
		TRefNamedSymbolCollection<RClass> mClasses;
		TRefNamedSymbolCollection<REnum> mEnums;
		IPdbReader* mReader;
		HANDLE mModuleHandle;
		bool mModuleIsFile;
		RNamedType* GetTypeForRTTITypeName(const char* inTypeName);
	public:
		CRefSymbolDb();
		~CRefSymbolDb();
		bool InitFromModule(HANDLE inModuleHandle, bool inModuleIsFile, const char* inModuleFileName, const char* inPdbFileName);
		static bool HasStringFlags(ulong inFlags) { return (inFlags & sStringFlags) != 0; } //894
		static void AddStringFlags(ulong inFlags) { inFlags |= sStringFlags; } //895
		static void RemoveStringFlags(ulong inFlags) { inFlags &= ~sStringFlags; } //896
		static void ToggleStringFlag(ulong inFlag) { HasStringFlags(inFlag) ? RemoveStringFlags(inFlag) : AddStringFlags(inFlag); } //897
		bool GetIsInitialized() { return mReader != NULL; } //904

		const TRefNamedSymbolCollection<RClass>* GetClasses() const { return &mClasses; } //906
		const TRefNamedSymbolCollection<REnum>* GetEnums() const { return &mEnums; } //907


		template <class _T> RNamedType* GetInstanceType(_T* inSymbol) //910
		{
			type_info* ti = inSymbol; ///?
			return GetTypeForRTTITypeName(inSymbol);
		}
	};
}
#endif