#include "CfgMachine.h"
#include "Debug.h"
#include "SexyAppBase.h"

using namespace Sexy;

CfgCompiler::FeastException::FeastException(const std::string& inText) { mText = inText; } //38
const char* CfgCompiler::FeastException::LibExceptionGetText() { return mText.c_str(); } //39

void CfgCompiler::CfgSymbol::ReadProperty() //154-166
{
	if (mSymbolType != SYMBOL_Property)
		return;

	switch (mValue.GetType())
	{
	case CFGMVT_Bool: mValue.SetBoolean(gSexyAppBase->GetBoolean(mName, false)); break;
	case CFGMVT_Int: mValue.SetInteger(gSexyAppBase->GetInteger(mName, false)); break;
	case CFGMVT_Double: mValue.SetDouble(gSexyAppBase->GetDouble(mName, 0.0)); break;
	case CFGMVT_String: mValue.SetString(gSexyAppBase->GetString(mName, _S(""))); break;
	default: assert(false && "Unrecognized symbol data type"); return; //164
	}
}
void CfgCompiler::CfgSymbol::WriteProperty() //168-180
{
	if (mSymbolType != SYMBOL_Property)
		return;

	switch (mValue.GetType())
	{
	case CFGMVT_Bool: gSexyAppBase->SetBoolean(mName, mValue.GetBoolean()); break;
	case CFGMVT_Int: gSexyAppBase->SetInteger(mName, mValue.GetInteger()); break;
	case CFGMVT_Double: gSexyAppBase->SetDouble(mName, mValue.GetDouble()); break;
	case CFGMVT_String: gSexyAppBase->SetString(mName, mValue.GetString()); break;
	default: assert(false && "Unrecognized symbol data type"); return; //178
	}
}

CfgCompiler::CfgSymbol::CfgSymbol(CfgScope* inScope, ESymbolType inSymbolType, ECfgMachineValueType inDataType) { mSymbolType = inSymbolType; mValue = inDataType; mName = ""; mIndex = -1; mScope = inScope; } //193

CfgCompiler::ESymbolType CfgCompiler::CfgSymbol::GetSymbolType() { return mSymbolType; } //195

CfgMachineValue CfgCompiler::CfgSymbol::GetValue(CfgMachineValue inValue) { ReadProperty(); return mValue; } //197
void CfgCompiler::CfgSymbol::SetValue(const CfgMachineValue& inValue) { mValue = inValue; WriteProperty(); } //198

CfgCompiler::CfgAttribute::CfgAttribute() { mValueType = CFGMVT_None; mWriteableSymbol = NULL; } //211

void CfgCompiler::CfgAttribute::AttrDestroy() //214-216
{
	delete this;
}
//FEAST104
void CfgCompiler::ThrowError(const std::string inError, int inLine, int inColumn) //276-279
{
	std::string aErrorStr = StrFormat("%s(%d) : semantic error(%d) : %s", mCurFileName.c_str(), inLine, inColumn, inError.c_str());
	throw FeastException(aErrorStr);
}

void CfgCompiler::ThrowTokenError(SLexToken* inToken, const std::string inError) //281-285
{
	std::string aErrorStr = StrFormat("%s(%d) : semantic error(%d) : \"%0.*s\" : %s", mCurFileName.c_str(), inToken->mTextLine + 1, inToken->mTextColumn + 1, inToken->mLexemeLen, inToken->mLexeme, inError.c_str());
	throw FeastException(aErrorStr);
}

void CfgCompiler::ThrowNodeError(IPrsNode* inNode, const std::string inError) //287-318
{
	struct Local
	{
		static SLexToken* RecursiveGetFirstToken(IPrsNode* inLocalNode) //291-305
		{
			if (inLocalNode == NULL)
				return NULL;

			SLexToken* aToken = inLocalNode->NodeGetToken();
			if (aToken)
				return aToken;
			int aChildCount = inLocalNode->NodeGetChildCount();
			for (int iChild = 0; iChild < aChildCount; iChild++)
			{
				aToken = RecursiveGetFirstToken(inLocalNode->NodeGetChild(iChild));
				if (aToken)
					return aToken;
			}
			return NULL;
		};
	};
	SLexToken* aToken = Local::RecursiveGetFirstToken(inNode);
	if (aToken)
		ThrowError(inError, aToken->mTextColumn + 1, aToken->mTextLine + 1);
	if (inNode->NodeGetParent() == NULL)
		ThrowError(inError);
	ThrowNodeError(inNode->NodeGetParent(), inError);
}


std::string CfgCompiler::GetTokenString(SLexToken* inToken) //320-322
{
	return StrFormat("%0.*s", inToken->mLexemeLen, inToken->mLexeme);
}

CfgCompiler::CfgScope::CfgScope(CfgCompiler* inCompiler, CfgScope* inParentScope, int inScopeIndex, bool inIsFunctionScope) { mCompiler = inCompiler; mTempSymbolIndex = 0; mFirstOpIndex = 0; mParentScope = inParentScope; mScopeIndex = inScopeIndex; mIsFunctionScope = inIsFunctionScope; mFuncReturnType = CFGMVT_None; } //354

CfgCompiler::CfgScope::~CfgScope() //356-361
{
	int aSymbolCount = mSymbols.size();
	for (int i = 0; i < aSymbolCount; i++)
		delete mSymbols[i];
	mSymbols.clear();
}

CfgCompiler::CfgSymbol* CfgCompiler::CfgScope::GetSymbol(const std::string& inName) //364-371
{
	CfgStringToSymbolMap::iterator it = mSymbolMap.find(inName);
	if (it != mSymbolMap.end())
		return it->second;
	if (mParentScope)
		return mParentScope->GetSymbol(inName);
	return NULL;
}
//FEAST104
CfgCompiler::CfgSymbol* CfgCompiler::CfgScope::GetSymbol(SLexToken* inToken, bool inRequired) //373-379
{
	std::string aName = GetTokenString(inToken);
	CfgSymbol* aSymbol = GetSymbol(aName);
	if (aSymbol != NULL && inRequired)
		mCompiler->ThrowTokenError(inToken, "Unrecognized symbol");
	return aSymbol;
}

CfgCompiler::CfgSymbol* CfgCompiler::CfgScope::AddSymbol(SLexToken* inToken, ESymbolType inSymbolType, ECfgMachineValueType inDataType) //382-399
{
	std::string aName = GetTokenString(inToken);
	if (GetSymbol(aName))
		mCompiler->ThrowTokenError(inToken, "Duplicate symbol");
	CfgSymbol* aSymbol = new CfgSymbol(this, inSymbolType, inDataType);
	aSymbol->mName = aName;
	aSymbol->mIndex = mSymbols.size();
	mSymbols.push_back(aSymbol);
	CfgStringToSymbolMap::value_type p; //?
	p.first = aName; //?
	p.second = aSymbol;
	mSymbolMap.insert(p);
	return aSymbol;
}

CfgCompiler::CfgSymbol* CfgCompiler::CfgScope::AddTempSymbol(ESymbolType inSymbolType, ECfgMachineValueType inDataType) //401-411
{
	std::string aName = StrFormat("$T%d", ++mTempSymbolIndex);
	CfgSymbol* aSymbol = new CfgSymbol(this, inSymbolType, inDataType);
	aSymbol->mName = aName;
	aSymbol->mIndex = mSymbols.size();
	mSymbols.push_back(aSymbol);
	return aSymbol;
}

CfgCompiler::CfgScope* CfgCompiler::CfgScope::GetNearestFunctionScope() //414-420
{
	if (mIsFunctionScope)
		return this;
	if (mParentScope)
		return mParentScope->GetNearestFunctionScope();
	return this;
}

CfgCompiler::CfgMachine::CfgMachine(CfgCompiler* inCompiler) //444-447
	: mCompiler(inCompiler), mBlockEmit(false)
{
	mCurrentScope = NULL;
	mGlobalScope = PushScope();
}

CfgCompiler::CfgMachine::~CfgMachine() //449-454
{
	int aScopeCount = mScopes.size();
	for (int i = 0; i < aScopeCount; i++)
		delete mScopes[i];
	mScopes.clear();
}

CfgCompiler::CfgScope* CfgCompiler::CfgMachine::GetGlobalScope() { return mGlobalScope; } //456
CfgCompiler::CfgScope* CfgCompiler::CfgMachine::GetCurrentScope() { return mCurrentScope; } //457

CfgCompiler::CfgScope* CfgCompiler::CfgMachine::GetFunctionScope(const std::string& inFunctionName) //460-465
{
	CfgStringToScopeMap::iterator it = mFunctionScopeMap.find(inFunctionName);
	if (it != mFunctionScopeMap.end())
		return it->second;
	return NULL;
}

CfgCompiler::CfgScope* CfgCompiler::CfgMachine::PushFunctionScope(SLexToken* inToken) //467-483
{
	std::string aName = GetTokenString(inToken);
	if (GetFunctionScope(aName))
		mCompiler->ThrowTokenError(inToken, "Duplicate function");
	
	CfgScope* aScope = new CfgScope(mCompiler, mCurrentScope, mScopes.size(), true);
	mScopes.push_back(aScope);
	CfgStringToScopeMap::value_type p;
	p.first = aName;
	p.second = aScope;
	mFunctionScopeMap.insert(p);
}

CfgCompiler::CfgScope* CfgCompiler::CfgMachine::PushScope() //485-492
{
	CfgScope* aScope = new CfgScope(mCompiler, mCurrentScope, mScopes.size(), false);
	mScopes.push_back(aScope);
	mCurrentScope = aScope;
	return aScope;
}

void CfgCompiler::CfgMachine::PopScope() { mCurrentScope = mCurrentScope->mParentScope; } //Heh popcapscope | 494

void CfgCompiler::CfgMachine::Emit(ulong inOp) { if (!mBlockEmit) mOps.push_back(inOp); } //496
ulong CfgCompiler::CfgMachine::EmitFixup() { ulong index = mOps.size(); Emit(0); return index; } //497
void CfgCompiler::CfgMachine::SetFixup(ulong inIndex) { mOps[inIndex] = mOps.size(); } //498
void CfgCompiler::CfgMachine::BlockEmit(bool inBlock) { mBlockEmit = inBlock; } //499

bool CfgCompiler::CfgMachine::DisassembleToFile(const std::string& inFileName) //502-627
{
	if (mOps.size() < 0)
		return false;

	Buffer aBuffer;
	for (ulong aIP = mOps[0]; aIP != 1; aIP++)
	{
		ulong aOpCode = aIP;
		ulong aOpIndex = aIP - mOps[0];
		switch (aOpCode)
		{
		case OP_Nop: aBuffer.WriteLine(StrFormat("%d:\tNop", aOpIndex)); break;
		case OP_Call: ulong aTarget = aIP++; aBuffer.WriteLine(StrFormat("%d:\tCall @%d", aOpIndex, aTarget)); break;
		case OP_Ret: aBuffer.WriteLine(StrFormat("%d:\tRet", aOpIndex)); break;
		case OP_Load:
			CfgScope* aScope = mScopes[aIP++];
			CfgSymbol* aSymbol = aScope->mSymbols[aIP++];
			switch (aSymbol->GetSymbolType())
			{
			case SYMBOL_Constant:
				CfgMachineValue aValue = aSymbol->GetValue(aValue);
				std::string aValueStr = SexyStringToString(aValue.GetString());
				switch (aValue.GetType())
				{
				case CFGMVT_Bool: aBuffer.WriteLine(StrFormat("%d:\tLoad Const Bool \"%s\"", aOpIndex, aValueStr)); break;
				case CFGMVT_Int: aBuffer.WriteLine(StrFormat("%d:\tLoad Const Int \"%s\"", aOpIndex, aValueStr)); break;
				case CFGMVT_Double: aBuffer.WriteLine(StrFormat("%d:\tLoad Const Float \"%s\"", aOpIndex, aValueStr)); break;
				case CFGMVT_String: aBuffer.WriteLine(StrFormat("%d:\tLoad Const String \"%s\"", aOpIndex, aValueStr)); break;
				default: aBuffer.WriteLine(StrFormat("%d:**BAD**\tLoad Const UNKNOWN", aOpIndex)); break;
				}
			case SYMBOL_Var: aBuffer.WriteLine(StrFormat("%d:\tLoad Var \"%s\" [%d:%d]", aOpIndex, aSymbol->mName.c_str(), aSymbol->mIndex, aSymbol->mScope->mScopeIndex)); break;
			case SYMBOL_Property: aBuffer.WriteLine(StrFormat("%d:\tLoad Property \"%s\" [%d:%d]", aOpIndex, aSymbol->mName.c_str(), aSymbol->mIndex, aSymbol->mScope->mScopeIndex)); break;
			default: aBuffer.WriteLine(StrFormat("%d:**BAD**\tLoad UNKNOWN", aOpIndex)); break;
			}
		case OP_Store:
			CfgScope* aScope = mScopes[aIP++];
			CfgSymbol* aSymbol = aScope->mSymbols[aIP++];
			switch (aSymbol->GetSymbolType())
			{
			case SYMBOL_Constant: aBuffer.WriteLine(StrFormat("%d:**BAD**\tStore CONSTANT", aOpIndex)); break;
			case SYMBOL_Var: aBuffer.WriteLine(StrFormat("%d:\tStore Var \"%s\" [%d:%d]", aOpIndex, aSymbol->mName.c_str(), aSymbol->mIndex, aSymbol->mScope->mScopeIndex)); break;
			case SYMBOL_Property: aBuffer.WriteLine(StrFormat("%d:\tStore Property \"%s\" [%d:%d]", aOpIndex, aSymbol->mName.c_str(), aSymbol->mIndex, aSymbol->mScope->mScopeIndex)); break;
			default: aBuffer.WriteLine(StrFormat("%d:**BAD**\tStore UNKNOWN", aOpIndex)); break;
			}
		case OP_Copy: aBuffer.WriteLine(StrFormat("%d:\tCopy", aOpIndex)); break;
		case OP_Pop: aBuffer.WriteLine(StrFormat("%d:\tPop", aOpIndex)); break;
		case OP_Clear: aBuffer.WriteLine(StrFormat("%d:\tClear", aOpIndex)); break;
		case OP_Jmp: aBuffer.WriteLine(StrFormat("%d:\tJmp @%d", aOpIndex, aIP++)); break;
		case OP_Jt: aBuffer.WriteLine(StrFormat("%d:\tJt @%d", aOpIndex, aIP++)); break;
		case OP_Jf: aBuffer.WriteLine(StrFormat("%d:\tJf @%d", aOpIndex, aIP++)); break;
		case OP_Inc: aBuffer.WriteLine(StrFormat("%d:\tInc", aOpIndex)); break;
		case OP_Dec: aBuffer.WriteLine(StrFormat("%d:\tDec", aOpIndex)); break;
		case OP_Neg: aBuffer.WriteLine(StrFormat("%d:\tNeg", aOpIndex)); break;
		case OP_BNot: aBuffer.WriteLine(StrFormat("%d:\tBNot", aOpIndex)); break;
		case OP_LNot: aBuffer.WriteLine(StrFormat("%d:\tLNot", aOpIndex)); break;
		case OP_CastBool: aBuffer.WriteLine(StrFormat("%d:\tCastBool", aOpIndex)); break;
		case OP_CastInt: aBuffer.WriteLine(StrFormat("%d:\tCastInt", aOpIndex)); break;
		case OP_CastFloat: aBuffer.WriteLine(StrFormat("%d:\tCastFloat", aOpIndex)); break;
		case OP_CastString: aBuffer.WriteLine(StrFormat("%d:\tCastString", aOpIndex)); break;
		case OP_Mul: aBuffer.WriteLine(StrFormat("%d:\tMul", aOpIndex)); break;
		case OP_Div: aBuffer.WriteLine(StrFormat("%d:\tDiv", aOpIndex)); break;
		case OP_Mod: aBuffer.WriteLine(StrFormat("%d:\tMod", aOpIndex)); break;
		case OP_Add: aBuffer.WriteLine(StrFormat("%d:\tAdd", aOpIndex)); break;
		case OP_Sub: aBuffer.WriteLine(StrFormat("%d:\tSub", aOpIndex)); break;
		case OP_Shl: aBuffer.WriteLine(StrFormat("%d:\tShl", aOpIndex)); break;
		case OP_Shr: aBuffer.WriteLine(StrFormat("%d:\tShr", aOpIndex)); break;
		case OP_CmpLT: aBuffer.WriteLine(StrFormat("%d:\tCmpLT", aOpIndex)); break;
		case OP_CmpGT: aBuffer.WriteLine(StrFormat("%d:\tCmpGT", aOpIndex)); break;
		case OP_CmpLE: aBuffer.WriteLine(StrFormat("%d:\tCmpLE", aOpIndex)); break;
		case OP_CmpGE: aBuffer.WriteLine(StrFormat("%d:\tCmpGE", aOpIndex)); break;
		case OP_CmpEQ: aBuffer.WriteLine(StrFormat("%d:\tCmpEQ", aOpIndex)); break;
		case OP_CmpNE: aBuffer.WriteLine(StrFormat("%d:\tCmpNE", aOpIndex)); break;
		case OP_BAnd: aBuffer.WriteLine(StrFormat("%d:\tBAnd", aOpIndex)); break;
		case OP_BXor: aBuffer.WriteLine(StrFormat("%d:\tBXor", aOpIndex)); break;
		case OP_BOr: aBuffer.WriteLine(StrFormat("%d:\tBOr", aOpIndex)); break;
		case OP_Log: aBuffer.WriteLine(StrFormat("%d:\tLog", aOpIndex)); break;
		default: assert(false && "Invalid instruction"); break; //621
		}
	}
	gSexyAppBase->WriteBufferToFile(inFileName, &aBuffer);
}

void CfgCompiler::CfgMachine::Execute(CfgScope* inScope, CfgMachineValue* outReturnValue) //630-1031
{
	if (mOps.size() > 0)
	{
		std::vector<CfgMachineValue> aValueStack;
		std::vector<ulong> aCallStack;
		bool done = false;
		for (ulong aIP = mOps[inScope->mFirstOpIndex]; aIP != 1 && !done; aIP++)
		{
			ulong aOpCode = aIP;
			ulong aOpIndex = aIP - mOps[0];
			switch (aOpCode)
			{
			case OP_Nop: continue;
			case OP_Call: aCallStack.push_back(aOpIndex + 2); ulong aTarget = aIP++; aIP = mOps[aTarget] - 1; break;
			case OP_Ret:
				if (aCallStack.empty())
				{
					if (outReturnValue)
					{
						if (aValueStack.empty())
							outReturnValue = CfgMachineValue(CFGMVT_None); //?
						else
							outReturnValue = &aValueStack.back();
					}
					done = true;
				}
				else
				{
					aIP = mOps[aCallStack.back() - 1];
					aCallStack.pop_back();
				}
				break;
			case OP_Load: CfgScope* aScope = mScopes[aIP++]; CfgSymbol* aSymbol = aScope->mSymbols[aIP++]; aValueStack.push_back(aSymbol->GetValue(aSymbol)); break;
			case OP_Store: CfgScope* aScope = mScopes[aIP++]; CfgSymbol* aSymbol = aScope->mSymbols[aIP++]; CfgMachineValue aValue = aValueStack.back(); aSymbol->SetValue(aValue); break;
			case OP_Copy: aValueStack.push_back(aValueStack.back()); break;
			case OP_Pop: aValueStack.pop_back(); break;
			case OP_Clear: aValueStack.clear(); break;
			case OP_Jmp: mOps[aIP++] - 1; break;
			case OP_Jt:
			case OP_Jf:
				CfgMachineValue aValue = aValueStack.back();
				aValueStack.pop_back();
				bool aCondition = aValue.GetBoolean();
				if (aOpCode == OP_Jf)
					aCondition != aCondition;
				if (aCondition)
					aIP = mOps[aIP++] - 1;
				break;
			case OP_Inc:
			case OP_Dec:
				CfgMachineValue aValue = aValueStack.back();
				aValueStack.pop_back();
				if (aValue.GetType() == CFGMVT_Int)
				{
					if (aOpCode == OP_Inc)
						aValue.SetInteger(aValue.GetInteger() + 1);
					else
						aValue.SetInteger(aValue.GetInteger() - 1);
				}
				else
				{
					if (aValue.GetType() != CFGMVT_Double)
						assert("false && \"OP_Inc / OP_Dec : Invalid value type\""); //758
					if (aOpCode == OP_Inc)
						aValue.SetDouble(aValue.GetDouble() + 1.0);
					else
						aValue.SetDouble(aValue.GetDouble() - 1.0);
				}
				aValueStack.push_back(aValue);
				break;
			case OP_Neg:
				CfgMachineValue aValue = aValueStack.back();
				aValueStack.pop_back();
				if (aValue.GetType() == CFGMVT_Int)
					aValue.SetInteger(-aValue.GetInteger());
				else
				{
					if (aValue.GetType() != CFGMVT_Double)
						assert("false && \"OP_Neg : Invalid value type\""); //779
					aValue.SetDouble(-aValue.GetDouble());
				}
				aValueStack.push_back(aValue);
				break;
			case OP_BNot:
				CfgMachineValue aValue = aValueStack.back();
				aValueStack.pop_back();
				if (aValue.GetType() != CFGMVT_Double)
					assert("false && \"OP_BNot : Invalid value type\""); //796
				aValue.SetInteger(-aValue.GetInteger());
				aValueStack.push_back(aValue);
				break;
			case OP_LNot:
				CfgMachineValue aValue = aValueStack.back();
				aValueStack.pop_back();
				if (aValue.GetType() == CFGMVT_Bool)
					aValue.SetBoolean(!aValue.GetBoolean());
				else
				{
					if (aValue.GetType() != CFGMVT_Int)
						assert("false && \"OP_LNot : Invalid value type\""); //817
					aValue.SetInteger(aValue.GetInteger() == 0);
				}
				aValue.SetInteger(-aValue.GetInteger());
				aValueStack.push_back(aValue);
				break;
			case OP_CastBool:
				CfgMachineValue aValue = aValueStack.back();
				aValueStack.pop_back();
				CfgMachineValue aOutValue(CFGMVT_Bool);
				aOutValue.SetBoolean(aOutValue.GetBoolean());
				aValueStack.push_back(aValue);
				break;
			case OP_CastInt:
				CfgMachineValue aValue = aValueStack.back();
				aValueStack.pop_back();
				CfgMachineValue aOutValue(CFGMVT_Int);
				aOutValue.SetInteger(aOutValue.GetInteger());
				aValueStack.push_back(aValue);
				break;
			case OP_CastFloat:
				CfgMachineValue aValue = aValueStack.back();
				aValueStack.pop_back();
				CfgMachineValue aOutValue(CFGMVT_Double);
				aOutValue.SetDouble(aOutValue.GetDouble());
				aValueStack.push_back(aValue);
				break;
			case OP_CastString:
				CfgMachineValue aValue = aValueStack.back();
				aValueStack.pop_back();
				CfgMachineValue aOutValue(CFGMVT_String);
				aOutValue.SetString(aOutValue.GetString());
				aValueStack.push_back(aValue);
				break;
			case OP_Mul:
			case OP_Div:
			case OP_Mod:
			case OP_Add:
			case OP_Sub:
				CfgMachineValue aRightValue = aValueStack.back();
				aValueStack.pop_back();
				CfgMachineValue aLeftValue = aValueStack.back();
				CfgMachineValue aOutValue(CFGMVT_None);
				aOutValue.SetType(aLeftValue.GetPromotedDataType(aLeftValue.GetType(), aRightValue.GetType()));
				if (aOutValue.GetType() == CFGMVT_Int)
				{
					switch (aOpCode)
					{
					case OP_Mul: aOutValue.SetInteger(aRightValue.GetInteger() * aLeftValue.GetInteger()); break;
					case OP_Div: aOutValue.SetInteger(aRightValue.GetInteger() / aLeftValue.GetInteger()); break;
					case OP_Mod: aOutValue.SetInteger(aRightValue.GetInteger() % aLeftValue.GetInteger()); break;
					case OP_Add: aOutValue.SetInteger(aRightValue.GetInteger() + aLeftValue.GetInteger()); break;
					case OP_Sub: aOutValue.SetInteger(aRightValue.GetInteger() - aLeftValue.GetInteger()); break;
					default: assert(false && "Invalid instruction for this data type"); break; //886
					}
				}
				if (aOutValue.GetType() == CFGMVT_Double)
				{
					switch (aOpCode)
					{
					case OP_Mul: aOutValue.SetDouble(aRightValue.GetDouble() * aLeftValue.GetDouble()); break;
					case OP_Div: aOutValue.SetDouble(aRightValue.GetDouble() / aLeftValue.GetDouble()); break;
					case OP_Mod: aOutValue.SetDouble(fmod(aRightValue.GetDouble(), aLeftValue.GetDouble())); break;
					case OP_Add: aOutValue.SetDouble(aRightValue.GetDouble() + aLeftValue.GetDouble()); break;
					case OP_Sub: aOutValue.SetDouble(aRightValue.GetDouble() - aLeftValue.GetDouble()); break;
					default: assert(false && "Invalid instruction for this data type"); break; //898
					}
				}
				else
				{
					if (aOpCode != OP_Add)
						assert(false && "Invalid instruction for this data type"); //906
					if (aOutValue.GetType() != CFGMVT_String)
						assert(false && "OP_Mul/OP_Div/OP_Mod/OP_Add/OP_Sub: Invalid value type"); //911
					aOutValue.SetString(aRightValue.GetString() + aLeftValue.GetString());
				}
				aValueStack.push_back(aOutValue);
				break;
			case OP_Shl:
			case OP_Shr:
			case OP_BAnd:
			case OP_BXor:
			case OP_BOr:
				CfgMachineValue aRightValue = aValueStack.back();
				aValueStack.pop_back();
				CfgMachineValue aLeftValue = aValueStack.back();
				CfgMachineValue aOutValue(CFGMVT_None);
				aOutValue.SetType(aLeftValue.GetPromotedDataType(aLeftValue.GetType(), aRightValue.GetType()));
				switch (aOpCode)
				{
				case OP_Shl: aOutValue.SetInteger(aRightValue.GetInteger() << aLeftValue.GetInteger()); break;
				case OP_Shr: aOutValue.SetInteger(aRightValue.GetInteger() >> aLeftValue.GetInteger()); break;
				case OP_BAnd: aOutValue.SetInteger(aRightValue.GetInteger() & aLeftValue.GetInteger()); break;
				case OP_BXor: aOutValue.SetInteger(aRightValue.GetInteger() ^ aLeftValue.GetInteger()); break;
				case OP_BOr: aOutValue.SetInteger(aRightValue.GetInteger() | aLeftValue.GetInteger()); break;
				default: assert(false && "Invalid instruction for this data type"); break; //940
				}
				if (aOutValue.GetType() != CFGMVT_Int)
					assert(false && "OP_Shl/OP_Shr/OP_BAnd/OP_BXor/OP_BOr: Invalid value type"); //945
				aValueStack.push_back(aOutValue);
				break;
			case OP_CmpLT:
			case OP_CmpGT:
			case OP_CmpLE:
			case OP_CmpGE:
			case OP_CmpEQ:
			case OP_CmpNE:
				CfgMachineValue aRightValue = aValueStack.back();
				aValueStack.pop_back();
				CfgMachineValue aLeftValue = aValueStack.back();
				ECfgMachineValueType aCompareType = aLeftValue.GetPromotedDataType(aLeftValue.GetType(), aRightValue.GetType());
				CfgMachineValue aOutValue(CFGMVT_Bool);
				switch (aCompareType)
				{
				case CFGMVT_Bool:
					if (aOpCode == OP_CmpEQ)
						aOutValue.SetBoolean(aRightValue.GetBoolean() == aLeftValue.GetBoolean());
					else
					{
						if (aOpCode != OP_CmpNE)
							assert(false && "Invalid instruction for this data type");
						aOutValue.SetBoolean(aRightValue.GetBoolean() != aLeftValue.GetBoolean());
					}
					break;
				case CFGMVT_Int:
					switch (aOpCode)
					{
					case OP_CmpLT: aOutValue.SetBoolean(aRightValue.GetInteger() < aLeftValue.GetInteger()); break;
					case OP_CmpGT: aOutValue.SetBoolean(aRightValue.GetInteger() > aLeftValue.GetInteger()); break;
					case OP_CmpLE: aOutValue.SetBoolean(aRightValue.GetInteger() <= aLeftValue.GetInteger()); break;
					case OP_CmpGE: aOutValue.SetBoolean(aRightValue.GetInteger() >= aLeftValue.GetInteger()); break;
					case OP_CmpEQ: aOutValue.SetBoolean(aRightValue.GetInteger() == aLeftValue.GetInteger()); break;
					case OP_CmpNE: aOutValue.SetBoolean(aRightValue.GetInteger() != aLeftValue.GetInteger()); break;
					default: assert(false && "Invalid instruction for this data type"); return; //986
					}
					break;
				case CFGMVT_Double:
					switch (aOpCode)
					{
					case OP_CmpLT: aOutValue.SetBoolean(aRightValue.GetDouble() < aLeftValue.GetDouble()); break;
					case OP_CmpGT: aOutValue.SetBoolean(aRightValue.GetDouble() > aLeftValue.GetDouble()); break;
					case OP_CmpLE: aOutValue.SetBoolean(aRightValue.GetDouble() <= aLeftValue.GetDouble()); break;
					case OP_CmpGE: aOutValue.SetBoolean(aRightValue.GetDouble() >= aLeftValue.GetDouble()); break;
					case OP_CmpEQ: aOutValue.SetBoolean(aRightValue.GetDouble() == aLeftValue.GetDouble()); break;
					case OP_CmpNE: aOutValue.SetBoolean(aRightValue.GetDouble() != aLeftValue.GetDouble()); break;
					default: assert(false && "Invalid instruction for this data type"); return; //999
					}
					break;
				default:
					if (aCompareType != CFGMVT_String)
						assert(false && "Comparison opcode: Invalid value type"); return; //1013
					if (aOpCode == OP_CmpEQ)
						aOutValue.SetBoolean(wcsicmp(SexyStringToWString(aRightValue.GetString()).c_str(), SexyStringToWString(aRightValue.GetString()).c_str()));
					else
					{
						if (aOutValue.GetType() != OP_CmpNE)
							assert(false && "Invalid instruction for this data type"); //1008
						aOutValue.SetBoolean(wcsicmp(SexyStringToWString(aRightValue.GetString()).c_str(), SexyStringToWString(aRightValue.GetString()).c_str()));
					}
					break;
				}
				aValueStack.push_back(aOutValue);
				break;
			case OP_Log:
				CfgMachineValue aValue = aValueStack.back();
				aValueStack.pop_back();
				aValue.GetString();
				OutputDebugStrF("CFGLOG: %s\n", SexyStringToString(aValue.GetString()).c_str());
				break;
			default: assert(false && "Invalid instruction"); //1028
			}
		}
	}
}

void CfgCompiler::CfgMachine::MachineDestroy() //1037-1039
{
	delete this;
}

bool CfgCompiler::CfgMachine::MachineExecuteFunction(const char* inFunctionName, CfgMachineValue* outReturnValue) //1042-1058
{
	if (inFunctionName)
	{
		CfgScope* aScope = GetFunctionScope(inFunctionName);
		if (aScope)
		{
			Execute(aScope, outReturnValue);
			return true;
		}
		else
			return false;
	}
	else
		Execute(GetGlobalScope(), outReturnValue);
}

bool CfgCompiler::CfgMachine::MachineDisassembleToFile(const std::string& inFileName) //1061-1063
{
	DisassembleToFile(inFileName);
}

CfgCompiler::CfgCompiler() //1073-1074
{
	mParser, mMachine = NULL;
}

CfgCompiler::~CfgCompiler() //1076-1079
{
	if (mParser)
		mParser->PrsDestroy();
}

void CfgCompiler::ExecuteTree(IPrsNode* inRoot) //1082-1826
{
	struct Local //TODO
	{
		static ECfgMachineValueType GetNodeValueType(IPrsNode* inNode) //1086-1091
		{
			CfgAttribute* aAttr = (CfgAttribute*)inNode->NodeGetAttr();
			ECfgMachineValueType aValueType = aAttr ? aAttr->mValueType : CFGMVT_None;
			assert(aValueType != CFGMVT_None); //1089
			return aValueType;
		}
		static CfgSymbol* GetNodeWriteableSymbol(IPrsNode* inNode) //1093-1096
		{
			CfgAttribute* aAttr = (CfgAttribute*)inNode->NodeGetAttr();
			return aAttr ? aAttr->mWriteableSymbol : NULL;
		}
		static void SetNodeValueType(IPrsNode* inNode, ECfgMachineValueType inValueType, CfgSymbol* inWriteableSymbol) //1098-1107
		{
			CfgAttribute* aAttr = (CfgAttribute*)inNode->NodeGetAttr();
			if (aAttr == NULL)
			{
				aAttr = new CfgAttribute();
				inNode->NodeSetAttr(aAttr);
			}
			aAttr->mValueType = inValueType;
			aAttr->mWriteableSymbol = inWriteableSymbol;
		}
		static ECfgMachineValueType ExecuteVarTypeNode(IPrsNode* inNode) //1110-1121
		{
			ECfgMachineValueType aType = (ECfgMachineValueType)inNode->NodeGetTag();
			switch (aType)
			{
			case NODE_VarTypeSpecBool: aType = CFGMVT_Bool; break;
			case NODE_VarTypeSpecInt: aType = CFGMVT_Int; break;
			case NODE_VarTypeSpecFloat: aType = CFGMVT_Double; break;
			case NODE_VarTypeSpecString: aType = CFGMVT_String; break;
			default: assert(false && "Unrecognized var type"); return; //1118
			}
			return aType;
		}
		static void ExecuteVarDeclIdentNode(CfgCompiler* inInfo, IPrsNode* inNode, ECfgMachineValueType inDataType, ESymbolType inSymbolType) //1124-1157
		{
			if (inNode == NULL)
				return;

			if (inNode->NodeGetTag() == NODE_List)
			{
				//ExecuteVarDeclIdentNode(inInfo, inNode->NodeGetChild(0), inSymbolType, v8);
				ExecuteVarDeclIdentNode(inInfo, inNode->NodeGetChild(0), CFGMVT_None, inSymbolType);
				//ExecuteVarDeclIdentNode(inInfo, inNode->NodeGetChild(1), CFGMVT_Bool, inSymbolType);
			}
			else if (inNode->NodeGetTag() == NODE_VarDeclIdent)
			{
				assert(inNode->NodeGetChild(0)->NodeGetTag() == NODE_TermIdentifier); //1138
				CfgSymbol* aIdentSymbol; // = inInfo->mMachine->AddSymbol(); | inInfo->mMachine->GetCurrentScope()->AddSymbol(inNode->NodeGetChild(inNode), SYMBOL_None, inNode->NodeGetChild(inNode)); //?
				IPrsNode* aExprNode = inNode->NodeGetChild(1);
				if (aExprNode != NULL)
				{
					ExecuteNode(inInfo, aExprNode);
					inInfo->mMachine->Emit(OP_Store);
					inInfo->mMachine->Emit(aIdentSymbol->mScope->mScopeIndex);
					inInfo->mMachine->Emit(aIdentSymbol->mIndex);
					inInfo->mMachine->Emit(OP_Pop);
				}
			}
		}

		static void ExecuteCallArgListNode(CfgCompiler* inInfo, IPrsNode* inNode, ulong& ioArgCount) //1160-1179
		{
			if (inNode->NodeGetTag() == NODE_List)
			{
				//ExecuteCallArgListNode(inInfo, inNode->NodeGetChild(0), 7);
				//ExecuteCallArgListNode(inInfo, inNode->NodeGetChild(), 1);
			}
			if (inNode->NodeGetTag() == NODE_CallArg)
			{
				ExecuteNode(inInfo, inNode->NodeGetChild(0));
				++ioArgCount;
			}
		}

		static void ExecuteFuncDeclArgListNode(CfgCompiler* inInfo, IPrsNode* inNode, CfgScope* inFuncScope) //1182-1202
		{
			if (inNode->NodeGetTag() == NODE_List)
			{
				//ExecuteFuncDeclArgListNode(inInfo, inNode->NodeGetChild(0), inSymbolType, v8);
				//ExecuteFuncDeclArgListNode(inInfo, inNode->NodeGetChild(1), CFGMVT_Bool, inSymbolType);
			}
			else if (inNode->NodeGetTag() == NODE_FuncDeclArg)
			{
				assert(inNode->NodeGetChild(1)->NodeGetTag() == NODE_TermIdentifier); //1193
				ECfgMachineValueType aArgType = ExecuteVarTypeNode(inNode->NodeGetChild(0));
				//CfgSymbol* aArgSymbol = inFuncScope->AddSymbol(inNode->NodeGetChild(1)->NodeGetToken(), SYMBOL_Constant, CFGMVT_None);
				//inFuncScope->mFuncArgSymbols.push_back(aArgSymbol);
			}
		}

		static void ExecuteNode(CfgCompiler* inInfo, IPrsNode* inNode) //1205-1820
		{
			if (inNode == NULL)
				return;

			switch (inNode->NodeGetTag())
			{
			case NODE_TermIdentifier:
				std::string aTokenString = GetTokenString(inNode->NodeGetToken());
				if (aTokenString == "true" || aTokenString == "false")
				{
					CfgMachineValue aValue(CFGMVT_Bool);
					CfgSymbol* aTempSymbol = inInfo->mMachine->GetCurrentScope()->AddTempSymbol(SYMBOL_Constant, CFGMVT_Bool);
					//aTempSymbol->SetValue(aValue.SetString(StringToSexyString(aTokenString)));
					//aTempSymbol->SetValue(aValue.SetString(StringToSexyString(aTokenString); //?
					inInfo->mMachine->Emit(OP_Load);
					inInfo->mMachine->Emit(aTempSymbol->mScope->mScopeIndex);
					inInfo->mMachine->Emit(aTempSymbol->mIndex);
					SetNodeValueType(inNode, CFGMVT_Bool, NULL);
				}
				else
				{
					CfgSymbol* aIdentSymbol = inInfo->mMachine->GetCurrentScope()->GetSymbol(inNode->NodeGetToken(), true);
					inInfo->mMachine->Emit(OP_Load);
					inInfo->mMachine->Emit(aIdentSymbol->mScope->mScopeIndex);
					inInfo->mMachine->Emit(aIdentSymbol->mIndex);
					//SetNodeValueType(inNode, ?, aIdentSymbol); //?
				}
				break;
			case NODE_TermString:
				inInfo->mMachine->GetCurrentScope()->AddTempSymbol(SYMBOL_Constant, CFGMVT_String);
				CfgSymbol* aIdentSymbol; //?
				inInfo->mMachine->Emit(OP_Load);
				inInfo->mMachine->Emit(aIdentSymbol->mScope->mScopeIndex);
				inInfo->mMachine->Emit(aIdentSymbol->mIndex);
				SetNodeValueType(inNode, CFGMVT_String, NULL);
				break;
			}
			//assert(inNode->NodeGetChild(0)->NodeGetTag() == NODE_TermIdentifier) //1281
			//assert(aTokenString.length() >= 2) //1361
			//assert(aTokenString.length() >= 3) //1408
			inInfo->ThrowNodeError(inNode, "operator '++' and '--' can only be used with int and float types");
		}
	};
	Local::ExecuteNode(this, inRoot);
	mMachine->Emit(OP_End);
}

void CfgCompiler::InitGrammar(IPrsParser* inParser) //1829-2060
{
	struct Local
	{
		static void BlockCommentIntercept(ILexLexer* inLex, SLexToken* inToken) //1841-1854 | //auto BlockCommentIntercept = [](ILexLexer* inLex, SLexToken* inToken) //1841-1854
		{
			char c;
			do
			{
				c = inLex->LexGetChar();
				if (!c)
					throw(FeastException("Found block comment without terminating \"*/\""));
			} while (c != '*' || inLex->LexGetChar(false) != '*');
			inLex->LexGetChar(true);
			inToken->mTag = 0;
		};
	};
	inParser->PrsRegisterNT("start", "statement_list", NODE_None);
	ILexLexer* lexer = inParser->PrsGetLexer();
	lexer->LexCaseSensitivity(true);
	lexer->LexTokenPriority(0);
	lexer->LexRegisterToken(TOKEN_None, ".");
	lexer->LexTokenPriority(1);
	lexer->LexRegisterToken(TOKEN_None, lexer->LexGetStockRegex(LEXREGEX_Whitespace));
	lexer->LexRegisterToken(TOKEN_None, lexer->LexGetStockRegex(LEXREGEX_EolComment));
	lexer->LexRegisterToken(TOKEN_BlockComment, "\\/\\*");
	lexer->LexTokenIntercept(TOKEN_BlockComment, Local::BlockCommentIntercept);
	lexer->LexRegisterToken(TOKEN_Identifier, lexer->LexGetStockRegex(LEXREGEX_Identifier));
	lexer->LexRegisterToken(TOKEN_String, lexer->LexGetStockRegex(LEXREGEX_String));
	lexer->LexRegisterToken(TOKEN_Float, lexer->LexGetStockRegex(LEXREGEX_Float));
	lexer->LexRegisterToken(TOKEN_DecInteger, lexer->LexGetStockRegex(LEXREGEX_DecInteger));
	lexer->LexRegisterToken(TOKEN_HexInteger, lexer->LexGetStockRegex(LEXREGEX_HexInteger));
	lexer->LexRegisterToken(TOKEN_Character, lexer->LexGetStockRegex(LEXREGEX_Character));
	inParser->PrsRegisterT("IDENTIFIER", LEXREGEX_Whitespace, NODE_TermIdentifier);
	inParser->PrsRegisterT("STRING", LEXREGEX_EolComment, NODE_TermString);
	inParser->PrsRegisterT("FLOAT", LEXREGEX_Identifier, NODE_TermFloat);
	inParser->PrsRegisterT("DECINTEGER", LEXREGEX_DecInteger, NODE_TermDecInteger);
	inParser->PrsRegisterT("HEXINTEGER", LEXREGEX_HexInteger, NODE_TermHexInteger);
	inParser->PrsRegisterT("CHARACTER", LEXREGEX_BlockComment, NODE_TermCharacter);
	inParser->PrsRegisterNT("integer", "DECINTEGER", NODE_None);
	inParser->PrsRegisterNT("integer", "HEXINTEGER", NODE_None);
	inParser->PrsRegisterNT("integer", "CHARACTER", NODE_None);
	inParser->PrsRegisterNT("statement_list", "statement", NODE_None);
	inParser->PrsRegisterNT("statement_list", "statement_list:1 statement:2", NODE_List);
	inParser->PrsRegisterNT("statement", "declaration_statement", NODE_None);
	inParser->PrsRegisterNT("statement", "compound_statement", NODE_None);
	inParser->PrsRegisterNT("statement", "expression_statement", NODE_None);
	inParser->PrsRegisterNT("statement", "selection_statement", NODE_None);
	inParser->PrsRegisterNT("statement", "log_statement", NODE_None);
	inParser->PrsRegisterNT("statement", "return_statement", NODE_None);
	inParser->PrsRegisterNT("log_statement", "\"log\" '(' expr:1 ')' ';'", NODE_Log);
	inParser->PrsRegisterNT("return_statement", "\"return\":0 ';'", NODE_Return);
	inParser->PrsRegisterNT("return_statement", "\"return\" expr:1 ';'", NODE_Return);
	inParser->PrsRegisterNT("declaration_statement", "var_decl:1 ';'", NODE_None);
	inParser->PrsRegisterNT("declaration_statement", "func_decl:1", NODE_None);
	inParser->PrsRegisterNT("compound_statement", "'{':0 '}'", NODE_StmtEmpty);
	inParser->PrsRegisterNT("compound_statement", "'{' statement_list:1 '}'", NODE_StmtCompound);
	inParser->PrsRegisterNT("expression_statement", "';':0", NODE_StmtEmpty);
	inParser->PrsRegisterNT("expression_statement", "expr:1 ';'", NODE_StmtExpr);
	inParser->PrsRegisterNT("selection_statement", "\"if\" '(' expr:1 ')' statement:2", NODE_SelIf);
	inParser->PrsRegisterNT("selection_statement", "\"if\" '(' expr:1 ')' statement:2 \"else\" statement:3", NODE_SelIf);
	inParser->PrsRegisterNT("var_decl", "\"var\" var_type:1 var_decl_ident_list:2", NODE_VarDecl);
	inParser->PrsRegisterNT("var_decl", "\"property\" var_type:1 var_decl_ident_list:2", NODE_PropertyDecl);
	inParser->PrsRegisterNT("var_decl_ident_list", "var_decl_ident", NODE_None);
	inParser->PrsRegisterNT("var_decl_ident_list", "var_decl_ident_list:1 ',' var_decl_ident:2", NODE_List);
	inParser->PrsRegisterNT("var_decl_ident", "IDENTIFIER:1", NODE_VarDeclIdent);
	inParser->PrsRegisterNT("var_decl_ident", "IDENTIFIER:1 '=' constant_expr:2", NODE_VarDeclIdent);
	inParser->PrsRegisterNT("func_decl", "\"function\" IDENTIFIER:1 '{' '}'", NODE_FuncDecl);
	inParser->PrsRegisterNT("func_decl", "\"function\" IDENTIFIER:1 '{' statement_list:2 '}'", NODE_FuncDecl);
	inParser->PrsRegisterNT("func_decl", "\"function\" '(' var_type:3 ')' IDENTIFIER:1 '{' '}'", NODE_FuncDecl);
	inParser->PrsRegisterNT("func_decl", "\"function\" '(' var_type:3 ')' IDENTIFIER:1 '{' statement_list:2 '}'", NODE_FuncDecl);
	inParser->PrsRegisterNT("func_decl", "\"function\" IDENTIFIER:1 '(' func_decl_arg_list:4 ')' '{' '}'", NODE_FuncDecl);
	inParser->PrsRegisterNT("func_decl", "\"function\" IDENTIFIER:1 '(' func_decl_arg_list:4 ')' '{' statement_list:2 '}'", NODE_FuncDecl);
	inParser->PrsRegisterNT("func_decl", "\"function\" '(' var_type:3 ')' IDENTIFIER:1 '(' func_decl_arg_list:4 ')' '{' '}'", NODE_FuncDecl);
	inParser->PrsRegisterNT("func_decl", "\"function\" '(' var_type:3 ')' IDENTIFIER:1 '(' func_decl_arg_list:4 ')' '{' statement_list:2 '}'", NODE_FuncDecl);
	inParser->PrsRegisterNT("func_decl_arg_list", "func_decl_arg", NODE_None);
	inParser->PrsRegisterNT("func_decl_arg_list", "func_decl_arg_list:1 ',' func_decl_arg:2", NODE_List);
	inParser->PrsRegisterNT("func_decl_arg", "var_type:1 IDENTIFIER:2", NODE_FuncDeclArg);
	inParser->PrsRegisterNT("var_type", "var_type_specifier", NODE_None);
	inParser->PrsRegisterNT("var_type_specifier", "\"bool\":0", NODE_VarTypeSpecBool);
	inParser->PrsRegisterNT("var_type_specifier", "\"int\":0", NODE_VarTypeSpecInt);
	inParser->PrsRegisterNT("var_type_specifier", "\"float\":0", NODE_VarTypeSpecFloat);
	inParser->PrsRegisterNT("var_type_specifier", "\"string\":0", NODE_VarTypeSpecString);
	inParser->PrsRegisterNT("primary_expr", "integer", NODE_None);
	inParser->PrsRegisterNT("primary_expr", "FLOAT", NODE_None);
	inParser->PrsRegisterNT("primary_expr", "STRING", NODE_None);
	inParser->PrsRegisterNT("primary_expr", "IDENTIFIER", NODE_None);
	inParser->PrsRegisterNT("primary_expr", "'(' expr:1 ')'", NODE_None);
	inParser->PrsRegisterNT("postfix_expr", "primary_expr", NODE_None);
	inParser->PrsRegisterNT("postfix_expr", "\"call\" IDENTIFIER:1 '(' ')'", NODE_Call);
	inParser->PrsRegisterNT("postfix_expr", "\"call\" IDENTIFIER:1 '(' argument_expr_list:2 ')'", NODE_Call);
	inParser->PrsRegisterNT("postfix_expr", "postfix_expr:1 \"++\"", NODE_PostInc);
	inParser->PrsRegisterNT("postfix_expr", "postfix_expr:1 \"--\"", NODE_PostDec);
	inParser->PrsRegisterNT("argument_expr_list", "argument_expr", NODE_None);
	inParser->PrsRegisterNT("argument_expr_list", "argument_expr_list:1 ',' argument_expr:2", NODE_List);
	inParser->PrsRegisterNT("argument_expr", "mov_expr", NODE_CallArg);
	inParser->PrsRegisterNT("unary_expr", "postfix_expr", NODE_None);
	inParser->PrsRegisterNT("unary_expr", "\"++\" unary_expr:1", NODE_PreInc);
	inParser->PrsRegisterNT("unary_expr", "\"--\" unary_expr:1", NODE_PreDec);
	inParser->PrsRegisterNT("unary_expr", "'-' cast_expr:1", NODE_Neg);
	inParser->PrsRegisterNT("unary_expr", "'~' cast_expr:1", NODE_BNot);
	inParser->PrsRegisterNT("unary_expr", "'!' cast_expr:1", NODE_LNot);
	inParser->PrsRegisterNT("cast_expr", "unary_expr", NODE_None);
	inParser->PrsRegisterNT("cast_expr", "\"cast\" '<' var_type:1 '>' '(' cast_expr:2 ')'", NODE_Cast);
	inParser->PrsRegisterNT("mul_expr", "cast_expr", NODE_None);
	inParser->PrsRegisterNT("mul_expr", "mul_expr:1 '*' cast_expr:2", NODE_Mul);
	inParser->PrsRegisterNT("mul_expr", "mul_expr:1 '/' cast_expr:2", NODE_Div);
	inParser->PrsRegisterNT("mul_expr", "mul_expr:1 '%' cast_expr:2", NODE_Mod);
	inParser->PrsRegisterNT("add_expr", "mul_expr", NODE_None);
	inParser->PrsRegisterNT("add_expr", "add_expr:1 '+' mul_expr:2", NODE_Add);
	inParser->PrsRegisterNT("add_expr", "add_expr:1 '-' mul_expr:2", NODE_Sub);
	inParser->PrsRegisterNT("shift_expr", "add_expr", NODE_None);
	inParser->PrsRegisterNT("shift_expr", "shift_expr:1 \"<<\" add_expr:2", NODE_Shl);
	inParser->PrsRegisterNT("shift_expr", "shift_expr:1 \">>\" add_expr:2", NODE_Shr);
	inParser->PrsRegisterNT("rel_expr", "shift_expr", NODE_None);
	inParser->PrsRegisterNT("rel_expr", "rel_expr:1 '<' shift_expr:2", NODE_CmpLT);
	inParser->PrsRegisterNT("rel_expr", "rel_expr:1 '>' shift_expr:2", NODE_CmpGT);
	inParser->PrsRegisterNT("rel_expr", "rel_expr:1 \"<=\" shift_expr:2", NODE_CmpLE);
	inParser->PrsRegisterNT("rel_expr", "rel_expr:1 \">=\" shift_expr:2", NODE_CmpGE);
	inParser->PrsRegisterNT("equal_expr", "rel_expr", NODE_None);
	inParser->PrsRegisterNT("equal_expr", "equal_expr:1 \"==\" rel_expr:2", NODE_CmpEQ);
	inParser->PrsRegisterNT("equal_expr", "equal_expr:1 \"!=\" rel_expr:2", NODE_CmpNE);
	inParser->PrsRegisterNT("band_expr", "equal_expr", NODE_None);
	inParser->PrsRegisterNT("band_expr", "band_expr:1 '&' equal_expr:2", NODE_BAnd);
	inParser->PrsRegisterNT("bxor_expr", "band_expr", NODE_None);
	inParser->PrsRegisterNT("bxor_expr", "bxor_expr:1 '^' band_expr:2", NODE_BXor);
	inParser->PrsRegisterNT("bor_expr", "bxor_expr", NODE_None);
	inParser->PrsRegisterNT("bor_expr", "bor_expr:1 '|' bxor_expr:2", NODE_BOr);
	inParser->PrsRegisterNT("land_expr", "bor_expr", NODE_None);
	inParser->PrsRegisterNT("land_expr", "land_expr:1 \"&&\" bor_expr:2", NODE_LAnd);
	inParser->PrsRegisterNT("lor_expr", "land_expr", NODE_None);
	inParser->PrsRegisterNT("lor_expr", "lor_expr:1 \"||\" land_expr:2", NODE_LOr);
	inParser->PrsRegisterNT("question_expr", "lor_expr", NODE_None);
	inParser->PrsRegisterNT("constant_expr", "question_expr", NODE_None);
	inParser->PrsRegisterNT("mov_expr", "question_expr", NODE_None);
	inParser->PrsRegisterNT("mov_expr", "unary_expr:1 '=' mov_expr:2", NODE_Mov);
	inParser->PrsRegisterNT("expr", "mov_expr", NODE_None);
}

void CfgCompiler::CompilerDestroy() //2066-2068
{
	delete this;
}

ICfgMachine* CfgCompiler::CompilerCreateMachineFromFile(const std::string& inFileName, std::string& outErrorString) //2071-2158
{
	class FeastClient : public ILibClient //Carbon copies of FEAST Lib functions, maybe FEAST was exclusive to CFGMachine oh the useless extra code
	{
	public:
		virtual void* LibMalloc(ulong inSize) //2077-2082
		{
			void* ptr = malloc(inSize);
			if (!ptr)
				LibError("Out of memory");
			return(ptr);
		}
		virtual void* LibRealloc(void* inPtr, ulong inNewSize)
		{
			void* ptr = realloc(inPtr, inNewSize);
			if (!ptr)
				LibError("Out of memory");
			return(ptr);
		}
		virtual void LibFree(void* inPtr)
		{
			if (inPtr)
				free(inPtr);
		}
		virtual void LibError(const char* inErrorStr)
		{
			OutputDebugStrF("FEAST Error: %s", inErrorStr); //At least this one tries to be unique
			exit(1);
		}
	};
	if (mParser != NULL)
	{
		static FeastClient sClient;
		LIB_SetClient(&sClient);
		mParser = IPrsParser::PrsCreate();
		InitGrammar(mParser);
		mParser->PrsBuild();
	}
	outErrorString = "";
	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile(inFileName, &aBuffer))
		return NULL;

	ByteVector aData = aBuffer.mData;
	aData.push_back(0);
	IPrsNode* aRoot = mParser->PrsExecute((char*)&aData[0], 4);
	if (aRoot)
	{
		ICfgMachine* aResultMachine = NULL;
		mCurFileName = inFileName;
		mMachine = new CfgMachine(this);
		ExecuteTree(aRoot);
		aResultMachine = mMachine;
		mMachine = NULL;
		aRoot->NodeDestroy();
		return aResultMachine;
	}
	else
	{
		ulong aLine, aColumn;
		std::string aStr = mParser->PrsGetLastError(&aLine, &aColumn);
		std::string aErrStr = StrFormat("%s(%d) : syntax error(%d) : %s", inFileName.c_str(), aLine, aColumn, aStr.c_str());
		outErrorString = aErrStr;
		return NULL;
	}
}

void CfgMachineValue::Init(ECfgMachineValueType inValueType) //2180-2185
{
	mType = inValueType;
	mIntValue = 0;
	mDoubleValue = 0.0;
	mStringValue = _S(""); //Confirmed by EAMT
}

CfgMachineValue::CfgMachineValue(const CfgMachineValue& inValue) //2188-2193
{
	mType = inValue.mType;
	mIntValue = inValue.mType;
	mDoubleValue = inValue.mDoubleValue;
	mStringValue = inValue.mStringValue;
}

bool CfgMachineValue::GetBoolean() //2196-2206
{
	switch (mType)
	{
	case CFGMVT_Bool:
	case CFGMVT_Int: return mIntValue != 0; break;
	case CFGMVT_Double: return mDoubleValue != 0.0; break;
	case CFGMVT_String: return mStringValue == _S("1") || mStringValue == _S("true"); break;
	default: assert(false && "Unrecognized symbol data type"); return false; //2203
	}
	return false; //?
}

int CfgMachineValue::GetInteger() //2209-2225
{
	switch (mType)
	{
	case CFGMVT_Bool: return mIntValue != 0; break;
	case CFGMVT_Int: return mIntValue; break;
	case CFGMVT_Double: return mDoubleValue; break;
	case CFGMVT_String:
		int aVal;
		if (StringToInt(mStringValue, &aVal))
			return aVal;
		else
			return 0;
		break;
	default: assert(false && "Unrecognized symbol data type"); return 0; //2222
	}
	return 0; //?
}

double CfgMachineValue::GetDouble() //2227-2244
{
	switch (mType)
	{
	case CFGMVT_Bool: return mIntValue ? 1.0 : 0.0; break;
	case CFGMVT_Int: return mIntValue; break;
	case CFGMVT_Double: return mDoubleValue; break;
	case CFGMVT_String:
		double aVal;
		if (StringToDouble(mStringValue, &aVal))
			return aVal;
		else
			return 0;
		break;
	default:
		assert(false && "Unrecognized symbol data type"); return 0.0; //2241 
	}
	return 0.0; //?
}

SexyString CfgMachineValue::GetString() //2246-2256
{
	switch (mType)
	{
		case CFGMVT_Bool: return mIntValue ? _S("true") : _S("false"); break;
		case CFGMVT_Int: return StrFormat(_S("%d"), mIntValue); break;
		case CFGMVT_Double: return StrFormat(_S("%f"), mDoubleValue); break;
		case CFGMVT_String: return mStringValue; break;
		default: assert(false && "Unrecognized symbol data type"); return _S(""); //2253
	}
	return _S("");
}

void CfgMachineValue::SetBoolean(bool inValue) //2259-2268
{
	switch (mType)
	{
		case CFGMVT_Bool: //Why
		case CFGMVT_Int: mIntValue = inValue; break;
		case CFGMVT_Double: inValue ? mDoubleValue = 1.0 : mDoubleValue = 0.0; break;
		case CFGMVT_String: inValue ? mStringValue = _S("true") : mStringValue = _S("false"); break;
		default: assert(false && "Unrecognized symbol data type"); return; //2266
	}
}

void CfgMachineValue::SetInteger(int inValue) //2270-2279
{
	switch (mType)
	{
		case CFGMVT_Bool: mIntValue = inValue != 0; break;
		case CFGMVT_Int: mIntValue = inValue; break;
		case CFGMVT_Double: mDoubleValue = inValue; break;
		case CFGMVT_String: mStringValue = StrFormat(_S("%d"), inValue); break;
		default: assert(false && "Unrecognized symbol data type"); return; //2277
	}
}

void CfgMachineValue::SetDouble(double inValue) //2281-2290
{
	switch (mType)
	{
		case CFGMVT_Bool: mIntValue = 0.0 != inValue; break;
		case CFGMVT_Int: mIntValue = inValue; break;
		case CFGMVT_Double: mDoubleValue = inValue; break;
		case CFGMVT_String: mStringValue = StrFormat(_S("%f"), inValue); break;
		default: assert(false && "Unrecognized symbol data type"); return; //2288
	}
}

void CfgMachineValue::SetString(const SexyString& inValue) //2292-2311
{
	switch (mType)
	{
		case CFGMVT_Bool: mIntValue = inValue == _S("1") || inValue == _S("true"); break;
		case CFGMVT_Int:
			if (!StringToInt(inValue, &mIntValue))
				mIntValue = 0;
			break;
		case CFGMVT_Double:
			if (!StringToDouble(inValue, &mDoubleValue))
				mDoubleValue = 0.0;
			break;
		case CFGMVT_String: mStringValue = inValue; break;
		default: assert(false && "Unrecognized symbol data type"); return; //2309
	};
}

ECfgMachineValueType CfgMachineValue::GetPromotedDataType(ECfgMachineValueType inA, ECfgMachineValueType inB) //2314-2318
{
	if (inA > inB)
		std::swap(inA, inB);
	return inB;
}

ICfgCompiler* ICfgCompiler::CompilerCreate() //2324-2326
{
	return new CfgCompiler();
}