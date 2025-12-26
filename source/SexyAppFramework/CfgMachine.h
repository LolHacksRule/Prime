#ifndef __CFGMACHINE_H__
#define __CFGMACHINE_H__

#include "Common.h"
#include "Feast.h"

//Present in EAMT, Windows and Mac, appears in iOS BejClassic < 2.4.0 PVZ fsr

using namespace FEAST;

namespace Sexy
{
    enum ECfgMachineValueType
    {
        CFGMVT_None,
        CFGMVT_Bool,
        CFGMVT_Int,
        CFGMVT_Double,
        CFGMVT_String,
        CFGMVT_COUNT
    };
    class CfgMachineValue
    {
    private:
        ECfgMachineValueType mType;
        int mIntValue;
        double mDoubleValue;
        SexyString mStringValue;
        void Init(ECfgMachineValueType inValueType);
    public:
        CfgMachineValue(const SexyString&);
        CfgMachineValue(double);
        CfgMachineValue(int);
        CfgMachineValue(bool);
        CfgMachineValue(const CfgMachineValue& inValue);
        CfgMachineValue(ECfgMachineValueType inValueType) { Init(inValueType); } //53
        //~CfgMachineValue();
        ECfgMachineValueType GetType() { return mType; } //61
        void SetType(ECfgMachineValueType inType) { mType = inType; } //62
        bool GetBoolean();
        int GetInteger();
        double GetDouble();
        SexyString GetString();
        void SetBoolean(bool inValue);
        void SetInteger(int inValue);
        void SetDouble(double inValue);
        void SetString(const SexyString& inValue);

        static ECfgMachineValueType GetPromotedDataType(ECfgMachineValueType inA, ECfgMachineValueType inB);
    };
    class ICfgMachine
    {
    public:
        virtual void MachineDestroy() = 0;
        virtual bool MachineExecuteFunction(const char* inFunctionName, CfgMachineValue* outReturnValue) = 0;
        virtual bool MachineDisassembleToFile(const std::string& inFileName) = 0;
    };
    class ICfgCompiler
    {
    public:
        virtual void CompilerDestroy() = 0;
        virtual ICfgMachine* CompilerCreateMachineFromFile(const std::string& inFileName, std::string& outErrorString) = 0;
        static ICfgCompiler* CompilerCreate();
    };
    class CfgCompiler : public ICfgCompiler
    {
    public:
        class FeastException : public FEAST::XLibException
        {
        protected:
            std::string mText;
        public:
            FeastException(const std::string& inText);
            const char* LibExceptionGetText();
        };
        enum EToken
        {
            TOKEN_None,
            TOKEN_Identifier,
            TOKEN_String,
            TOKEN_Character,
            TOKEN_Float,
            TOKEN_DecInteger,
            TOKEN_HexInteger,
            TOKEN_BinInteger,
            TOKEN_OctInteger,
            TOKEN_BlockComment,
            TOKEN_COUNT
        };
        enum ENode
        {
            NODE_None,
            NODE_TermIdentifier,
            NODE_TermString,
            NODE_TermCharacter,
            NODE_TermFloat,
            NODE_TermDecInteger,
            NODE_TermHexInteger,
            NODE_List,
            NODE_StmtEmpty,
            NODE_StmtExpr,
            NODE_StmtCompound,
            NODE_SelIf,
            NODE_VarDecl,
            NODE_PropertyDecl,
            NODE_VarDeclIdent,
            NODE_VarTypeSpecBool,
            NODE_VarTypeSpecInt,
            NODE_VarTypeSpecFloat,
            NODE_VarTypeSpecString,
            NODE_FuncDecl,
            NODE_FuncDeclArg,
            NODE_PostInc,
            NODE_PostDec,
            NODE_PreInc,
            NODE_PreDec,
            NODE_Neg,
            NODE_BNot,
            NODE_LNot,
            NODE_Cast,
            NODE_Mul,
            NODE_Div,
            NODE_Mod,
            NODE_Add,
            NODE_Sub,
            NODE_Shl,
            NODE_Shr,
            NODE_CmpLT,
            NODE_CmpGT,
            NODE_CmpLE,
            NODE_CmpGE,
            NODE_CmpEQ,
            NODE_CmpNE,
            NODE_BAnd,
            NODE_BXor,
            NODE_BOr,
            NODE_LAnd,
            NODE_LOr,
            NODE_Mov,
            NODE_MulMov,
            NODE_DivMov,
            NODE_ModMov,
            NODE_AddMov,
            NODE_SubMov,
            NODE_ShlMov,
            NODE_ShrMov,
            NODE_BAndMov,
            NODE_BXorMov,
            NODE_BOrMov,
            NODE_Log,
            NODE_Call,
            NODE_CallArg,
            NODE_Return,
            NODE_COUNT
        };
        enum ESymbolType
        {
            SYMBOL_None,
            SYMBOL_Constant,
            SYMBOL_Var,
            SYMBOL_Property,
            SYMBOL_COUNT
        };
        class CfgScope
        {
        private:
            CfgCompiler* mCompiler;
            typedef std::map<std::string, CfgSymbol*> CfgStringToSymbolMap;
            CfgStringToSymbolMap mSymbolMap;
            std::vector<CfgSymbol*> mSymbols;
            int mTempSymbolIndex;
        public:
            int mFirstOpIndex;
            CfgScope* mParentScope;
            int mScopeIndex;
            bool mIsFunctionScope;
            std::vector<CfgSymbol*> mFuncArgSymbols;
            ECfgMachineValueType mFuncReturnType;
            CfgScope(CfgCompiler* inCompiler, CfgScope* inParentScope, int inScopeIndex, bool inIsFunctionScope);
            ~CfgScope();
            CfgSymbol* GetSymbol(SLexToken* inToken, bool inRequired);
            CfgSymbol* GetSymbol(const std::string& inName);
            CfgSymbol* AddSymbol(SLexToken* inToken, ESymbolType inSymbolType, ECfgMachineValueType inDataType);
            CfgSymbol* AddTempSymbol(ESymbolType inSymbolType, ECfgMachineValueType inDataType);
            CfgScope* GetNearestFunctionScope();
        };
        class CfgSymbol
        {
        private:
            ESymbolType mSymbolType;
            CfgMachineValue mValue;
        public:
            std::string mName;
            int mIndex;
            CfgScope* mScope;
            CfgSymbol(CfgScope* inScope, ESymbolType inSymbolType, ECfgMachineValueType inDataType);
            ~CfgSymbol();
        private:
            void ReadProperty();
            void WriteProperty();
        public:
            ESymbolType GetSymbolType();
            CfgMachineValue GetValue(CfgMachineValue inValue);
            void SetValue(const CfgMachineValue& inValue);
        };
        class CfgAttribute : public FEAST::IPrsAttr
        {
        public:
            ECfgMachineValueType mValueType;
            CfgSymbol* mWriteableSymbol;
            CfgAttribute();
            virtual void AttrDestroy();
        };
        enum EOpCode
        {
            OP_Nop,
            OP_End,
            OP_Call,
            OP_Ret,
            OP_Load,
            OP_Store,
            OP_Copy,
            OP_Pop,
            OP_Clear,
            OP_Jmp,
            OP_Jt,
            OP_Jf,
            OP_Inc,
            OP_Dec,
            OP_Neg,
            OP_BNot,
            OP_LNot,
            OP_CastBool,
            OP_CastInt,
            OP_CastFloat,
            OP_CastString,
            OP_Mul,
            OP_Div,
            OP_Mod,
            OP_Add,
            OP_Sub,
            OP_Shl,
            OP_Shr,
            OP_CmpLT,
            OP_CmpGT,
            OP_CmpLE,
            OP_CmpGE,
            OP_CmpEQ,
            OP_CmpNE,
            OP_BAnd,
            OP_BXor,
            OP_BOr,
            OP_Log
        };
    public:
        void ThrowError(const std::string inError, int inLine = 0, int inColumn = 0);
        void ThrowTokenError(SLexToken* inToken, const std::string inError);
        void ThrowNodeError(IPrsNode* inNode, const std::string inError);
        static std::string GetTokenString(SLexToken* inToken);
        class CfgMachine : public ICfgMachine
        {
        private:
            CfgCompiler* mCompiler;
            std::vector<CfgScope*> mScopes;
            bool mBlockEmit;
            typedef std::map<std::string, CfgScope*> CfgStringToScopeMap;
            CfgStringToScopeMap mFunctionScopeMap;
            CfgScope* mGlobalScope;
            CfgScope* mCurrentScope;
        public:
            std::vector<ulong> mOps;
            CfgMachine(CfgCompiler* inCompiler);
            ~CfgMachine();
            CfgScope* GetGlobalScope();
            CfgScope* GetCurrentScope();
            CfgScope* GetFunctionScope(const std::string& inFunctionName);
            CfgScope* PushFunctionScope(FEAST::SLexToken* inToken);
            CfgScope* PushScope();
            void PopScope(); //Heh popcapscope
            void Emit(ulong inOp);
            ulong EmitFixup();
            void SetFixup(ulong inIndex);
            void BlockEmit(bool inBlock);
            bool DisassembleToFile(const std::string& inFileName);
            void Execute(CfgScope* inScope, CfgMachineValue* outReturnValue);
            void MachineDestroy();
            bool MachineExecuteFunction(const char* inFunctionName, CfgMachineValue* outReturnValue);
            bool MachineDisassembleToFile(const std::string& inFileName);
        };
        IPrsParser* mParser;
        std::string mCurFileName;
        CfgMachine* mMachine;
        CfgCompiler();
        ~CfgCompiler();
        void ExecuteTree(IPrsNode* inRoot);
        void InitGrammar(IPrsParser* inParser);
        void CompilerDestroy();
        ICfgMachine* CompilerCreateMachineFromFile(const std::string& inFileName, std::string& outErrorString);
    };
}

#endif //__CFGMACHINE_H__