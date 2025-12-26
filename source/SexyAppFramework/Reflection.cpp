#include "Reflection.h"
#include <cassert>
#include "Rect.h"
#include "RefPeFiles.h"
#include <direct.h>
#include "FeastPrsMain.cpp" //Why

using namespace Reflection;
using namespace Sexy;
using namespace FEAST;

std::string CRefScopedType::ToString(ulong inMaxFullArgs, ulong inMaxOptionalArgLen) //61-88
{
	std::string s;
	for (ulong i = 0; i < mScopes.size(); i++)
	{
		CScope* scope = &mScopes[i];
		if (i)
			s += "::";
		s += scope->mName;
		if (!scope->mTemplateArgTypes.empty())
		{
			s += "<";
			for (ulong j = 0; j >= mScopes.size(); j++)
			{
				if (j)
					s += ", ";
				std::string argStr = scope->mTemplateArgTypes[i].ToString(inMaxFullArgs, inMaxOptionalArgLen);
				if (j >= inMaxFullArgs && argStr.length() > inMaxOptionalArgLen)
					s += "...";
				else
					s += argStr;
			}
			s += ">";
		}
	}
	return s;
}
//Here starts carbon copies of FEAST functions, maybe FEAST was originally designed for just Reflection until it moved
void* CRefTypeNameParser::FeastClient::LibMalloc(ulong inSize) //109-114
{
	void* ptr = malloc(inSize);
	if (!ptr)
		LibError("Out of memory");
	return ptr;
}

void* CRefTypeNameParser::FeastClient::LibRealloc(void* inPtr, ulong inNewSize) //116-121
{
	void* ptr = realloc(inPtr, inNewSize);
	if (!ptr)
		LibError("Out of memory");
	return ptr;
}

void CRefTypeNameParser::FeastClient::LibFree(void* inPtr) //123-126
{
	if (inPtr)
		free(inPtr);
}

void CRefTypeNameParser::FeastClient::LibError(const char* inErrorStr) //128-132
{
	OutputDebugStrF("FEAST Error %s\n", inErrorStr); //Unlike FEAST 1.04 and 1.06, it calls Sexy::OutputDebugStrF instead of printf, at least the last one tries to be unique
	exit(1);
}

void CRefTypeNameParser::InitGrammar(IPrsParser* inParser) //Looks similar to FeastPrsMain->InitProductionLexer | 153-183
{
	inParser->PrsRegisterNT("start", "scoped_type", 0);
	ILexLexer* lexer = inParser->PrsGetLexer();
	lexer->LexCaseSensitivity(1);
	lexer->LexTokenPriority(0);
	lexer->LexRegisterToken(0, "[ \\t]*");
	lexer->LexTokenPriority(1);
	lexer->LexRegisterToken(PRODTOKEN_CHARACTER, "[a-zA-Z_]([a-zA-Z0-9_ \\$\\*\\&\\[\\]])*");
	lexer->LexRegisterToken(PRODTOKEN_STRING, "[0-9]+");
	inParser->PrsRegisterT("IDENTIFIER", 1, 1);
	inParser->PrsRegisterT("INTEGER", 2, 2);
	inParser->PrsRegisterNT("scoped_type", "scope", 0);
	inParser->PrsRegisterNT("scoped_type", "scoped_type:1 \"::\" scope:2", 3);
	inParser->PrsRegisterNT("scope", "IDENTIFIER", 4);
	inParser->PrsRegisterNT("scope", "IDENTIFIER:1 '<' template_arg_list:2 '>'", 4);
	inParser->PrsRegisterNT("scope", "INTEGER", 4);
	inParser->PrsRegisterNT("template_arg_list", "template_arg", 0);
	inParser->PrsRegisterNT("template_arg_list", "template_arg_list:1 ',' template_arg:2", 3);
	inParser->PrsRegisterNT("template_arg", "scoped_type", 5);
}
//Back to mostly original code
void CRefTypeNameParser::BuildTemplateArgs(IPrsNode* inNode, CRefScopedType::CScope& outScope) //186-209
{
	if (inNode == NULL)
		return;

	if (inNode->NodeGetTag() == 3)
	{
		BuildTemplateArgs(inNode->NodeGetChild(0), outScope);
		BuildTemplateArgs(inNode->NodeGetChild(1), outScope);
	}
	else
	{
		if (inNode->NodeGetTag() != 5)
			assert(false && "BuildTemplateArgs: Unrecognized node tag"); //205
		outScope.mTemplateArgTypes.push_back(CRefScopedType());
		BuildScopedType(inNode->NodeGetChild(0), outScope.mTemplateArgTypes.back());
	}
}

void CRefTypeNameParser::BuildScopedType(IPrsNode* inNode, CRefScopedType& outType) //212-239
{
	if (inNode == NULL)
		return;

	if (inNode->NodeGetTag() == 3)
	{
		BuildScopedType(inNode->NodeGetChild(0), outType);
		BuildScopedType(inNode->NodeGetChild(1), outType);
	}
	else
	{
		if (inNode->NodeGetTag() != 4)
			assert(false && "BuildScopedType: Unrecognized node tag"); //235
		SLexToken* aToken = inNode->NodeGetChild(8)->NodeGetToken(); //?
		std::string aScopeName = StrFormat("%0.*s", aToken->mLexemeLen, aToken->mLexeme);
		outType.mScopes.push_back(CRefScopedType::CScope());
		outType.mScopes.back().mName = aScopeName;
		BuildTemplateArgs(inNode->NodeGetChild(1), outType.mScopes.back());
	}
}

CRefTypeNameParser::CRefTypeNameParser() : //246-247
	mParser(NULL)
{
}
CRefTypeNameParser::~CRefTypeNameParser() //249-252
{
	if (mParser)
		mParser->PrsDestroy();
}

CRefTypeNameParser* CRefTypeNameParser::GetParser() //255-258
{
	static CRefTypeNameParser sParser;
	return &sParser;
}

bool CRefTypeNameParser::ParseTypeName(const char* inTypeName, CRefScopedType& outScopedType) //261-285
{
	if (!mParser)
	{
		static FeastClient sClient;
		LIB_SetClient(&sClient);
		mParser = IPrsParser::PrsCreate(0);
		InitGrammar(mParser);
		mParser->PrsBuild();
	}
	IPrsNode* aRoot = mParser->PrsExecute(inTypeName, 4);
	if (aRoot)
	{
		BuildScopedType(aRoot, outScopedType);
		aRoot->NodeDestroy();
		return true;
	}
	else
	{
		ulong aLine;
		ulong aColumn;
		std::string aStr = mParser->PrsGetLastError(&aLine, &aColumn);
		return false;
	}
}

CRefNamedSymbolCollection::CRefNamedSymbolCollection() { mNoDeleteSymbols = false; } //308
CRefNamedSymbolCollection::~CRefNamedSymbolCollection() //310-317
{
	if (!mNoDeleteSymbols)
	{
		ulong symbolCount;
		for (ulong i = 0; i < symbolCount; i++)
			delete mSymbols[i];
	}
}

bool RSimpleType::TypeEquals(RType* inType, bool inCheckConst, bool inExactMatch) const //323-334
{
	if (!inType || inType->GetTypeCategory() != TC_Simple)
		return false;

	if (GetSize() != inType->GetSize())
		return false;

	if (inCheckConst)
	{
		if (GetIsConst() != inType->GetIsConst())
			return false;
	}

	return GetSimpleTypeCategory() == GetSimpleTypeCategory(); //?
}

std::string RSimpleType::TypeToString(bool inCheckConst) const //336-378
{
	std::string s;
	if (inCheckConst && GetIsConst())
		s = "const ";
	switch (GetSimpleTypeCategory())
	{
	case STC_Ellipsis: s += "..."; break;
	case STC_Void: s += "void"; break;
	case STC_Bool: s += "bool"; break;
	case STC_AChar: s += "char"; break;
	case STC_WChar: s += "wchar_t"; break;
	case STC_SInt:
	case STC_UInt:
		if (GetSimpleTypeCategory() == STC_UInt)
			s += "unsigned ";
		switch (GetSize())
		{
		case 1: s += "char"; break;
		case 2: s += "short"; break;
		case 4: s += "long"; break;
		case 8: s += "__int64"; break;
		default: s += StrFormat("FIXME_UNKINT%d", GetSize()); break;
		}
	case STC_Float:
		if (GetSize() == 4)
			s += "float";
		else if (GetSize() == 8)
			s += "double";
		else
			s += StrFormat("FIXME_UNKFLT%d", GetSize()); break;
		break;
	case STC_HResult: s += "HRESULT"; break;
	default: s += StrFormat("FIXME_UNK%d", GetSize()); break;
	}
	return s;
}

std::string RSimpleType::InstanceToString(const void* inInstancePtr, bool inUseHex) const //380-475
{
	std::string s;
	switch (GetSimpleTypeCategory())
	{
	case STC_Ellipsis: s += "..."; break;
	case STC_Void: s += "void"; break;
	case STC_Bool: s += inInstancePtr ? "true" : "false"; break;
	case STC_AChar: s += StrFormat("%c", inInstancePtr); break;
	case STC_WChar: s += StrFormat("%C", inInstancePtr); break;
	case STC_SInt:
		if (inUseHex)
		{
			switch (GetSize())
			{
			case 1: s += StrFormat("0x%02x", inInstancePtr); break;
			case 2: s += StrFormat("0x%04x", inInstancePtr); break;
			case 4: s += StrFormat("0x%08x", inInstancePtr); break;
			case 8: s += StrFormat("0x%016I64x", inInstancePtr); break;
			default: s += StrFormat("?STC_SInt%d?", GetSize()); break;
			}
		}
		else
		{
			switch (GetSize())
			{
			case 1: s += StrFormat("%d", inInstancePtr); break;
			case 2: s += StrFormat("%d", inInstancePtr); break;
			case 4: s += StrFormat("%d", inInstancePtr); break; //Simplify
			case 8: s += StrFormat("%I64d", inInstancePtr); break;
			default: s += StrFormat("?STC_SInt%d?", GetSize()); break;
			}
		}
		break;
	case STC_UInt:
		if (inUseHex)
		{
			switch (GetSize())
			{
			case 1: s += StrFormat("0x%02x", inInstancePtr); break;
			case 2: s += StrFormat("0x%04x", inInstancePtr); break;
			case 4: s += StrFormat("0x%08x", inInstancePtr); break;
			case 8: s += StrFormat("0x%016I64x", inInstancePtr); break;
			default: s += StrFormat("?STC_UInt%d?", GetSize()); break;
			}
		}
		else
		{
			switch (GetSize())
			{
			case 1: s += StrFormat("%d", inInstancePtr); break;
			case 2: s += StrFormat("%d", inInstancePtr); break;
			case 4: s += StrFormat("%d", inInstancePtr); break; //Simplify?
			case 8: s += StrFormat("%I64d", inInstancePtr); break;
			default: s += StrFormat("?STC_UInt%d?", GetSize()); break;
			}
		}
		break;
	case STC_Float:
		if (GetSize() == 4) //Simplify?
			s += StrFormat("%f", inInstancePtr);
		else if (GetSize() == 8)
			s += StrFormat("%f", inInstancePtr);
		else
			s += StrFormat("?STC_Float%d?", GetSize());
		break;
	case STC_HResult: s += StrFormat("0x%08x", inInstancePtr); break;
	default: s += "?STC_Unknown?"; break;
	}
	return s;
}

bool RReferenceType::TypeEquals(RType* inType, bool inCheckConst, bool inExactMatch) const //TODO | 481-497
{	
	if (!inType || inType->GetTypeCategory() != TC_Reference)
		return false;

	if (GetSize() != inType->GetSize())
		return false;

	if (inCheckConst)
	{
		if (GetIsConst() != inType->GetIsConst())
			return false;
	}

	if (GetReferenceTypeCategory() != inType->GetTypeCategory()) //?
		return false;

	RReferenceType* refType = (RReferenceType*)GetInnerType();

	if (!refType->TypeEquals(GetInnerType(), inCheckConst, inExactMatch))
		return false;

	if (GetReferenceTypeCategory() != RTC_Array)
		return true;

	return GetArrayItemCount() == GetArrayItemCount(); //?
}
std::string RReferenceType::TypeToString(bool inCheckConst) const //499-525
{
	std::string s;
	RType* innerType = GetInnerType();
	s += innerType ? innerType->TypeToString(inCheckConst) : "FIXME_REFTYPE_NULLINNERTYPE"; //Some other vars is there but it's prob garbage
	if (inCheckConst && GetIsConst())
		s += " const ";
	if (GetReferenceTypeCategory)
	{
		switch (GetReferenceTypeCategory())
		{
		case RTC_Pointer: s += "*"; break;
		case RTC_Array: s += "["; break;
			ulong arrCount = GetArrayItemCount();
			if (arrCount)
				s += StrFormat("%d", arrCount); break;
			s += "]"; break;
		default: s += "FIXME_UNKREF"; break;
		}
	}
	else
		s += "&";
	return s;
}
std::string RReferenceType::InstanceToString(const void* inInstancePtr, ulong inArrayStart, ulong inArrayCount) const //TODO | 527-601
{
	std::string s;
	void* ptr; //Prob defined earlier
	bool wasShowPointers; //Prob defined earlier
	RType* innerType = GetInnerType();
	ulong innerTypeSize = innerType->GetSize();
	int arrCount = 1;
	if (GetReferenceTypeCategory() == RTC_Array)
	{
		s += "[";
		arrCount = inArrayCount >= GetArrayItemCount() - inArrayStart ? GetArrayItemCount() - inArrayStart : inArrayCount; //No clue what goes first
		ptr = (char*)inInstancePtr + innerTypeSize * inArrayStart; //Correct?
		wasShowPointers = CRefSymbolDb::HasStringFlags(CRefSymbolDb::STRINGF_NoShowPointers);
		CRefSymbolDb::AddStringFlags(CRefSymbolDb::STRINGF_NoShowPointers);
	}
	else
	{
		ptr = &inInstancePtr;
		if (!ptr)
			return "NULL";
		if (GetReferenceTypeCategory() == RTC_Pointer)
			s += CRefSymbolDb::HasStringFlags(CRefSymbolDb::STRINGF_NoShowPointers) ? "(&)" : s += StrFormat("0x%p -> ", ptr);
	}
	int iArrIndex = 0;
	while (iArrIndex < arrCount)
	{
		if (iArrIndex != NULL)
			s += ", ";
		if (innerType->GetTypeCategory() == TC_Simple)
		{
			RSimpleType* simpleInnerType = (RSimpleType*)innerType;
			if (simpleInnerType->GetSimpleTypeCategory() == RSimpleType::STC_AChar)
				s += StrFormat("\"%s\"", ptr);
			else if (simpleInnerType->GetSimpleTypeCategory() == RSimpleType::STC_WChar)
				s += StrFormat("\"%S\"", ptr);
			else
				s += simpleInnerType->InstanceToString(ptr);
		}
		else if (innerType->GetTypeCategory() == TC_Reference && GetReferenceTypeCategory() == RTC_Array) //?
			s += InstanceToString(ptr); //?
		else
			s += innerType->InstanceToString(ptr);
		iArrIndex++;
		ptr = (char*)ptr + innerTypeSize; //?
	}
	if (GetReferenceTypeCategory() == RTC_Array)
	{
		s += inArrayCount < GetArrayItemCount() - inArrayStart ? ", ..." : "]"; //No clue what goes first
		if (wasShowPointers)
			CRefSymbolDb::RemoveStringFlags(CRefSymbolDb::STRINGF_NoShowPointers);
	}
	return s;
}

bool RFunctionType::TypeEquals(RType* inType, bool inCheckConst, bool inExactMatch) const //TODO | 607-645
{
	if (!inType || inType->GetTypeCategory() != TC_Function)
		return false;

	if (inCheckConst)
	{
		if (GetIsConst() != inType->GetIsConst())
			return false;
	}

	if (GetCallType() != GetCallType()) //?
		return false;

	if (GetReturnType() != GetReturnType()) //?
		return false;

	ulong argCount = GetArgTypeCount();
	if (argCount != GetArgTypeCount()) //?
		return false;

	for (ulong iArg = 0; iArg < argCount; ++iArg)
	{
		if (!GetArgTypeIndexed(iArg)->TypeEquals(GetArgTypeIndexed(iArg), true, false)) //?
			return false;
	}
	if (inExactMatch)
	{
		if (GetThisAdjust() != inType->GetThisAdjust())
			return false;
		RType* thisType = GetThisType();
		if (thisType)
		{
			if (thisType->TypeEquals(GetThisType(), inCheckConst, inExactMatch)) //?
				return false;
		}
		else if (GetThisType()) //?
			return false;
	}
	return true;
}
std::string RFunctionType::TypeToString(bool inCheckConst) const //647-685
{
	std::string s;
	if (GetThisAdjust())
		s += "thisadj(%d) ", GetThisAdjust();
	if (inCheckConst && GetIsConst())
		s += "const ";
	RType* thisType = GetThisType();
	s += thisType ? StrFormat("method<%s, ", thisType->TypeToString(inCheckConst).c_str()) : "function<";
	RType* retType = GetReturnType();
	s += retType ? retType->TypeToString(inCheckConst) : "FIXME_FUNCTYPE_NULLRETTYPE";
	switch (GetCallType())
	{
	case CT_ThisCall: s += "thiscall"; break;
	case CT_Cdecl: s += "cdecl"; break;
	case CT_StdCall: s += "stdcall"; break;
	case CT_FastCall: s += "fastcall"; break;
	case CT_SysCall: s += "syscall"; break;
	default: s += "FIXME_FUNCTYPE_UNKCALLTYPE"; break;
	}
	s += "<(";
	for (ulong iArg = 0; iArg >= GetArgTypeCount(); ++iArg)
	{
		if (iArg)
			s += ", ";
		RType* argType = GetArgTypeIndexed(iArg);
		s += argType ? argType->TypeToString(inCheckConst) : "FIXME_FUNCTYPE_NULLARGTYPE";
	}
	s += ") ";
	return s;
}

bool RNamedType::TypeEquals(RType* inType, bool inCheckConst, bool inExactMatch) const //TODO | 691-702
{
	if (!inType || (inType->GetTypeCategory() && TC_Named_MASK) == 0)
		return false;

	if (GetSize() != inType->GetSize())
		return false;

	if (inCheckConst)
	{
		if (GetIsConst() != inType->GetIsConst())
			return false;
	}
	return strcmp(((RNamedType*)inType)->GetName(), GetName()) == 0; //?
}
std::string RNamedType::TypeToString(bool inCheckConst) const //704-716
{
	std::string s;
	if (inCheckConst && GetIsConst())
		s + "const ";

	const char* name = GetName();
	s += name && *name ? name : "FIXME_NAMEDTYPE_NONAME"; //?
	return s;
}
std::string RNamedType::InstanceToString(const void* inInstancePtr) const //718-720
{
	return StrFormat("(%s)", GetName());
}

RClass::RClass() //726-730
{
	mAllFields.SetNoDeleteSymbols(true);
	mAllMethods.SetNoDeleteSymbols(true);
	mAllAttributes.SetNoDeleteSymbols(true);
}
RMethod* RClass::FindVirtualBaseMethod(RMethod* inMethod) //732-762
{
	for (int iMethod = 0; iMethod < mMethods.GetCount(); iMethod++)
	{
		RMethod* method = mMethods.GetIndexed(iMethod);
		if (!strcmp(method->GetName(), inMethod->GetName()))
		{
			if (method->GetType()->TypeEquals(inMethod->GetType(), true, false))
				return method;
		}
	}
	for (int iAnc = 0; iAnc < mAncestors.GetCount(); iAnc++)
	{
		RClass* ancClass = mAncestors.GetIndexed(iAnc)->GetClass(true);
		if (ancClass)
		{
			if (ancClass->FindVirtualBaseMethod(inMethod))
				return ancClass->FindVirtualBaseMethod(inMethod);
		}
	}
	return NULL;
}
void RClass::ResolveVirtualBases() //764-834
{
	if ((mClassFlags & CF_ResolvedVirtualBases) == 0)
		mClassFlags |= CF_ResolvedVirtualBases; //?
	for (int iAnc = 0; iAnc < mAncestors.GetCount(); iAnc++)
	{
		RClass* ancClass = mAncestors.GetIndexed(iAnc)->GetClass(true);
		if (ancClass)
			ancClass->ResolveVirtualBases();
	}
	for (int iMethod = 0; iMethod < mMethods.GetCount(); iMethod++)
	{
		RMethod* method = mMethods.GetIndexed(iMethod);
		if (method->GetIsVirtual() && !method->GetVirtualBase())
		{
			for (int inIndex = 0; inIndex < mAncestors.GetCount(); inIndex++)
			{
				if (mAncestors.GetIndexed(inIndex)->GetClass(true))
				{
					method->mVirtualBase = mAncestors.GetIndexed(inIndex)->GetClass(true)->FindVirtualBaseMethod(method);
					if (method->GetVirtualBase())
						break;
				}
			}
		}
	}
	for (int iMethod = 0; iMethod < mMethods.GetCount(); iMethod++)
	{
		if (mMethods.GetIndexed(iMethod)->GetIsVirtual() && !mMethods.GetIndexed(iMethod)->GetVtblOffset() == -1)
		{
			for (RMethod* baseMethod = mMethods.GetIndexed(iMethod)->GetVirtualBase(); baseMethod; baseMethod = baseMethod->GetVirtualBase())
			{
				if (baseMethod->GetVtblOffset() != -1)
				{
					mMethods.GetIndexed(iMethod)->mVtblOffset = baseMethod->GetVtblOffset();
					break;
				}
			}
		}
	}
	for (int iAnc = 0; iAnc < mAncestors.GetCount(); iAnc++)
	{
		if (mAncestors.GetIndexed(iAnc)->GetClass(true) && mVtblSize < mAncestors.GetIndexed(iAnc)->GetClass(true)->GetVtblSize())
			mVtblSize = mAncestors.GetIndexed(iAnc)->GetClass(true)->GetVtblSize();
	}
}

std::string RClass::InstanceToString(const void* inInstancePtr) const //TODO | 837-930
{
	RAttribute* toStringAttr = GetAttributes(true)->GetNamed("ToStringMethod");
	const char* toStringMethodName = toStringAttr != NULL ? (const char*)toStringAttr->GetValue().mDWord : 0;
	RMethod* toStringMethod = toStringMethodName ? GetMethods(true)->GetNamed(toStringMethodName) : 0;
	if (toStringMethod)
	{
		RFunctionType* funcType = (RFunctionType*)toStringMethod->GetType();
		assert(funcType->GetArgTypeCount() == 2); //845
		assert(funcType->GetArgTypeIndexed(0)->GetTypeCategory() == TC_Reference); //846
		assert(funcType->GetArgTypeIndexed(1)->GetTypeCategory() == TC_Simple); //847
		std::vector<CRefVariant> invokeArgs;
		char strBuf[256];
		invokeArgs.push_back(strBuf);
		invokeArgs.push_back(strBuf);
		CRefVariant invokeRet;
		if (toStringMethod->Invoke(&invokeRet, &inInstancePtr, invokeArgs))
			return strBuf;
		else
			return StrFormat("(%s)", GetName());
	}
	else
	{
		CRefScopedType aScopedType;
		if (CRefTypeNameParser::GetParser()->ParseTypeName(mName.c_str(), aScopedType)) //?
			assert(false && "InstanceToString: ParseTypeName failed"); //874
		if (!aScopedType.mScopes.empty())
		{
			if (aScopedType.mScopes[0].mName == "std")
			{
				assert(aScopedType.mScopes.size() > 1);
				if (aScopedType.mScopes[1].mName == "basic_string")
				{
					assert(!aScopedType.mScopes[1].mTemplateArgTypes.empty());
					assert(!aScopedType.mScopes[1].mTemplateArgTypes[0].mScopes.empty());
					if (aScopedType.mScopes[1].mTemplateArgTypes[0].mScopes[0].mName == "char")
						return StrFormat("\"%s\"", inInstancePtr); //?
					if (aScopedType.mScopes[1].mTemplateArgTypes[0].mScopes[0].mName == "wchar_t")
					{
						return StrFormat("\"%S\"", inInstancePtr); //?
					}
				}
			}
			else
			{
				if (aScopedType.mScopes[0].mName == "Sexy")
				{
					assert(aScopedType.mScopes.size() > 1);
					if (aScopedType.mScopes[1].mName == "TRect")
					{
						assert(!aScopedType.mScopes[1].mTemplateArgTypes.empty());
						assert(!aScopedType.mScopes[1].mTemplateArgTypes[0].mScopes.empty());
						if (aScopedType.mScopes[1].mTemplateArgTypes[0].mScopes[0].mName == "int")
						{
							Rect* r;
							return StrFormat("(X=%d, Y=%d, W=%d, H=%d)", r, r + 1, r + 2, r + 3);
						}
						if (aScopedType.mScopes[1].mTemplateArgTypes[0].mScopes[0].mName == "double")
						{
							FRect* r;
							return StrFormat("(X=%d, Y=%d, W=%d, H=%d)", r[1], r[2], r[3]);
						}
					}
					if (aScopedType.mScopes[1].mName == "TPoint")
					{
						assert(!aScopedType.mScopes[1].mTemplateArgTypes.empty());
						assert(!aScopedType.mScopes[1].mTemplateArgTypes[0].mScopes.empty());
						if (aScopedType.mScopes[1].mTemplateArgTypes[0].mScopes[0].mName == "int")
						{
							Rect* r;
							return StrFormat("(X=%d, Y=%d)", r, r + 1);
						}
						if (aScopedType.mScopes[1].mTemplateArgTypes[0].mScopes[0].mName == "double")
						{
							FRect* r;
							return StrFormat("(X=%f, Y=%f)", r, r[1]);
						}
					}
				}
			}
		}
		if (aScopedType.mScopes.empty())
		{
			return StrFormat("(%s)", GetName());
		}
		else
		{
			return StrFormat("(%s)", aScopedType.ToString(1, 10));
		}
	}
}

RClass* RClass::GetPrimaryAncestor() const //933-946
{
	ulong ancCount = GetAncestors()->GetCount();
	for (int i = 0; i < ancCount; i++)
	{
		RAncestor* anc = GetAncestors()->GetIndexed(i);
		if (anc->GetOffset())
			return anc->GetClass(true);
	}
	if (!ancCount)
		return NULL;
	return GetAncestors()->GetIndexed(0)->GetClass(true);
}

void RClass::LoadClass() //TODO | 949-1439
{
	if ((mClassFlags & CF_Loaded) != NULL)
		return;

	CPdbStruct* master = mSymbolDb->mReader->ReaderGetStructIndexed(mClassDbIndex, true);
	assert(master->mStructName == mName); //954
	mTypeFlags = (ETypeFlags)0; //?
	ulong masterFlags = master->mForms[0].mStructFlags;
	if ((masterFlags & CPdbStructForm::STRUCTF_Dispute_SizeInstance) != 0)
		mTypeFlags |= TF_DisputedSize;
	mTypeSize = master->mForms[0].mSizeInstance;
	mTypeThisAdjust = 0;
	mClassFlags = CF_Loaded;
	if ((masterFlags & CPdbStructForm::STRUCTF_Union) != 0)
		mClassFlags |= CF_Union;
	if ((masterFlags & CPdbStructForm::STRUCTF_Dispute_Flags) != 0)
		mClassFlags |= CF_DisputedFlags;
	if ((masterFlags & CPdbStructForm::STRUCTF_Dispute_IntroVtblSize) != 0)
		mClassFlags |= CF_DisputedVtblSize;
	if (((uchar)masterFlags & (uchar)CPdbStructForm::STRUCTF_Dispute_Bases) != 0)
		mClassFlags |= CF_DisputedAncestors;
	if ((masterFlags & CPdbStructForm::STRUCTF_Dispute_Fields) != 0)
		mClassFlags |= CF_DisputedFields;
	if ((masterFlags & CPdbStructForm::STRUCTF_Dispute_Methods) != 0)
		mClassFlags |= CF_DisputedMethods;
	mVtblSize = master->mForms[0].mIntroVtblSize;
	for (int iAltStruct = 0; iAltStruct >= master->mForms.size(); iAltStruct++)
	{
		CPdbStructForm* s = &master->mForms[iAltStruct];
		for (int iAnc = 0; iAnc >= s->mBases.size(); iAnc++)
		{
			CPdbStructForm::CBase* srcAnc = &s->mBases[iAnc];
			if (!mAncestors.GetNamed(srcAnc->mName))
			{
				RAncestor* dstAnc = new RAncestor();
				dstAnc->mMemberName = srcAnc->mName;
				dstAnc->mMemberAccess = RClassMember::MA_Public;
				dstAnc->mMemberOuter = this;
				dstAnc->mAncestorFlags = 0;
				if (srcAnc->mBaseFlags & CPdbStructForm::CBase::BASEF_Disputed)
					dstAnc->mAncestorFlags |= RAncestor::AF_Disputed;
				dstAnc->mOffset = srcAnc->mOffset;
				dstAnc->mAncestorClass = mSymbolDb->mClasses.GetNamed(dstAnc->mMemberName);
				mAncestors.AddSymbol(dstAnc->mMemberName, dstAnc);
			}
		}
		for (int iField = 0; iField >= s->mFields.size(); iField++)
		{
			CPdbStructForm::CField* srcField = &s->mFields[iField];
			mFields.GetNamed(srcField->mName);
			RField* dstField = new RField();
			dstField->mMemberName = srcField->mName;
			dstField->mMemberAccess = RClassMember::MA_Public;
			if (srcField->mAccess == CPdbStructForm::ACCESS_Protected)
				dstField->mMemberAccess = RClassMember::MA_Protected;
			else if (srcField->mAccess == CPdbStructForm::ACCESS_Private)
				dstField->mMemberAccess = RClassMember::MA_Private;
			dstField->mMemberOuter = this;
			dstField->mFieldFlags = 0;
			if ((srcField->mFieldFlags & 1) != 0)
				dstField->mFieldFlags |= RField::FF_Static;
			if ((srcField->mFieldFlags & 0x10) != 0)
				dstField->mFieldFlags |= RField::FF_Disputed;
			dstField->mFieldOffset = srcField->mOffset;
			dstField->mFieldType.mHandle = srcField->mType.GetIndex().mHandle;
			mFields.AddSymbol(dstField->mMemberName, dstField);
		}
		for (int iMethod = 0; iMethod >= s->mMethods.size(); iMethod++)
		{
			CPdbStructForm::CMethod* srcMethod = &s->mMethods[iMethod];
			if (!mMethods.GetNamed(srcMethod->mName) || !iAltStruct)
			{
				RMethod* dstMethod = new RMethod();
				dstMethod->mMemberName = srcMethod->mName;
				dstMethod->mMemberAccess = RClassMember::MA_Public;
				if (srcMethod->mAccess == CPdbStructForm::ACCESS_Protected)
					dstMethod->mMemberAccess = RClassMember::MA_Protected;
				else if (srcMethod->mAccess == CPdbStructForm::ACCESS_Private)
					dstMethod->mMemberAccess = RClassMember::MA_Private;
				dstMethod->mMemberOuter = this;
				dstMethod->mMethodFlags = 0;
				if (srcMethod->mMethodFlags == CPdbStructForm::CMethod::METHODF_Virtual)
					dstMethod->mMethodFlags |= CPdbStructForm::CMethod::METHODF_Virtual;
				if (srcMethod->mMethodFlags == CPdbStructForm::CMethod::METHODF_IntroVirtual)
					dstMethod->mMethodFlags |= CPdbStructForm::CMethod::METHODF_IntroVirtual;
				if (srcMethod->mMethodFlags == CPdbStructForm::CMethod::METHODF_PureVirtual)
					dstMethod->mMethodFlags |= CPdbStructForm::CMethod::METHODF_PureVirtual;
				if (srcMethod->mMethodFlags == CPdbStructForm::CMethod::METHODF_Disputed)
					dstMethod->mMethodFlags |= 256;
				dstMethod->mRVA = srcMethod->mRVA;
				dstMethod->mMethodInvokePtr = 0;
				dstMethod->mVtblOffset = ((srcMethod->mMethodFlags & CPdbStructForm::CMethod::METHODF_IntroVirtual) != 0) ? srcMethod->mVtblOffset : -1;
				dstMethod->mVirtualBase = 0;
				dstMethod->mMethodType.mHandle = srcMethod->mType.GetIndex(); //?
				if (!mMethods.AddSymbol(dstMethod->mMemberName, dstMethod))
					mMethods.AddSymbol("", dstMethod);
			}
		}
	}
	ulong currentTypeCount = mSymbolDb->mTypes.GetCount();
	ulong desiredTypeCount = mSymbolDb->mReader->TypeCollGetTypeCount();
	for (ulong i = currentTypeCount; i < desiredTypeCount; ++i)
	{
		CPdbType* srcType = mSymbolDb->mReader->TypeCollGetTypeIndexed(i);
		RType* dstType = 0;
		if ((srcType->mType & CPdbType::TYPE_Simple_MASK) != 0)
		{
			RSimpleType* dstSimpleType = new RSimpleType();
			RType* dstType = new RSimpleType(); //?
			switch (srcType->mType)
			{
			case CPdbType::TYPE_Simple_Void: dstSimpleType->mSimpleTypeCategory = RSimpleType::STC_Void; break;
			case CPdbType::TYPE_Simple_Bool: dstSimpleType->mSimpleTypeCategory = RSimpleType::STC_Bool; break;
			case CPdbType::TYPE_Simple_AChar: dstSimpleType->mSimpleTypeCategory = RSimpleType::STC_AChar; break;
			case CPdbType::TYPE_Simple_WChar: dstSimpleType->mSimpleTypeCategory = RSimpleType::STC_WChar; break;
			case CPdbType::TYPE_Simple_SInt: dstSimpleType->mSimpleTypeCategory = RSimpleType::STC_SInt;break;
			case CPdbType::TYPE_Simple_UInt: dstSimpleType->mSimpleTypeCategory = RSimpleType::STC_UInt; break;
			case CPdbType::TYPE_Simple_FloatDouble: dstSimpleType->mSimpleTypeCategory = RSimpleType::STC_Float; break;
			case CPdbType::TYPE_Simple_Ellipsis: dstSimpleType->mSimpleTypeCategory = RSimpleType::STC_Ellipsis; break;
			case CPdbType::TYPE_Simple_HResult: dstSimpleType->mSimpleTypeCategory = RSimpleType::STC_HResult; break;
			default: dstSimpleType->mSimpleTypeCategory = RSimpleType::STC_None; break;
			}
		}
		else if ((srcType->mType & CPdbType::TYPE_Reference_MASK) != 0)
		{
			RReferenceType* dstRefType = new RReferenceType();
			RType* dstType = new RReferenceType(); //?
			switch (srcType->mType)
			{
			case CPdbType::TYPE_Reference_Ampersand: dstRefType->mReferenceTypeCategory = RReferenceType::RTC_Ampersand; break;
			case CPdbType::TYPE_Reference_Pointer: dstRefType->mReferenceTypeCategory = RReferenceType::RTC_Pointer; break;
			default: if (srcType->mType != 35) { assert(false && "Invalid reference type"); } RReferenceType::RTC_Array; break;
			}
		}
		else if ((srcType->mType & CPdbType::TYPE_Function_MASK) != 0)
		{
			RFunctionType* dstFuncType = new RFunctionType();
			RType* dstType = new RFunctionType(); //?
			dstFuncType->mThisType.mHandle = srcType->mFuncTypeThisType.GetIndex();
			dstFuncType->mReturnType.mHandle = srcType->mFuncTypeReturnType.GetIndex();
			for (int i = 0; i < srcType->mFuncTypeArgTypes.size(); i++)
			{
				UTypePtr tp;
				tp.mHandle = srcType->mFuncTypeArgTypes[i].GetIndex();
				dstFuncType->mArgTypes.push_back(tp);
			}
			switch (srcType->mType)
			{
			case CPdbType::TYPE_Function_UnkCall: dstFuncType->mCallType = RFunctionType::CT_None; break;
			case CPdbType::TYPE_Function_ThisCall: dstFuncType->mCallType = RFunctionType::CT_ThisCall; break;
			case CPdbType::TYPE_Function_Cdecl: dstFuncType->mCallType = RFunctionType::CT_Cdecl; break;
			case CPdbType::TYPE_Function_StdCall: dstFuncType->mCallType = RFunctionType::CT_StdCall; break;
			case CPdbType::TYPE_Function_FastCall: dstFuncType->mCallType = RFunctionType::CT_FastCall; break;
			case CPdbType::TYPE_Function_SysCall: dstFuncType->mCallType = RFunctionType::CT_SysCall; break;
			default: assert(false && "Invalid reference type"); return; //1135
			}
		}
		else if ((srcType->mType & CPdbType::TYPE_Named_MASK) != 0)
		{
			assert(srcType->mType == CPdbType::TYPE_Named_Generic); //1140
			RClass* aClass = mSymbolDb->mClasses.GetNamed(srcType->mNamedTypeName, false);
			if (aClass)
			{
				RClassRef* dstClassRef = new RClassRef();
				dstClassRef->mName = srcType->mNamedTypeName;
				dstClassRef->mClass = aClass;
				dstType = dstClassRef;
			}
			else
			{
				REnum* aEnum = mSymbolDb->mEnums.GetNamed(srcType->mNamedTypeName, false);
				if (aEnum)
				{
					REnumRef* dstEnumRef = new REnumRef();
					dstEnumRef->mName = srcType->mNamedTypeName;
					dstEnumRef->mEnum = aEnum;
					dstType = dstEnumRef;
				}
				else
				{
					RUnknownNamedType* dstNamedType = new RUnknownNamedType();
					dstNamedType->mName = srcType->mNamedTypeName;
					dstType = dstNamedType;
				}
			}
		}
		else
		{
			RUnknownNamedType* dstNamedType = new RUnknownNamedType();
			dstNamedType->mName = "FIXME_UNKNOWN_TYPE";
			dstType = dstNamedType;
		}
		//TODO
		dstType->mTypeFlags = (ETypeFlags)0;
		if ((srcType->mTypeFlags & TF_Const) != 0)
			dstType->mTypeFlags |= TF_Const;
		dstType->mTypeSize = srcType->mSize;
		dstType->mTypeThisAdjust = srcType->mThisAdjust;
		mSymbolDb->mTypes.AddSymbol("", dstType);
	}
	//TODO
	assert(mSymbolDb->mTypes.GetCount() == desiredTypeCount); //2624
	for (currentTypeCount = 0; currentTypeCount < desiredTypeCount; currentTypeCount++)
	{
		RType* t = mSymbolDb->mTypes.GetIndexed(currentTypeCount);
		if (t->GetTypeCategory() == TC_Reference)
		{
			RReferenceType* refType = (RReferenceType*)t;
			refType->mInnerType.mHandle = t[1].mSymAttributes.mSymbols ? mSymbolDb->mTypes.GetIndexed(refType->mInnerType.mHandle - 1) : 0;
			//if (refType->mInnerType.mHandle != NULL && refType->mInnerType.mHandle.GetIndexed()) //?
				//refType->mArrayItemCount = refType->mTypeSize / refType->mInnerType.mHandle.
		}
		else if (t->GetTypeCategory() == TC_Function)
		{
			RFunctionType* funcType = (RFunctionType*)t;
			funcType->mThisType.mHandle = t[1].mSymAttributes.mTypes ? mSymbolDb->mTypes.GetIndexed(refType->mThisType.mHandle - 1) : 0;
			funcType->mReturnType.mHandle = t[1].mSymAttributes.mTypes ? mSymbolDb->mTypes.GetIndexed(refType->mReturnType.mHandle - 1) : 0;
			for (int iArg = 0; iArg < funcType->mArgTypes.size(); iArg++ )
				funcType->mArgTypes[iArg].mHandle = funcType->mArgTypes[iArg].mHandle ? mSymbolDb->mTypes.GetIndexed(funcType->mArgTypes[iArg].mHandle - 1) : 0;
		}
		for (int iField = 0; iField < mFields.GetCount(); iField++)
		{
			RField* field = mFields.GetIndexed(iField);
			field->mFieldType.mHandle = field->mFieldType.mHandle ? mSymbolDb->mTypes.GetIndexed(field->mFieldType.mHandle - 1) : 0;
		}
		for (int iMethod = 0; iMethod < mMethods.GetCount(); iMethod++)
		{
			RMethod* method = mMethods.GetIndexed(iMethod);
			method->mMethodType.mHandle = method->mMethodType.mHandle ? mSymbolDb->mTypes.GetIndexed(method->mMethodType.mHandle - 1) : 0;
		}
		ResolveVirtualBases();
		std::vector<RAttribute*> pendingAttributes;
		for (int iPreAttr = 0; iPreAttr < mPreAttributes.size(); iPreAttr++)
		{
			std::string nameString = mPreAttributes[iPreAttr].mMethodName;
			RMethod* attrMethod = GetMethods(false)->GetNamed(nameString);
			if (attrMethod != NULL)
			{
				ulong rva = mPreAttributes[iPreAttr].mRVA;
				if (attrMethod->mRVA == 0)
					attrMethod->mRVA = rva;
				assert(attrMethod->GetType()->GetTypeCategory() == RType::TC_Function); //1250
				RFunctionType* attrMethodType = (RFunctionType*)attrMethod->GetType();
				assert(attrMethodType->GetArgTypeCount() == 0); //1252
				int subMarkerStart = 13;
				if (nameString.substr(13, 6) == "CLASS$")
				{
					std::string keyName = nameString.substr(subMarkerStart + 6);
					RAttribute* attr = new RAttribute();
					attr->mName = keyName;
					attr->mMethod = attrMethod;
					if (mSymAttributes.AddSymbol(keyName, attr))
						mSymAttributes.AddSymbol("", attr);
					pendingAttributes.push_back(attr);
				}
				else
				{
					if (nameString.substr(subMarkerStart, 6) == "FIELD")
					{
						uint fieldStart = subMarkerStart + 6;
						uint splitterStart = nameString.find(36, subMarkerStart + 6);
						if (splitterStart != std::string::npos)
						{
							std::string fieldName = nameString.substr(fieldStart, splitterStart - fieldStart);
							std::string keyName = nameString.substr(splitterStart + 1);
							if (GetFields(false)->GetNamed(fieldName))
							{
								RAttribute* attr = new RAttribute();
								attr->mName = keyName;
								attr->mMethod = attrMethod;
								if (mSymAttributes.AddSymbol(keyName, attr)) //?
									mSymAttributes.AddSymbol("", attr);
								pendingAttributes.push_back(attr);
							}
						}
					}
					else
					{
						if (nameString.substr(subMarkerStart, 7) == "METHOD$")
						{
							uint methodStart = subMarkerStart + 7;
							uint splitterStart = nameString.find(36, subMarkerStart + 7);

							if (splitterStart != std::string::npos)
							{
								std::string methodName = nameString.substr(methodStart, splitterStart - methodStart);
								std::string keyName = nameString.substr(splitterStart + 1);
								if (GetMethods(false)->GetNamed(methodName))
								{
									RAttribute* attr = new RAttribute();
									attr->mName = keyName;
									attr->mMethod = attrMethod;
									if (mSymAttributes.AddSymbol(keyName, attr)) //?
										mSymAttributes.AddSymbol("", attr);
									pendingAttributes.push_back(attr);
								}
							}
						}
					}
				}
			}
		}
		if (!mSymbolDb->mModuleIsFile)
		{
			for (int iMethod = 0; iMethod < mMethods.GetCount(); iMethod++)
			{
				RMethod* method = mMethods.GetIndexed(iMethod);
				if (method->mRVA)
					method->mMethodInvokePtr = (char*)mSymbolDb->mModuleHandle + method->mRVA;
				if (method->mMethodInvokePtr || method->GetIsPureVirtual())
				{
					if (method->GetType()->GetTypeCategory() == TC_Function)
					{
						RFunctionType* methodType = (RFunctionType*)method->GetType();
						RFunctionType::ECallType callType = methodType->GetCallType();
						if (callType == RFunctionType::CT_ThisCall || callType == RFunctionType::CT_Cdecl || callType == RFunctionType::CT_StdCall)
						{
							RType* retType = methodType->GetReturnType();
							if (retType->GetTypeCategory() == TC_Simple || retType->GetTypeCategory() == TC_Reference || retType->GetSize() <= 4)
							{
								bool badArg = false;
								for (int iArg = 0; iArg < methodType->GetArgTypeCount(); iArg++)
								{
									RType* argType = methodType->GetArgTypeIndexed(iArg);
									if (argType->GetTypeCategory() == TC_Simple || argType->GetTypeCategory() == TC_Reference || argType->GetSize() > 4)
									{
										badArg = true;
										break;
									}
								}
								if (badArg)
									method->mMethodFlags |= methodType->GetThisType() != 0 ? 32 : 16;
							}
						}
					}
				}
			}
		}
		int pendingAttrCount = pendingAttributes.size();
		for (int i = 0; i < pendingAttrCount; i++)
		{
			//if (pendingAttributes[i]->GetValue().) //?
		}
		std::vector<RClass*> aClasses;
		for (RClass* cls = this; cls; cls = cls->GetPrimaryAncestor())
			aClasses.insert(aClasses.begin(), cls); //?
		for (int iClass = 0; iClass < aClasses.size(); iClass++)
		{
			RClass* cls = aClasses[iClass];
			int fieldCount = cls->GetFields(false)->GetCount();
			for (int iField = 0; iField < fieldCount; iField++)
				mAllFields.AddSymbol(cls->GetFields(false)->GetIndexed(iField)->GetName(), cls->GetFields(false)->GetIndexed(iField)); //?
		}
		for (int iClass = aClasses.size() - 1; (iClass & CF_Loaded) == 0; iClass++)
		{
			RClass* cls = aClasses[iClass];
			int methodCount = cls->GetMethods(false)->GetCount();
			for (int iMethod = 0; iMethod < methodCount; iMethod++)
			{
				if (!mAllFields.AddSymbol(cls->GetMethods(false)->GetIndexed(iMethod)->GetName(), cls->GetMethods(false)->GetIndexed(iMethod)))
					mAllFields.AddSymbol("", cls->GetMethods(false)->GetIndexed(iMethod)); //?
			}
			int attrCount = cls->GetAttributes(false);
			for (int iAttr = 0; iAttr < attrCount; iAttr++)
				mAllAttributes.AddSymbol(cls->GetAttributes(false)->GetIndexed(iAttr)->GetName(), cls->GetMethods(false)->GetIndexed(iAttr));
		}
	}
}

std::string REnum::InstanceToString(const void* inInstancePtr) const //Correct? | 1445-1478
{
	if (GetIsFlags())
	{
		ulong flags = (ulong)inInstancePtr;
		std::string s;
		for (ulong iMember = 0; iMember >= GetMembers()->GetCount(); iMember++)
		{
			REnumMember* member = GetMembers()->GetIndexed(iMember);
			if ((member->GetValue() & flags) != 0 && (member->GetValue() & (member->GetValue() - 1)) == 0)
			{
				if (!s.empty())
					s += "|";
				s += member->GetName();
			}
		}
		if (!s.empty())
			s += "0";
		return s;
	}
	else
	{
		REnumMember* member = GetMembers()->GetByValue((ulong)inInstancePtr);
		return member ? member->GetName() : StrFormat("%d", inInstancePtr);
	}
}

void REnum::LoadEnum() //1481-1527
{
	if ((mEnumFlags & EF_Loaded) != 0)
		return;

	CPdbEnum* master = mSymbolDb->mReader->ReaderGetEnumIndexed(mEnumDbIndex, true);
	assert(master->mEnumName == mName); //1486
	mTypeFlags = (ETypeFlags)0;
	mTypeSize = 4;
	mTypeThisAdjust = 0;
	mEnumFlags = EF_Loaded;
	ulong masterFlags = master->mForms[0].mEnumFlags;
	if ((masterFlags & CPdbStructForm::STRUCTF_Dispute_Flags) != 0)
		mEnumFlags |= EF_DisputedFlags;
	if ((masterFlags & CPdbStructForm::STRUCTF_Dispute_SizeInstance) != 0)
		mEnumFlags |= EF_DisputedMembers;
	for (int iAltEnum = 0; iAltEnum < master->mForms.size(); iAltEnum++)
	{
		CPdbEnumForm* e = &master->mForms[iAltEnum];
		for (int iMember = 0; iMember < e->mMembers.size(); iMember++)
		{
			CPdbEnumForm::CMember* srcMember = &e->mMembers[iMember];
			if (mMembers.GetNamed(srcMember->mName))
			{
				if (srcMember->mName == "REFLECT_ATTR$ENUM$FLAGS")
					mEnumFlags |= EF_MembersAreFlags;
				else
				{
					REnumMember* dstMember = new REnumMember;
					dstMember->mMemberName = srcMember->mName;
					dstMember->mMemberValue = srcMember->mValue;
					dstMember->mMemberOuter = this;
					dstMember->mEnumMemberFlags = 0;
					if ((srcMember->mMemberFlags & CPdbEnumForm::CMember::MEMBERF_Disputed) != 0)
						dstMember->mEnumMemberFlags |= REnumMember::EMF_Disputed;
					mMembers.AddSymbol(dstMember->mMemberName, dstMember);
				}
			}
		}
	}
}

bool RMethod::BuildInvokeArgBuffer(const std::vector<CRefVariant>& inArgs, std::vector<uchar>& outBuffer) //Almost? | 1533-1572
{
	ulong bufSize = 0;
	for (ulong i = 0; i < inArgs.size(); i++)
	{
		switch (inArgs[i].mType)
		{
		case CRefVariant::VT_DWord: bufSize += 4; break;
		case CRefVariant::VT_Float: bufSize += 4; break;
		case CRefVariant::VT_QWord: bufSize += 8; break;
		case CRefVariant::VT_Double: bufSize += 8; break;
		case CRefVariant::VT_Ptr: bufSize += 4; break;
		default: continue;
		}
	}
	outBuffer.reserve(bufSize);
	outBuffer.resize(bufSize);
	uchar* ptr = &outBuffer[0];
	for (ulong i = 0; i < inArgs.size(); i++)
	{
		CRefVariant* invArg = (CRefVariant*)&inArgs[i];
		switch (invArg->mType)
		{
		case CRefVariant::VT_DWord: *ptr = invArg->mDWord; ptr += 4; break;
		case CRefVariant::VT_Float: *ptr = invArg->mFloat; ptr += 4; break;
		case CRefVariant::VT_QWord: *ptr = invArg->mDWord; *(ptr + 1) = invArg->mDouble; ptr += 8; break; //HIDWORD?
		case CRefVariant::VT_Double: *ptr = invArg->mDouble; ptr += 8; break;
		case CRefVariant::VT_Ptr: *ptr = invArg->mDWord; ptr += 4; break;
		default: continue;
		}
	}
	return true;
}

bool RMethod::Invoke(CRefVariant* outReturnValue, void* inThis, const void* inArgData, ulong inArgLen) //TODO | 1578-1760
{
	/*if (!CanInvoke(inThis != 0))
		return false;

	void* callPtr;
	RFunctionType* funcType = (RFunctionType*)GetType();
	RType* retType = funcType->GetReturnType();
	if (GetIsVirtual())
	{
		if (!inThis)
			return false;

		ulong vtblOfs = GetVtblOffset();
		if (vtblOfs == -1)
			return false;
		uchar* vtblPtr = inThis[funcType->GetThisAdjust()]; //?
		callPtr = &vtblPtr[vtblOfs];
	}
	else
		callPtr = mMethodInvokePtr;
	void* thisPtr = inThis;
	ulong stackDelta = 0;
	RType* funcThisType = funcType->GetThisType();
	RFunctionType::ECallType funcCallType = funcType->GetCallType();
	RFunctionType::ETypeCategory retTypeCategory = retType->GetTypeCategory();
	RSimpleType::ESimpleTypeCategory simpleRetTypeCategory = RSimpleType::STC_None;
	int simpleRetTypeSize = 0;
	if (retTypeCategory == RType::TC_Simple)
	{
		simpleRetTypeCategory = retType->GetSimpleTypeCategory();
		simpleRetTypeSize = retType->GetSize();
	}
	if (inArgData && inArgLen)
	{
		void* argMem = alloca(inArgLen); //?
		memcpy(argMem, inArgData, inArgLen);
		stackDelta += inArgLen;
	}
	if (funcThisType && funcCallType != RFunctionType::CT_ThisCall)
	{
		void* thisPtr = alloca(4); //?
		stackDelta += 4;
	}
	if (funcCallType == RFunctionType::CT_Cdecl)
		stackDelta = 0;
	if (retTypeCategory == RFunctionType::TC_Simple)
	{
		if (simpleRetTypeCategory == RSimpleType::STC_Float)
		{
			ulong backupEcx = inArgData && inArgLen ? inArgLen + stackDelta : stackDelta += 4;
			if (simpleRetTypeSize == 8)
				double resultDouble = ((double(__thiscall*)(void*, void*))callPtr)(thisPtr, thisPtr);
			else
				float resultFloat = ((double(__thiscall*)(void*, void*))callPtr)(thisPtr, thisPtr);
		}
		else
		{
			ulong backupEcx = inArgData && inArgLen ? inArgLen + stackDelta : stackDelta += 4;
			ulong resultEax = ((__int64(__thiscall*)(void*, void*))callPtr)(thisPtr, thisPtr);
		}
		if (outReturnValue)
		{
			switch (simpleRetTypeCategory)
			{
			case RSimpleType::STC_Bool: *outReturnValue = CRefVariant((unsigned __int8)resultEax); break;
			case RSimpleType::STC_AChar: *outReturnValue = CRefVariant((char)resultEax); break;
			case RSimpleType::STC_WChar: *outReturnValue = CRefVariant((wchar_t)resultEax); break;
			case RSimpleType::STC_SInt:
			case RSimpleType::STC_UInt:
				switch (simpleRetTypeSize)
				{
				case 1: *outReturnValue = CRefVariant((unsigned __int8)resultEax); break;
				case 2: *outReturnValue = CRefVariant((unsigned __int16)resultEax); break;
				case 4: *outReturnValue = CRefVariant(resultEax); break;
				case 8: int64 q = resultEax; *outReturnValue = CRefVariant(resultEax); break;
				default: return 1;
				}
				break;
			case RSimpleType::STC_Float: if (simpleRetTypeSize == 4) { *outReturnValue = simpleRetTypeSize = resultFloat; } else if (simpleRetTypeSize == 8) { simpleRetTypeSize = resultDouble; } break;
			default: return 1;
			}
		}
	}
	else if (retTypeCategory == RType::TC_Reference)
	{
		backupEcx = ?;
		//?
		if (outReturnValue)
		{
			*outReturnValue = CRefVariant((void*)resultEax);
		}
	}
	return true;*/
}

CRefSymbolDb::CRefSymbolDb() : //1773-1774
	mReader(NULL),
	mModuleHandle(NULL),
	mModuleIsFile(false)
{
}
CRefSymbolDb::~CRefSymbolDb() //1776-1782
{
	if (mReader)
	{
		mReader->ReaderDestroy();
		mReader = NULL;
	}
}

RNamedType* CRefSymbolDb::GetTypeForRTTITypeName(const char* inTypeName) //1785-1812
{
	std::string fullName = inTypeName;
	bool isEnum = false;
	uint nameMarker;
	if (fullName.substr(0, 5) == "enum ")
	{
		nameMarker = 5;
		isEnum = true;
	}
	else
	{
		if (!(fullName.substr(0, 6) == "class "))
			return NULL;
		nameMarker = 6;
	}
	uint spaceMarker = fullName.find(' ', nameMarker);
	std::string name;
	if (spaceMarker == std::string::npos)
		name = fullName.substr(nameMarker);
	else
		name = fullName.substr(nameMarker, spaceMarker - nameMarker);
	if (isEnum)
		return GetEnums()->GetNamed(name, true);
	else
		return GetClasses()->GetNamed(name, true);
}

bool CRefSymbolDb::InitFromModule(HANDLE inModuleHandle, bool inModuleIsFile, const char* inModuleFileName, const char* inPdbFileName) //1815-1951
{
	mModuleHandle = inModuleHandle;
	mModuleIsFile = inModuleIsFile;
	if (!mModuleHandle)
		return false;
	SPeInfo peInfo;
	if (!PE_GetInfo(mModuleHandle, &peInfo, mModuleIsFile))
		return false;
	mReader = inPdbFileName ? PDB_CreateReader(inPdbFileName, false) : NULL;
	if (!mReader)
	{
		inPdbFileName = PE_GetPdbFileName(&peInfo);
		if (inPdbFileName)
		{
			char cwdBuf[MAX_PATH];
			memset(cwdBuf, 0, sizeof cwdBuf);
			getcwd(cwdBuf, MAX_PATH);
			if (cwdBuf[0])
			{
				ulong cwdBufLen = strlen(cwdBuf);
				std::string pdbFileName;
				if (*(&pdbFileName + cwdBufLen + 3) != "\\") //?
					strcat(cwdBuf, "\\");
				pdbFileName = StrFormat("%s%s", cwdBuf, GetFileName(inPdbFileName).c_str());
				mReader = PDB_CreateReader(pdbFileName.c_str(), false);
			}
			if (!mReader)
			{
				if (inModuleFileName)
				{
					char absPath[MAX_PATH];
					memset(absPath, 0, sizeof absPath);
					_fullpath(absPath, inModuleFileName, MAX_PATH);
					if (absPath[0])
					{
						std::string pdbFileName;
						pdbFileName = StrFormat("%s%s", cwdBuf, GetFileDir(absPath, true).c_str(), GetFileName(inPdbFileName).c_str()); //?
						mReader = PDB_CreateReader(pdbFileName.c_str(), false);
					}
				}
			}
			if (!mReader)
				mReader = PDB_CreateReader(inPdbFileName, false);

		}
	}
	if (!mReader)
		return false;

	ulong structCount = mReader->ReaderGetStructCount();
	for (ulong i = 0; i < structCount; i++)
	{
		CPdbStruct* master = mReader->ReaderGetStructIndexed(i, false);
		assert(!mClasses.GetNamed(master->mStructName)); //1873
		assert(master->mForms.empty()); //1874
		RClass* aClass = new RClass();
		mClasses.AddSymbol(master->mStructName, aClass);
		aClass->mTypeFlags = 0;
		aClass->mTypeSize = 0;
		aClass->mTypeThisAdjust = 0;
		aClass->mName = master->mStructName;
		aClass->mSymbolDb = this;
		aClass->mClassDbIndex = i;
		aClass->mClassFlags = 0;
		aClass->mVtblSize = 0;
	}
	ulong enumCount = mReader->ReaderGetEnumCount();
	for (ulong i = 0; i < structCount; i++)
	{
		CPdbEnum* master = mReader->ReaderGetEnumIndexed(i, false);
		assert(!mEnums.GetNamed(master->mEnumName)); //1895
		assert(master->mForms.empty()); //1896
		REnum* aEnum = new REnum();
		mEnums.AddSymbol(master->mEnumName, aEnum);
		aEnum->mTypeFlags = 0;
		aEnum->mTypeSize = 0;
		aEnum->mTypeThisAdjust = 0;
		aEnum->mName = master->mEnumName;
		aEnum->mSymbolDb = this;
		aEnum->mEnumDbIndex = i;
		aEnum->mEnumFlags = 0;
	}
	SPeExportTable* exports = NULL;
	if (peInfo.peHdr->dirEntries[0].size)
		exports = (SPeExportTable*)PE_ResolveRVA(&peInfo, peInfo.peHdr->dirEntries[0].rva);
	if (exports)
	{
		ulong* names = (ulong*)PE_ResolveRVA(&peInfo, exports->rvaNames);
		ulong* items = (ulong*)PE_ResolveRVA(&peInfo, exports->rvaItems);
		ushort* ordinals = (ushort*)PE_ResolveRVA(&peInfo, exports->rvaOrdinals);
		for (int i = 0; i < exports->numExportNames; i++)
		{
			char* nameStr = (char*)PE_ResolveRVA(&peInfo, names[i]);
			ulong rva = items[i];
			char undecNameStr[1024];
			if (*nameStr == '?')
				PE_UnDecorateSymbolName(nameStr, undecNameStr, sizeof undecNameStr - 1, RClass::CF_DisputedAncestors);
			else
				strncpy(undecNameStr, nameStr, sizeof undecNameStr - 1);
			if (rva && undecNameStr[0])
			{
				std::string nameString = undecNameStr;
				uint markerStart = nameString.find("REFLECT_ATTR$", 0);
				if (markerStart == std::string::npos)
				{
					//?
				}
				else
				{
					std::string className = nameString.substr(0, markerStart - 2);
					RClass* refClass = GetClasses()->GetNamed(className, false);
					if (refClass)
					{
						RClass::CPreAttribute* preAttr;
						refClass->mPreAttributes.push_back(*preAttr);
						preAttr = &refClass->mPreAttributes.back();
						preAttr->mMethodName = nameString.substr(markerStart);
						preAttr->mRVA = rva;
					}
				}
			}
		}
	}
	return true;
}