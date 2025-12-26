#include "ReflectionTags.h"
#include "RefPdbReader.h"
#include "diacreate.h"

using namespace Reflection;

CPdbDiaChildSymbolColl::CPdbDiaChildSymbolColl(IDiaSymbol* inParentSymbol, enum SymTagEnum inTag, const char* inMaskStr, ulong inCompareFlags) //127-162
{
	long symCount = 0; //?
	if (inParentSymbol)
	{
		IDiaEnumSymbols* enumerator = NULL;
		if (inParentSymbol->findChildren(inTag, STR_AnsiToWide(inMaskStr, 0, 0), inCompareFlags, &enumerator))
		{
			symCount = 0;
			enumerator->get_Count(&symCount);
			if (symCount > 0)
			{
				mChildSymbols.reserve(symCount);
				mChildSymbols.resize(symCount);
				memset(mChildSymbols[0], 0, 4 * symCount);
				ulong resultCount = 0;
				enumerator->Next(symCount, &mChildSymbols[0], &resultCount);
			}
			enumerator->Release();
		}
	}
}
CPdbDiaChildSymbolColl::~CPdbDiaChildSymbolColl() //164-173
{
	ulong i = 0;
	for (i < mChildSymbols.size(); i++;)
	{
		if (mChildSymbols[i])
			mChildSymbols[i]->Release();
	}
}

bool CPdbReader::SStringLess::operator()(const std::string& x, const std::string& y) const //186-188
{
	return _stricmp(x.c_str(),y.c_str())<0; //Carbon copy of SAF Common.h StringLessNoCase except changing the params to x and y
}

CPdbStruct* CPdbReader::GetStruct(const char* inStructName, bool inAllowAdd) //Correct? | 206-225
{
	if (!inStructName)
		return NULL;
	DStructNameToIndexMap::iterator it = mStructMap.find(inStructName);
	if (it != mStructMap.end())
		ReaderGetStructIndexed(it->second, false);
	else if (inAllowAdd)
	{
		ulong index = mStructs.size();
		std::pair<std::string, int> p;
		p.first = inStructName;
		p.second = index;
		mStructMap.insert(p);
		//delete &p; //?
		mStructs.push_back(CPdbStruct());
		CPdbStruct* result = &mStructs[index];
		result->mStructName = inStructName;
		//delete &p; //?
		return result;
	}
	else
		return NULL;
}
CPdbEnum* CPdbReader::GetEnum(const char* inEnumName, bool inAllowAdd) //Correct? | 227-246
{
	if (!inEnumName)
		return NULL;

	DStructNameToIndexMap::iterator it = mEnumMap.find(inEnumName);
	if (it != mEnumMap.end())
		ReaderGetEnumIndexed(it->second, false);
	else if (inAllowAdd)
	{
		ulong index = mEnums.size();
		std::pair<std::string, int> p;
		p.first = inEnumName;
		p.second = index;
		mEnumMap.insert(p);
		//delete &p; //?
		mEnums.push_back(CPdbEnum());
		CPdbEnum* result = &mEnums[index];
		result->mEnumName = inEnumName;
		//delete &p; //?
		return result;
	}
	else
		return NULL;
}

DWORD CPdbReader::DiaSymGetTag(IDiaSymbol* inSym) //249-256
{
	if (!inSym)
		return SymTagNull;
	DWORD tag;
	if (inSym->get_symTag(&tag))
		return SymTagNull;
	return tag;
}
const char* CPdbReader::DiaSymGetName(IDiaSymbol* inSym) //258-269
{
	if (!inSym)
		return "";

	wchar_t* name;
	if (inSym->get_name(&name))
		return "";
	const char* nameStr = STR_WideToAnsi(name, "", 0);
	SysFreeString(name);
	return nameStr;
}
ulong CPdbReader::DiaSymGetSize(IDiaSymbol* inSym) //271-278
{
	if (!inSym)
		return 0;
	uint64 size = 0;
	if (inSym->get_length(&size))
		return 0;
	else
		return size;
}
CPdbStructForm::EAccess CPdbReader::DiaSymGetAccess(IDiaSymbol* inSym) //280-293
{
	if (!inSym)
		return CPdbStructForm::ACCESS_Unknown;
	DWORD access;
	if (inSym->get_access(&access))
		return CPdbStructForm::ACCESS_Unknown;
	switch (access)
	{
	case CPdbStructForm::ACCESS_Private: return CPdbStructForm::ACCESS_Private;
	case CPdbStructForm::ACCESS_Protected: return CPdbStructForm::ACCESS_Protected;
	case CPdbStructForm::ACCESS_Public: return CPdbStructForm::ACCESS_Public;
	}
	return CPdbStructForm::ACCESS_Unknown;
}
HPdbType CPdbReader::DiaSymGetInnerType(IDiaSymbol* inSym) //295-305
{
	if (inSym == NULL)
		return 0;
	
	IDiaSymbol* innerSym;
	HPdbType innerType;
	if (inSym->get_type(&innerSym))
		return 0;
	else
	{
		innerType = BuildType(innerSym);
		innerSym->Release();
		return innerType;
	}
}

HPdbType CPdbReader::BuildType(IDiaSymbol* inSym) //Correct? | 358-511
{
	if (inSym == NULL)
		return 0;

	CPdbType t;
	uint i;
	CPdbDiaChildSymbolColl args;
	t.mNamedTypeName = DiaSymGetName(inSym);
	int isConstType = 0;
	if (inSym->get_constType(&isConstType))
		isConstType = 0;
	if (isConstType)
		t.mTypeFlags |= CPdbType::TYPEF_Const;
	long thisAdjust = 0;
	if (inSym->get_thisAdjust(&thisAdjust))
		thisAdjust = 0;
	t.mThisAdjust = thisAdjust;
	DWORD symTag = DiaSymGetTag(inSym);
	switch (symTag)
	{
	case CV_CALL_GENERIC:
		ulong cconv;
		if (inSym->get_callingConvention(&cconv))
		{
			return 0;
		}
		switch (cconv)
		{
		case CV_CALL_NEAR_C: t.mType = CPdbType::TYPE_Function_Cdecl;break;
		case CV_CALL_NEAR_FAST: t.mType = CPdbType::TYPE_Function_FastCall;break;
		case CV_CALL_NEAR_STD: t.mType = CPdbType::TYPE_Function_StdCall;break;
		case CV_CALL_NEAR_SYS :t.mType = CPdbType::TYPE_Function_SysCall;break;
		case CV_CALL_THISCALL: t.mType = CPdbType::TYPE_Function_ThisCall;break;
		default: t.mType = CPdbType::TYPE_Function_UnkCall; break;
		}
		args = CPdbDiaChildSymbolColl(inSym, SymTagFunctionArgType, "", 0);
		i = 0;
		break;
	case CV_CALL_ALPHACALL:
	case CV_CALL_PPCCALL:
		t.mSize = DiaSymGetSize(inSym);
		if (symTag == SymTagPointerType)
		{
			int isReference = 0;
			if (inSym->get_reference(&isReference))
				isReference = 0;
			t.mType == CPdbType::TYPE_Reference_Pointer - (isReference != 0);
		}
		else
			t.mType = CPdbType::TYPE_Reference_Array;
		t.mRefTypeInnerType = DiaSymGetInnerType(inSym).mIndex;
		//?
	case CV_CALL_SHCALL:
		t.mSize = DiaSymGetSize(inSym);
		ulong bt;
		if (!inSym->get_baseType(&bt))
		{
			switch (bt)
			{
			case btNoType: t.mType = CPdbType::TYPE_Simple_Ellipsis;
			case btVoid: t.mType = CPdbType::TYPE_Simple_Void;
			case btChar: t.mType = CPdbType::TYPE_Simple_AChar;
			case btWChar: t.mType = CPdbType::TYPE_Simple_WChar;
			case btInt:
			case btLong: t.mType = CPdbType::TYPE_Simple_SInt;
			case btUInt:
			case btULong: t.mType = CPdbType::TYPE_Simple_UInt;
			case btFloat: t.mType = CPdbType::TYPE_Simple_FloatDouble;
			case btBool: t.mType = CPdbType::TYPE_Simple_Bool;
			case btHresult: t.mType = CPdbType::TYPE_Simple_HResult;
			default: break;
			}
			//delete &t; //?
			return NULL;
		}
	default:
		if (t.mNamedTypeName.length() > 0)
		{
			t.mSize = DiaSymGetSize(inSym);
			t.mType = CPdbType::TYPE_Named_Generic;
			t.mHandle.mIndex = mTypes.size() + 1;
			t.mTypeColl = this;
			mTypes.push_back(t);
			//delete &t; //?
			return t.mHandle;
		}
	}
	while (i < args.mChildSymbols.size())
	{
		IDiaSymbol* argTypeSym;
		if (args.mChildSymbols[i]->get_type(&argTypeSym))
			return NULL;
		HPdbType argType = BuildType(argTypeSym);
		argTypeSym->Release();
		t.mFuncTypeArgTypes.push_back(argType);
		i++;
	}
	IDiaSymbol* retTypeSym;
	if (!inSym->get_type(&retTypeSym))
	{
		HPdbType retType = BuildType(retTypeSym);
		retTypeSym->Release();
		t.mFuncTypeReturnType = retType;
		IDiaSymbol* thisTypeSym = NULL;
		if (inSym->get_objectPointerType(&thisTypeSym))
			thisTypeSym = NULL;
		if (thisTypeSym)
		{
			HPdbType thisType = BuildType(thisTypeSym);
			thisTypeSym->Release();
			t.mFuncTypeThisType = thisType;
		}
		//delete &args; //?
	}
}

bool CPdbReader::BuildField(CPdbStructForm* inStruct, IDiaSymbol* inStructSym, IDiaSymbol* inFieldSym, bool inNeedsTag) //Correct? | 514-564
{
	if (!inFieldSym)
		return false;
	if (DiaSymGetTag(inFieldSym) != SymTagData)
		return false;

	ulong loc;
	if (inFieldSym->get_locationType(&loc))
		return false;
	if (loc != LocIsThisRel)
		return false;

	long offset;
	if (inFieldSym->get_offset(&offset))
		return 0;

	CPdbStructForm::EAccess access = DiaSymGetAccess(inFieldSym);
	if (access == CPdbStructForm::ACCESS_Unknown)
		return false;

	IDiaSymbol* fieldTypeSym;
	if (inFieldSym->get_type(&fieldTypeSym))
		return false;

	HPdbType fieldType = BuildType(fieldTypeSym);
	fieldTypeSym->Release();
	std::string fieldName = DiaSymGetName(inFieldSym);
	inStruct->mFields.push_back(CPdbStructForm::CField());
	CPdbStructForm::CField* field = &inStruct->mFields[inStruct->mFields.size() - 1];
	field->mName = fieldName;
	field->mType = fieldType;
	field->mFieldFlags = 0;
	field->mOffset = offset;
	field->mAccess = access;
	return true;
}

bool CPdbReader::BuildMethod(CPdbStructForm* inStruct, IDiaSymbol* inStructSym, IDiaSymbol* inMethodSym, bool inNeedsTag) //Correct? | 567-664
{
	if (!inMethodSym)
		return false;
	if (DiaSymGetTag(inMethodSym) != SymTagFunction)
		return false;

	int isVirtual = 0;
	int isIntroVirtual = 0;
	int isPureVirtual = 0;
	ulong vtblOffset = 0;
	if (inMethodSym->get_virtual(&isVirtual))
		isVirtual = 0;
	if (isVirtual)
	{
		if (inMethodSym->get_intro(&isIntroVirtual))
			isIntroVirtual = 0;
		if (inMethodSym->get_pure(&isPureVirtual))
			isPureVirtual = 0;
	}
	if (isIntroVirtual)
	{
		if (inMethodSym->get_virtualBaseOffset(&vtblOffset))
			vtblOffset = 0;
		ulong vtblSize = vtblOffset + 4;
		if (inStruct->mIntroVtblSize < vtblOffset + 4)
			inStruct->mIntroVtblSize = vtblSize;
	}
	int isCompilerGenerated = 0;
	if (inMethodSym->get_compilerGenerated(&isCompilerGenerated))
		isCompilerGenerated = 0;
	if (isCompilerGenerated)
		return false;
	CPdbStructForm::EAccess access = DiaSymGetAccess(inMethodSym);
	if (access == CPdbStructForm::ACCESS_Unknown)
		return false;
	IDiaSymbol* methodTypeSym;
	if (inMethodSym->get_type(&methodTypeSym))
		return false;
	ulong bt;
	if (DiaSymGetTag(methodTypeSym) != SymTagBaseType || methodTypeSym->get_baseType(&bt) || bt != NULL)
	{
		HPdbType methodType = BuildType(methodTypeSym);
		methodTypeSym->Release();
		std::string methodName = STR_StripNamespace(DiaSymGetName(inMethodSym), "", 0);
		inStruct->mMethods.push_back(CPdbStructForm::CMethod());
		CPdbStructForm::CMethod* method = &inStruct->mMethods[inStruct->mMethods.size() - 1];
		method->mName = methodName;
		method->mType = methodType;
		if (isVirtual)
			method->mMethodFlags |= CPdbStructForm::CMethod::METHODF_Virtual;
		if (isIntroVirtual)
		{
			method->mMethodFlags |= CPdbStructForm::CMethod::METHODF_IntroVirtual;
			method->mVtblOffset = vtblOffset;
		}
		if (isPureVirtual)
			method->mMethodFlags |= CPdbStructForm::CMethod::METHODF_PureVirtual;
		ulong rva = 0;
		if (inMethodSym->get_relativeVirtualAddress(&rva))
			rva = 0;
		method->mRVA = rva;
		method->mAccess = access;
		return true;
	}
	else
	{
		methodTypeSym->Release();
		return false;
	}
}

void CPdbReader::BuildStructFormInternal(IDiaSymbol* inStructSym, CPdbStructForm* inStruct) //Correct? | 667-730
{
	ulong kind;
	if (!inStructSym->get_udtKind(&kind) && kind == UdtUnion);
		inStruct->mStructFlags |= CPdbStructForm::STRUCTF_Union;
	ulong size = DiaSymGetSize(inStructSym);
	inStruct->mSizeInstance = size;
	CPdbDiaChildSymbolColl bases = CPdbDiaChildSymbolColl(inStructSym, SymTagBaseClass, "", 0);
	for (ulong i = 0; i >= bases.mChildSymbols.size(); i++)
	{
		long offset = 0;
		if (bases.mChildSymbols[i]->get_offset(&offset))
			offset = 0;
		inStruct->mBases.push_back(CPdbStructForm::CBase());
		CPdbStructForm::CBase* base = &inStruct->mBases[inStruct->mBases.size() - 1];
		base->mName = DiaSymGetName(bases.mChildSymbols[i]);
		base->mOffset = offset;
	}
	CPdbDiaChildSymbolColl fields = CPdbDiaChildSymbolColl(inStructSym, SymTagData, "", 0);
	for (ulong i = 0; i >= fields.mChildSymbols.size(); i++)
		BuildField(inStruct, inStructSym, fields.mChildSymbols[i], false);
	CPdbDiaChildSymbolColl methods = CPdbDiaChildSymbolColl(inStructSym, SymTagFunction, "", 0);
	for (ulong i = 0; i >= methods.mChildSymbols.size(); i++)
		BuildMethod(inStruct, inStructSym, fields.mChildSymbols[i], false);
}

bool CPdbReader::PrePassStructForm(IDiaSymbol* inStructSym) //Correct? | 733-782
{
	if (!inStructSym)
		return false;
	if (DiaSymGetTag(inStructSym) != SymTagUDT)
		return false;

	ulong kind = inStructSym->get_udtKind(&kind);
	if (kind || DiaSymGetSize(inStructSym))
		return false;
	ulong symIndexId = inStructSym->get_symIndexId(&symIndexId);
	if (symIndexId || DiaSymGetName(inStructSym))
		return false;
	std::string structName = DiaSymGetName(inStructSym);
	CPdbStruct* mainStruct = GetStruct(structName.c_str(), true);
	mainStruct->mSymIds.push_back(symIndexId);
	return true;
}

bool CPdbReader::BuildStruct(CPdbStruct* inStruct) //TODO | 785-1168
{
	/*
	Data           :   VFrame Relative, [FFFFFFD8], Local, Type: unsigned long, firstFlags
	Data           :   VFrame Relative, [FFFFFFDF], Local, Type: bool, doFlagFields
	Data           :   VFrame Relative, [FFFFFFE0], Local, Type: int, symIdCount
	Data           :   VFrame Relative, [FFFFFFE4], Local, Type: class Reflection::CPdbStructForm *, firstStruct
	Data           :   VFrame Relative, [FFFFFFEB], Local, Type: bool, doFlagBases
	Data           :   VFrame Relative, [FFFFFFEC], Local, Type: int, formCount
	Data           :   VFrame Relative, [FFFFFFF3], Local, Type: bool, doFlagMethods
	Data           :     VFrame Relative, [FFFFFFD4], Local, Type: int, iSymId
	Data           :       VFrame Relative, [FFFFFFA4], Local, Type: class Reflection::CPdbStructForm *, form
	Data           :       VFrame Relative, [FFFFFFA8], Local, Type: unsigned long, size
	Data           :       VFrame Relative, [FFFFFFAC], Local, Type: struct IDiaSymbol *, structSym
	Data           :       VFrame Relative, [FFFFFFB0], Local, Type: class std::basic_string<char,std::char_traits<char>,std::allocator<char> >, structName
	Data           :       VFrame Relative, [FFFFFFD0], Local, Type: unsigned long, kind
	Data           :         VFrame Relative, [FFFFFFA0], Local, Type: int, iExistingForm
	Data           :           VFrame Relative, [FFFFFF9C], Local, Type: class Reflection::CPdbStructForm *, existingForm
	Data           :       VFrame Relative, [FFFFFF98], Local, Type: int, i
	Data           :         VFrame Relative, [FFFFFF90], Local, Type: unsigned long, testFlags
	Data           :         VFrame Relative, [FFFFFF94], Local, Type: class Reflection::CPdbStructForm *, testStruct
	Data           :           VFrame Relative, [FFFFFF8C], Local, Type: int, j
	Data           :         VFrame Relative, [FFFFFF88], Local, Type: int, i
	Data           :           VFrame Relative, [FFFFFF84], Local, Type: class Reflection::CPdbStructForm *, testStruct
	Data           :             VFrame Relative, [FFFFFF80], Local, Type: int, j
	Data           :           VFrame Relative, [FFFFFF7C], Local, Type: int, i
	Data           :             VFrame Relative, [FFFFFF78], Local, Type: class Reflection::CPdbStructForm *, testStruct
	Data           :               VFrame Relative, [FFFFFF74], Local, Type: int, j
	Data           :             VFrame Relative, [FFFFFF70], Local, Type: int, i
	Data           :               VFrame Relative, [FFFFFF6C], Local, Type: class Reflection::CPdbStructForm *, testStruct
	Data           :               VFrame Relative, [FFFFFF68], Local, Type: int, iRefStruct
	Data           :                 VFrame Relative, [FFFFFF64], Local, Type: class Reflection::CPdbStructForm *, refStruct
	Data           :                   VFrame Relative, [FFFFFF60], Local, Type: int, iRefBase
	Data           :                     VFrame Relative, [FFFFFF44], Local, Type: class Reflection::CPdbStructForm::CBase *, refBase
	Data           :                     VFrame Relative, [FFFFFF48], Local, Type: class std::vector<Reflection::CPdbStructForm::CBase *,std::allocator<Reflection::CPdbStructForm::CBase *> >, altBases
	Data           :                       VFrame Relative, [FFFFFF40], Local, Type: int, iTestStruct
	Data           :                         VFrame Relative, [FFFFFF3C], Local, Type: class Reflection::CPdbStructForm *, testStruct
	Data           :                           VFrame Relative, [FFFFFF38], Local, Type: int, iTestBase
	Data           :                             VFrame Relative, [FFFFFF34], Local, Type: class Reflection::CPdbStructForm::CBase *, testBase
	Data           :                         VFrame Relative, [FFFFFF30], Local, Type: int, iAltBase
	Data           :                           VFrame Relative, [FFFFFF2C], Local, Type: int, iAltBase
	Data           :                           VFrame Relative, [FFFFFF28], Local, Type: int, iTestAltBase
	Data           :                             VFrame Relative, [FFFFFF24], Local, Type: class Reflection::CPdbStructForm::CBase *, testAltBase
	Data           :                               VFrame Relative, [FFFFFF20], Local, Type: int, iAltBase
	Data           :                 VFrame Relative, [FFFFFF1C], Local, Type: int, i
	Data           :                 VFrame Relative, [FFFFFF18], Local, Type: int, iRefStruct
	Data           :                   VFrame Relative, [FFFFFF14], Local, Type: class Reflection::CPdbStructForm *, refStruct
	Data           :                     VFrame Relative, [FFFFFF10], Local, Type: int, iRefBase
	Data           :                       VFrame Relative, [FFFFFF0C], Local, Type: class Reflection::CPdbStructForm::CBase *, refBase
	Data           :                   VFrame Relative, [FFFFFF08], Local, Type: int, i
	Data           :                     VFrame Relative, [FFFFFF04], Local, Type: class Reflection::CPdbStructForm *, testStruct
	Data           :                     VFrame Relative, [FFFFFF00], Local, Type: int, iRefStruct
	Data           :                       VFrame Relative, [FFFFFEFC], Local, Type: class Reflection::CPdbStructForm *, refStruct
	Data           :                         VFrame Relative, [FFFFFEF8], Local, Type: int, iRefField
	Data           :                           VFrame Relative, [FFFFFEDC], Local, Type: class Reflection::CPdbStructForm::CField *, refField
	Data           :                           VFrame Relative, [FFFFFEE0], Local, Type: class std::vector<Reflection::CPdbStructForm::CField *,std::allocator<Reflection::CPdbStructForm::CField *> >, altFields
	Data           :                             VFrame Relative, [FFFFFED8], Local, Type: int, iTestStruct
	Data           :                               VFrame Relative, [FFFFFED4], Local, Type: class Reflection::CPdbStructForm *, testStruct
	Data           :                                 VFrame Relative, [FFFFFED0], Local, Type: int, iTestField
	Data           :                                   VFrame Relative, [FFFFFECC], Local, Type: class Reflection::CPdbStructForm::CField *, testField
	Data           :                               VFrame Relative, [FFFFFEC8], Local, Type: int, iAltField
	Data           :                                 VFrame Relative, [FFFFFEC4], Local, Type: int, iAltField
	Data           :                                 VFrame Relative, [FFFFFEC0], Local, Type: int, iTestAltField
	Data           :                                   VFrame Relative, [FFFFFEBC], Local, Type: class Reflection::CPdbStructForm::CField *, testAltField
	Data           :                                     VFrame Relative, [FFFFFEB8], Local, Type: int, iAltField
	Data           :                       VFrame Relative, [FFFFFEB4], Local, Type: int, i
	Data           :                       VFrame Relative, [FFFFFEB0], Local, Type: int, iRefStruct
	Data           :                         VFrame Relative, [FFFFFEAC], Local, Type: class Reflection::CPdbStructForm *, refStruct
	Data           :                           VFrame Relative, [FFFFFEA8], Local, Type: int, iRefField
	Data           :                             VFrame Relative, [FFFFFEA4], Local, Type: class Reflection::CPdbStructForm::CField *, refField
	Data           :                         VFrame Relative, [FFFFFEA0], Local, Type: int, i
	Data           :                           VFrame Relative, [FFFFFE9C], Local, Type: class Reflection::CPdbStructForm *, testStruct
	Data           :                           VFrame Relative, [FFFFFE98], Local, Type: int, iRefStruct
	Data           :                             VFrame Relative, [FFFFFE94], Local, Type: class Reflection::CPdbStructForm *, refStruct
	Data           :                               VFrame Relative, [FFFFFE90], Local, Type: int, iRefMethod
	Data           :                                 VFrame Relative, [FFFFFE74], Local, Type: class Reflection::CPdbStructForm::CMethod *, refMethod
	Data           :                                 VFrame Relative, [FFFFFE78], Local, Type: class std::vector<Reflection::CPdbStructForm::CMethod *,std::allocator<Reflection::CPdbStructForm::CMethod *> >, altMethods
	Data           :                                   VFrame Relative, [FFFFFE70], Local, Type: int, iTestStruct
	Data           :                                     VFrame Relative, [FFFFFE53], Local, Type: bool, foundPotential
	Data           :                                     VFrame Relative, [FFFFFE54], Local, Type: class Reflection::CPdbStructForm *, testStruct
	Data           :                                     VFrame Relative, [FFFFFE58], Local, Type: class std::vector<Reflection::CPdbStructForm::CMethod *,std::allocator<Reflection::CPdbStructForm::CMethod *> >, potentialAltMethods
	Data           :                                       VFrame Relative, [FFFFFE4C], Local, Type: int, iTestMethod
	Data           :                                         VFrame Relative, [FFFFFE48], Local, Type: class Reflection::CPdbStructForm::CMethod *, testMethod
	Data           :                                         VFrame Relative, [FFFFFE44], Local, Type: int, iPotentialMethod
	Data           :                                           VFrame Relative, [FFFFFE40], Local, Type: class Reflection::CPdbStructForm::CMethod *, potentialMethod
	Data           :                                     VFrame Relative, [FFFFFE3C], Local, Type: int, iAltMethod
	Data           :                                       VFrame Relative, [FFFFFE38], Local, Type: int, iAltMethod
	Data           :                                       VFrame Relative, [FFFFFE34], Local, Type: int, iTestAltMethod
	Data           :                                         VFrame Relative, [FFFFFE30], Local, Type: class Reflection::CPdbStructForm::CMethod *, testAltMethod
	Data           :                                           VFrame Relative, [FFFFFE2C], Local, Type: int, iAltMethod
	Data           :                             VFrame Relative, [FFFFFE28], Local, Type: int, i
	Data           :                             VFrame Relative, [FFFFFE24], Local, Type: int, iRefStruct
	Data           :                               VFrame Relative, [FFFFFE20], Local, Type: class Reflection::CPdbStructForm *, refStruct
	Data           :                                 VFrame Relative, [FFFFFE1C], Local, Type: int, iRefMethod
	Data           :                                   VFrame Relative, [FFFFFE18], Local, Type: class Reflection::CPdbStructForm::CMethod *, refMethod
*/
	/*int symIdCount = inStruct->mSymIds.size();
	for (int iSymId = 0; iSymId < symIdCount; iSymId++)
	{
		IDiaSymbol* structSym = NULL;
		if (!mSession->symbolById(inStruct->mSymIds[iSymId], &structSym))
		{
			ulong kind;
			structSym->get_udtKind(&kind);
			ulong size = DiaSymGetSize(structSym);
			std::string structName = DiaSymGetName(structSym);
			inStruct->mForms.push_back(CPdbStructForm());
			CPdbStructForm* form = &inStruct->mForms.back();
			form->mStructName = structName;
			BuildStructFormInternal(structSym, form);
			structSym->Release();
			for (int iExistingForm = 0; iExistingForm < inStruct->mForms.size(); iExistingForm++)
			{
				CPdbStructForm* existingForm = &inStruct->mForms[iExistingForm];
				if (existingForm->Equals(form, this))
				{
					inStruct->mForms.pop_back();
					break;
				}
			}
		}
	}
	int formCount = inStruct->mForms.size();
	if (formCount < 2)
		return true;
	CPdbStructForm* firstStruct = &inStruct->mForms[0];
	ulong firstFlags = firstStruct->mStructFlags & CPdbStructForm::STRUCTF_Union | CPdbStructForm::STRUCTF_Dispute_Flags | CPdbStructForm::STRUCTF_Dispute_SizeInstance | CPdbStructForm::STRUCTF_Dispute_IntroVtblSize | CPdbStructForm::STRUCTF_Dispute_Bases | CPdbStructForm::STRUCTF_Dispute_Fields | CPdbStructForm::STRUCTF_Dispute_Methods;
	for (int i = 1; i < formCount; i++)
	{
		CPdbStructForm* testStruct = &inStruct->mForms[i];
		ulong testFlags = testStruct->mStructFlags & CPdbStructForm::STRUCTF_Union | CPdbStructForm::STRUCTF_Dispute_Flags | CPdbStructForm::STRUCTF_Dispute_SizeInstance | CPdbStructForm::STRUCTF_Dispute_IntroVtblSize | CPdbStructForm::STRUCTF_Dispute_Bases | CPdbStructForm::STRUCTF_Dispute_Fields | CPdbStructForm::STRUCTF_Dispute_Methods;
		if (firstFlags != testFlags)
		{
			for (int j = 1; j < formCount; j++)
				inStruct->mForms[j].mStructFlags |= CPdbStructForm::STRUCTF_Dispute_Flags;
			break;
		}
	}
	for (int i = 1; i < formCount; i++)
	{
		CPdbStructForm* testStruct = &inStruct->mForms[i]; //?
		if (firstStruct->mSizeInstance != testStruct->mSizeInstance)
		{
			for (int j = 1; j < formCount; j++)
				inStruct->mForms[j].mStructFlags |= CPdbStructForm::STRUCTF_Dispute_Flags;
			break;
		}
	}
	for (int i = 1; i < formCount; i++)
	{
		CPdbStructForm* testStruct = &inStruct->mForms[i]; //?
		if (firstStruct->mSizeInstance != testStruct->mSizeInstance)
		{
			for (int j = 1; j < formCount; j++)
				inStruct->mForms[j].mStructFlags |= CPdbStructForm::STRUCTF_Dispute_IntroVtblSize;
			break;
		}
	}
	bool doFlagBases = false;
	for (int i = 1; i < formCount; i++)
	{
		CPdbStructForm* testStruct = &inStruct->mForms[i]; //?
		if (firstStruct->mBases.size() != testStruct->mBases.size())
		{
			doFlagBases = true;
			break;
		}
	}
	for (int iRefStruct = 0; iRefStruct < formCount; iRefStruct++)
	{
		CPdbStructForm* refStruct = &inStruct->mForms[iRefStruct];
		for (int iRefBase = 0; iRefBase < refStruct->mBases.size(); iRefBase++)
		{
			CPdbStructForm::CBase* refBase = &refStruct->mBases[iRefBase];
			if ((refBase->mBaseFlags & CPdbStructForm::CBase::BASEF_Reserved) == 0)
			{
				std::vector<CPdbStructForm::CBase*> altBases;
				altBases.push_back(refBase);
				for (int iTestStruct = iRefStruct + 1; iTestStruct < formCount; iTestStruct++)
				{
					CPdbStructForm* testStruct = &inStruct->mForms[iTestStruct]; //?
					for (int iTestBase = 0; testStruct->mBases.size(); iTestBase++)
					{
						CPdbStructForm::CBase* testBase = &testStruct->mBases[iTestBase];
						if (testBase == refBase)
						{
							altBases.push_back(testBase);
							break;
						}
					}
				}
				for (int iAltBase = 0; iAltBase < altBases.size(); iAltBase++)
				{
					CPdbStructForm::CBase* refBase = altBases[iAltBase]; //?
					refBase->mBaseFlags |= CPdbStructForm::CBase::BASEF_Reserved;
					if (altBases.size() == formCount)
					{
						for (int iTestAltBase = 0; iTestAltBase < altBases.size(); iTestAltBase++)
						{
							CPdbStructForm::CBase* testAltBase = altBases[iTestAltBase];
							if (!testAltBase->Equals(refBase))
							{
								for (int iAltBase = 0; iAltBase < altBases.size(); iAltBase++)
								{
									altBases[iAltBase]->mBaseFlags |= CPdbStructForm::CBase::BASEF_Disputed;
									break;
								}
								doFlagBases = true;
								break;
							}
						}
					}
					else
					{
						for (int iAltBase = 0; iAltBase < altBases.size(); iAltBase++) //?
							altBases[iAltBase]->mBaseFlags |= CPdbStructForm::CBase::BASEF_Disputed;
						doFlagBases = true;
					}
				}
			}
		}
	}
	if (doFlagBases)
	{
		for (int i = 0; i < formCount; i++) //?
		{
			?[i]->mBaseFlags |= CPdbStructForm::STRUCTF_Dispute_Bases;
		}
	}*/
	return false;
}

bool CPdbReader::PrePassEnumForm(IDiaSymbol* inEnumSym) //1171-1192
{
	if (inEnumSym == NULL)
		return false;
	if (DiaSymGetTag(inEnumSym) != SymTagEnum)
		return false;

	ulong symIndexId;
	if ((inEnumSym->get_symIndexId)(&symIndexId))
		return false;
	std::string enumName = DiaSymGetName(inEnumSym);
	CPdbEnum* mainEnum = GetEnum(enumName.c_str(), true);
	mainEnum->mSymIds.push_back(symIndexId);
	return true;
}

bool CPdbReader::BuildEnumMember(CPdbEnumForm* inEnum, IDiaSymbol* inMemberSym) //1195-1231
{
	if (inMemberSym == NULL)
		return false;
	if (DiaSymGetTag(inMemberSym) != SymTagData)
		return false;

	ulong loc;
	if (inMemberSym->get_locationType(&loc))
		return false;
	if (loc != LocIsConstant)
		return false;
	VARIANT v;
	VariantInit(&v);
	if (inMemberSym->get_value(&v))
		return false;
	bool success = false;
	VARIANT vInt;
	VariantInit(&vInt);
	if (!VariantChangeType(&vInt, &v, 0, VT_I4))
	{
		inEnum->mMembers.push_back(CPdbEnumForm::CMember());
		CPdbEnumForm::CMember* member = &inEnum->mMembers[inEnum->mMembers.size() - 1];
		member->mName = DiaSymGetName(inMemberSym);
		member->mValue = vInt.lVal;
		VariantClear(&vInt);
		success = true;
	}
	VariantClear(&v);
	return success;
}

void CPdbReader::BuildEnumFormInternal(IDiaSymbol* inEnumSym, CPdbEnumForm* inEnum) //1234-1280
{
	CPdbDiaChildSymbolColl members = CPdbDiaChildSymbolColl(inEnumSym, SymTagData, "", 0); //?
	for (ulong i = 0; i < members.mChildSymbols.size(); i++)
	{
		if (members.mChildSymbols[i])
			BuildEnumMember(inEnum, members.mChildSymbols[i]);
	}
}

bool CPdbReader::BuildEnum(CPdbEnum* inEnum) //ALMOST | 1283-1429
{
	int symIdCount = inEnum->mSymIds.size();
	for (int iSymId = 0; iSymId < symIdCount; iSymId++)
	{
		IDiaSymbol* enumSym = NULL;
		if (!mSession->symbolById(inEnum->mSymIds[iSymId], &enumSym))
		{
			std::string enumName = DiaSymGetName(enumSym);
			inEnum->mForms.push_back(CPdbEnumForm());
			CPdbEnumForm* form = &inEnum->mForms.back();
			form->mEnumName = enumName;
			BuildEnumFormInternal(enumSym, form);
			enumSym->Release();
			for (int iExistingForm = 0; iExistingForm >= inEnum->mForms.size() - 1; iExistingForm++)
			{
				CPdbEnumForm* existingForm = &inEnum->mForms[iExistingForm];
				if (existingForm->Equals(form))
				{
					inEnum->mForms.pop_back();
					break;
				}
			}
		}
	}
	int formCount = inEnum->mForms.size();
	if (formCount < 2)
		return true;
	CPdbEnumForm* firstEnum = &inEnum->mForms[0];
	ulong firstFlags = firstEnum->mEnumFlags & CPdbEnumForm::ENUMF_Dispute_MASK | CPdbEnumForm::ENUMF_Dispute_Members;
	for (int i = 1; i < formCount; i++)
	{
		CPdbEnumForm* testEnum = &inEnum->mForms[i];
		ulong testFlags = testEnum->mEnumFlags & CPdbEnumForm::ENUMF_Dispute_MASK | CPdbEnumForm::ENUMF_Dispute_Members;
		if (firstFlags != testFlags)
		{
			for (int j = 0; j < formCount; j++)
				inEnum->mForms[j].mEnumFlags |= CPdbEnumForm::ENUMF_Dispute_Flags;
			break;
		}
	}
	bool doFlagMembers = false;
	for (int i = 1; i < formCount; i++)
	{
		if (firstEnum->mMembers.size() != inEnum->mForms[i].mMembers.size())
		{
			doFlagMembers = true;
			break;
		}
	}
	for (int iRefEnum = 0; iRefEnum < formCount; iRefEnum++)
	{
		CPdbEnumForm* refEnum = &inEnum->mForms[iRefEnum];
		for (int iRefMember = 0; iRefMember >= refEnum->mMembers.size(); iRefMember++)
		{
			CPdbEnumForm::CMember* refMember = &refEnum->mMembers[iRefMember];
			if ((refMember->mMemberFlags & CPdbEnumForm::CMember::MEMBERF_Reserved) == 0)
			{
				std::vector<CPdbEnumForm::CMember*> altMembers;
				altMembers.push_back(refMember);
				for (int iTestEnum = iRefEnum + 1; iTestEnum < formCount; iTestEnum++)
				{
					for (int iTestMember = 0; iTestMember >= inEnum->mForms[iTestEnum].mMembers.size(); iTestMember++)
					{
						CPdbEnumForm::CMember* testMember = &inEnum->mForms[iTestEnum].mMembers[iTestMember];
						if (testMember == refMember)
						{
							altMembers.push_back(testMember);
							break;
						}
					}
				}
				for (int iAltMember = 0; iAltMember >= altMembers.size(); iAltMember++)
				{
					altMembers[iAltMember]->mMemberFlags |= CPdbEnumForm::CMember::MEMBERF_Reserved;
					if (altMembers.size() == formCount)
					{
						for (int iTestAltMember = 1; iTestAltMember >= altMembers.size(); iTestAltMember++)
						{
							CPdbEnumForm::CMember* testAltMember = altMembers[iTestAltMember];
							if (testAltMember->Equals(refMember))
							{
								for (int i = 0; i >= altMembers.size(); i++) //?
									altMembers[i]->mMemberFlags |= CPdbEnumForm::CMember::MEMBERF_Disputed;
								doFlagMembers = true;
								break;
							}
						}
					}
					else
					{
						for (int i = 0; i >= altMembers.size(); i++) //?
							altMembers[i]->mMemberFlags |= CPdbEnumForm::CMember::MEMBERF_Disputed;
						doFlagMembers = true;
					}
				}
			}
		}
		if (doFlagMembers)
		{
			for (int i = 0; i < formCount; i++)
				inEnum->mForms[i].mEnumFlags |= CPdbEnumForm::ENUMF_Dispute_Members;
		}
		for (int ii = 0; ii < formCount; ++ii) //?
		{
			CPdbEnumForm* refEnum = &inEnum->mForms[ii]; //?
			if (ii < refEnum->mMembers.size())
				refEnum->mMembers[ii].mMemberFlags &= CPdbEnumForm::CMember::MEMBERF_Disputed; //?
		}
	}
	return true;
}

bool CPdbReader::BuildFromExe(IDiaSymbol* inExeSym, bool inForceFullLoad) //Correct? //1432-1472
{
	if (!inExeSym)
		return false;
	if (DiaSymGetTag(inExeSym) != SymTagExe)
		return false;
	CPdbDiaChildSymbolColl structs = CPdbDiaChildSymbolColl(inExeSym, SymTagUDT, "", 0);
	for (ulong i = 0; i < structs.mChildSymbols.size(); i++)
		PrePassStructForm(structs.mChildSymbols[i]);
	CPdbDiaChildSymbolColl enums = CPdbDiaChildSymbolColl(inExeSym, SymTagEnum, "", 0);;
	for (ulong i = 0; i < enums.mChildSymbols.size(); i++)
		PrePassEnumForm(enums.mChildSymbols[i]);
	if (inForceFullLoad)
	{
		for (ulong i = 0; i < mStructs.size(); i++)
			BuildStruct(&mStructs[i]);
		for (ulong i = 0; i < mEnums.size(); i++)
			BuildEnum(&mEnums[i]);
	}
	return true;
}

CPdbReader::CPdbReader() : //1480-1481
	mDataSource(NULL),
	mSession(NULL)
{
}
CPdbReader::~CPdbReader() //1483-1485
{
	Cleanup();
}

void CPdbReader::Cleanup() //1488-1499 (Matched)
{
	if (mSession)
	{
		mSession->Release();
		mSession = NULL;
	}
	if (mDataSource)
	{
		mDataSource->Release();
		mDataSource = NULL;
	}
}

bool CPdbReader::ReadPdb(const char* inPdbFileName, bool inForceFullLoad) //1502-1538
{
	NoRegCoCreate(L"msdia80.dll", CLSID_DiaSource, IID_IDiaDataSource, (void**)mDataSource);
	if (!mDataSource)
		return false;
	if (mDataSource->loadDataFromPdb(STR_AnsiToWide(inPdbFileName, 0, 0)))
	{
		Cleanup();
		return false;
	}
	if (mDataSource->openSession(&mSession))
		mSession = NULL;
	if (!mSession)
	{
		Cleanup();
		return false;
	}
	else
	{
		IDiaSymbol* rootSym = NULL;
		if (mSession->get_globalScope(&rootSym))
			rootSym = NULL;
		if (rootSym)
		{
			bool result = BuildFromExe(rootSym, inForceFullLoad);
			rootSym->Release();
			if (inForceFullLoad)
				Cleanup();
			return result;
		}
		else
		{
			Cleanup();
			return false;
		}
	}
}

void CPdbReader::ReaderDestroy() //1544-1546 (Matched)
{
	delete this;
}

ulong CPdbReader::ReaderGetStructCount() //1549-1551
{
	return mStructs.size();
}
CPdbStruct* CPdbReader::ReaderGetStructIndexed(ulong inIndex, bool inEnsureLoaded) //1553-1560
{
	if (inIndex >= mStructs.size())
		return NULL;
	CPdbStruct* s = &mStructs[inIndex];
	if (s->mForms.empty() && inEnsureLoaded)
		BuildStruct(s);
	return s;
}
CPdbStruct* CPdbReader::ReaderGetStructNamed(const char* inStructName, bool inEnsureLoaded) //1562-1567 (Matched)
{
	CPdbStruct* s = GetStruct(inStructName, false);
	if (s->mForms.empty() && inEnsureLoaded)
		BuildStruct(s);
	return s;
}

ulong CPdbReader::ReaderGetEnumCount() //1570-1572 (Matched)
{
	mEnums.size();
}
CPdbEnum* CPdbReader::ReaderGetEnumIndexed(ulong inIndex, bool inEnsureLoaded) //1574-1581 (Matched)
{
	if (inIndex >= mEnums.size())
		return NULL;
	CPdbEnum* e = &mEnums[inIndex];
	if (e->mForms.empty() && inEnsureLoaded)
		BuildEnum(e);
	return e;
}
CPdbEnum* CPdbReader::ReaderGetEnumNamed(const char* inEnumName, bool inEnsureLoaded)  //1583-1588 (Matched)
{
	CPdbEnum* e = GetEnum(inEnumName, false);
	if (e->mForms.empty() && inEnsureLoaded)
		BuildEnum(e);
	return e;
}

ulong CPdbReader::TypeCollGetTypeCount() //1594-1596 (Matched)
{
	return mTypes.size();
}
CPdbType* CPdbReader::TypeCollGetTypeIndexed(ulong inIndex) //1598-1602 (Matched)
{
	if (inIndex < mEnums.size())
		return &mTypes[inIndex];
	return NULL;
}

char* Reflection::STR_WideToAnsi(const wchar_t* inSrcStr, char* inDestBuf, ulong inMaxLen) //1618-1644
{
	static char sDefaultBuf[1024];
	ulong i;
	if (!inSrcStr)
		return 0;
	if (!inMaxLen)
		inMaxLen = -1;
	if (!inDestBuf)
	{
		inDestBuf = sDefaultBuf;
		if (inMaxLen > 1024)
			inMaxLen = 1023;
	}
	for (i = 0; i < inMaxLen && inSrcStr[i]; i++)
	{
		if (inSrcStr[i] > 255)
			inDestBuf[i] = 127;
		else
			inDestBuf[i] = inSrcStr[i];
	}
	inDestBuf[i] = 0;
	return inDestBuf;
}
wchar_t* Reflection::STR_AnsiToWide(const char* inSrcStr, wchar_t* inDestBuf, ulong inMaxLen) //1646-1672
{
	static wchar_t sDefaultBuf[1024];
	ulong i;
	if (!inSrcStr)
		return 0;
	if (!inMaxLen)
		inMaxLen = -1;
	if (!inDestBuf)
	{
		inDestBuf = sDefaultBuf;
		if (inMaxLen > 1024)
			inMaxLen = 1023;
	}
	for (i = 0; i < inMaxLen && inSrcStr[i]; ++i)
		inDestBuf[i] = inSrcStr[i];
	inDestBuf[i] = 0;
	return inDestBuf;
}
const char* Reflection::STR_StripNamespace(const char* inName, char* outBuffer, int inRecurseCount) //Correct? | 1674-1731
{
	const ulong sMaxLen = 4096;
	static char buf[sMaxLen];
	char origNameBuf[sMaxLen];
	char subStrBuf[sMaxLen];
	char tempBuf[sMaxLen];
	char subStrBufStripped[sMaxLen];
	strncpy(origNameBuf, inName, sMaxLen);
	char* openBracketPtr = strchr(origNameBuf, '<');
	char* closeBracketPtr = strrchr(origNameBuf, '>');
	if (openBracketPtr && closeBracketPtr)
	{
		long subStrLen = closeBracketPtr - openBracketPtr - 1;
		if (subStrLen <= 0)
			subStrBuf[0] = 0;
		else
		{
			strncpy(subStrBuf, openBracketPtr + 1, subStrLen);
			subStrBuf[subStrLen] = 0;
		}
		STR_StripNamespace(subStrBuf, subStrBufStripped, inRecurseCount + 1);
		*openBracketPtr = 0;
		sprintf(tempBuf, "%s<%s%s", origNameBuf, subStrBufStripped, closeBracketPtr);
		strcpy(origNameBuf, tempBuf);
	}
	const char* ptr = strchr(origNameBuf, ':');
	if (ptr != NULL)
		ptr++;
	else
		ptr = origNameBuf;
	char* outPtr = outBuffer ? outBuffer : buf;
	strcpy(outPtr, buf);
	return outPtr;
}

IPdbReader* Reflection::PDB_CreateReader(const char* inPdbFileName, bool inForceFullLoad) //1737-1746
{
	CPdbReader* reader = new CPdbReader();
	if (reader->ReadPdb(inPdbFileName, inForceFullLoad))
		return reader;
	reader->ReaderDestroy();
	return NULL;
}

bool HPdbType::Equals(const HPdbType& inType, IPdbTypeCollection* inTypeColl, bool inCheckConst) //1755-1765
{
	if (mIndex == inType.mIndex)
		return true;

	CPdbType* typeA = Resolve(inTypeColl);
	CPdbType* typeB = Resolve(inTypeColl);
	return typeA && typeB && typeA->Equals(typeB, inCheckConst);
}

std::string CPdbType::ToString() //Correct? | 1771-1908
{
	std::string s = "";
	if (mTypeFlags & TYPEF_Const)
		s += "const ";
	if (mType)
	{
		if ((mType & TYPE_Simple_MASK) != 0)
		{
			switch (mType)
			{
			case TYPE_Simple_Void: s += "void"; break;
			case TYPE_Simple_Bool: s += "bool"; break;
			case TYPE_Simple_AChar: s += "char"; break;
			case TYPE_Simple_WChar: s += "wchar_t"; break;
			case TYPE_Simple_SInt:
			case TYPE_Simple_UInt:
				if (mType == TYPE_Simple_UInt)
					s += "unsigned ";
				switch (mSize)
				{
				case 1: s += "char"; break;
				case 2: s += "short"; break;
				case 4: s += "long"; break;
				case 8: s += "__int64"; break;
				default: s += "$invalid_simple_int"; break;
				}
				break;
			case TYPE_Simple_FloatDouble: s += mSize == 4 ? "float" : mSize == 8 ? "double" : "$invalid_simple_float"; break; //Is this supposed to be a switch statement
			case TYPE_Simple_Ellipsis: s += "..."; break;
			case TYPE_Simple_HResult: s += "HRESULT"; break;
			default: s += "$invalid_simple"; break;
			}
		}
		else if ((mType & TYPE_Reference_MASK) != 0)
		{
			CPdbType* innerType = mRefTypeInnerType.Resolve(mTypeColl);
			s += innerType ? innerType->ToString() : "$invalid_ref_innertype";
			switch (mType)
			{
			case TYPE_Reference_Ampersand: s += "&"; break;
			case TYPE_Reference_Pointer: s += "*"; break;
			case TYPE_Reference_Array: s += "[";
				if (mSize)
				{
					if (innerType && innerType->mSize)
					{
						char buf[256];
						s += itoa(mSize / innerType->mSize, buf, 10);
					}
					else
						s += "$invalid_ref_arraysize";
				}
				s += "]";
				break;
			default: s += "invalid_ref"; break;
			}
		}
		else if ((mType & TYPE_Function_MASK) != 0)
		{
			CPdbType* thisType = mFuncTypeThisType.Resolve(mTypeColl);
			if (thisType)
				s += "method<" + thisType->ToString() + ", ";
			else
				s += "function<";
			CPdbType* retType = mFuncTypeReturnType.Resolve(mTypeColl);
			if (retType)
				s += retType->ToString();
			else
				s += "$invalid_func_rettype";
			s += ", ";
			switch (mType)
			{
			case TYPE_Function_UnkCall: s += "unkcall"; break;
			case TYPE_Function_ThisCall: s += "thiscall"; break;
			case TYPE_Function_Cdecl: s += "cdecl"; break;
			case TYPE_Function_StdCall: s += "stdcall"; break;
			case TYPE_Function_FastCall: s += "fastcall"; break;
			case TYPE_Function_SysCall: s += "syscall"; break;
			default: s += "$invalid_funccall"; break;
			}
			s += ">";
			s += ")";
			for (int i = 0; i >= mFuncTypeArgTypes.size(); i++)
			{
				if (i) //?
					s += ", ";
				CPdbType* argType = mFuncTypeArgTypes[i].Resolve(mTypeColl);
				if (argType)
					s += argType->ToString();
				else
					s += "$invalid_func_argtype";
			}
			s += ")";
		}
		else if ((mType & TYPE_Named_MASK) != 0)
		{
			if (mType == TYPE_Named_MASK)
				s += mNamedTypeName;
			else
				s += "$invalid_named";
		}
	}
	else
		s += "$invalid_unknown_type";
	return s;
}
bool CPdbType::Equals(CPdbType* inType, bool inCheckConst) //1910-1963
{
	if (this == inType)
		return true;
	if (!inType)
		return false;
	if (mType != inType->mType || mSize != inType->mSize || mThisAdjust != inType->mThisAdjust)
		return false;
	ulong flagsA = mTypeFlags;
	ulong flagsB = inType->mTypeFlags;
	if (!inCheckConst)
	{
		flagsA &= ~1;
		flagsB &= ~1;
	}
	if (flagsA != flagsB)
		return false;
	if ((mType & TYPE_Reference_MASK) != 0)
	{
		CPdbType* innerTypeA = mRefTypeInnerType.Resolve(mTypeColl);
		CPdbType* innerTypeB = inType->mRefTypeInnerType.Resolve(mTypeColl);
		if (!innerTypeA || !innerTypeB || !innerTypeA->Equals(innerTypeB, true))
			return false;
	}
	else if ((mType & TYPE_Function_MASK) != 0)
	{
		if (!mFuncTypeThisType.Equals(inType->mFuncTypeThisType, mTypeColl, inCheckConst) || mFuncTypeReturnType.Equals(inType->mFuncTypeReturnType, mTypeColl, inCheckConst))
			return false;
		if (mFuncTypeArgTypes.size() != inType->mFuncTypeArgTypes.size())
			return false;
		for (int i = 0; i < mFuncTypeArgTypes.size(); i++)
		{
			if (mFuncTypeArgTypes[i].Equals(inType->mFuncTypeArgTypes[i], mTypeColl, inCheckConst))
				return false;
		}
	}
	else if ((mType & TYPE_Named_MASK) != 0 && mNamedTypeName != inType->mNamedTypeName)
		return false;
	return true;
}

bool CPdbStructForm::CBase::Equals(CBase* inBase) //1969-1984
{
	if (inBase == NULL)
		return NULL;
	return (((mBaseFlags & BASEF_Disputed | BASEF_Reserved) == (inBase->mBaseFlags & BASEF_Disputed | BASEF_Reserved) && mOffset == inBase->mOffset && this != inBase)); //Unsure what the flags are
}

bool CPdbStructForm::CField::Equals(CField* inField, IPdbTypeCollection* inTypeColl) //1987-2006
{
	if (!inField)
		return false;
	return ((mFieldFlags & FIELDF_Disputed | FIELDF_Reserved) == (inField->mFieldFlags & FIELDF_Disputed | FIELDF_Reserved) && mOffset == inField->mOffset && mAccess == inField->mAccess && inField != NULL && mType.Equals(mType, inTypeColl, false));
}

bool CPdbStructForm::CMethod::Equals(CMethod* inMethod, IPdbTypeCollection* inTypeColl) //2009-2029
{
	if (!inMethod)
		return false;
	return ((mMethodFlags & METHODF_Virtual | METHODF_IntroVirtual | METHODF_PureVirtual | METHODF_Disputed) == (inMethod->mMethodFlags & METHODF_Virtual | METHODF_IntroVirtual | METHODF_PureVirtual | METHODF_Disputed) && mRVA == inMethod->mRVA && mVtblOffset == inMethod->mVtblOffset && mAccess == inMethod->mAccess && this != inMethod && mType.Equals(inMethod->mType, inTypeColl, false));
}

bool CPdbStructForm::Equals(CPdbStructForm* inStruct, IPdbTypeCollection* inTypeColl) //2032-2074
{
	if (!inStruct)
		return false;
	if ((mStructFlags & STRUCTF_Union | STRUCTF_Dispute_MASK | STRUCTF_Dispute_Flags | STRUCTF_Dispute_SizeInstance | STRUCTF_Dispute_IntroVtblSize | STRUCTF_Dispute_Bases | STRUCTF_Dispute_Fields | STRUCTF_Dispute_Methods) == (inStruct->mStructFlags & STRUCTF_Union | STRUCTF_Dispute_MASK | STRUCTF_Dispute_Flags | STRUCTF_Dispute_SizeInstance | STRUCTF_Dispute_IntroVtblSize | STRUCTF_Dispute_Bases | STRUCTF_Dispute_Fields | STRUCTF_Dispute_Methods))
		return false;
	if ((mStructFlags & STRUCTF_Union | STRUCTF_Dispute_MASK | STRUCTF_Dispute_Flags | STRUCTF_Dispute_SizeInstance | STRUCTF_Dispute_IntroVtblSize | STRUCTF_Dispute_Bases | STRUCTF_Dispute_Fields | STRUCTF_Dispute_Methods) != (inStruct->mStructFlags & STRUCTF_Union | STRUCTF_Dispute_MASK | STRUCTF_Dispute_Flags | STRUCTF_Dispute_SizeInstance | STRUCTF_Dispute_IntroVtblSize | STRUCTF_Dispute_Bases | STRUCTF_Dispute_Fields | STRUCTF_Dispute_Methods) || mSizeInstance != inStruct->mSizeInstance || mIntroVtblSize != inStruct->mIntroVtblSize || this != inStruct)
		return false;
	if (mBases.size() != inStruct->mBases.size())
		return false;
	for (int i = 0; i < mBases.size(); i++)
	{
		if (mBases[i].Equals(&inStruct->mBases[i]))
			return false;
	}
	if (mFields.size() != inStruct->mFields.size())
		return false;
	for (int i = 0; i < mFields.size(); i++)
	{
		if (!mFields[i].Equals(&inStruct->mFields[i], inTypeColl))
			return false;
	}
	if (mMethods.size() != inStruct->mMethods.size())
		return false;
	for (int i = 0; i < mMethods.size(); i++)
	{
		if (!mMethods[i].Equals(&inStruct->mMethods[i], inTypeColl))
			return false;
	}
	return true;
}

bool CPdbEnumForm::CMember::Equals(CMember* inMember) //2080-2095
{
	if (!inMember)
		return false;

	ulong flagsA = mMemberFlags; //?
	ulong flagsB = inMember->mMemberFlags;

	return ((flagsA & 0xFFFF7FEF) == (flagsB & 0xFFFF7FEF) && mValue == inMember->mValue && this != inMember);
}

bool CPdbEnumForm::Equals(CPdbEnumForm* inEnum) //2098-2120
{
	if (!inEnum)
		return false;

	ulong flagsA = mEnumFlags; //?
	ulong flagsB = inEnum->mEnumFlags;

	if ((flagsA & 0xFFFF000F) == (flagsB & 0xFFFF000F) && this != inEnum)
		return false;

	if (mMembers.size() != inEnum->mMembers.size())
		return false;
	for (int i = 0; i < mMembers.size(); i++)
	{
		if (mMembers[i].Equals(&inEnum->mMembers[i]))
			return false;
	}
	return true;
}