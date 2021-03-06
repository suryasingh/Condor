// Copyright 2016 Chase Willden and The CondorLang Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef SEMANTICS_H_
#define SEMANTICS_H_

#include "condor/mem/isolate.h"
#include "condor/ast/scope.h"
#include "condor/error/error.h"
#include "condor/parser/parser.h"
#include "binary.h"
#include "internal.h"
#include "condor/assert.h"

namespace Condor {
namespace internal{

	class Parser;

	class Semantics
	{
	private: 
		Isolate* isolate;
		std::vector<Scope*> scopes;
		Scope* baseScope;
		Parser* parser;
		std::vector<ASTObject*> kThis;
		bool trace;
		int indent;
		bool inObject;
		bool isThis;
		bool staticRequired;
		bool expand;

		void RemoveThis(){if (kThis.size() > 0) kThis.erase(kThis.begin());}
		void AddThis(ASTObject* obj){kThis.insert(kThis.begin(), obj);}
		ASTObject* GetThis(){if (kThis.size() > 0) {return kThis[0];} return NULL;}

		void Trace(const char* first, const char* second);
		void Trace(const char* first, TOKEN second);
		void Expand(bool idt, ASTNode* node);
		void SetRowCol(ASTNode* node);
		void ScanScope(Scope* scope);
		void ValidateVar(ASTVar* var);
		void ValidateFor(ASTForExpr* expr);
		void ValidateBoolean(ASTBinaryExpr* expr);
		void ValidateIf(ASTIf* expr);
		void ValidateFunc(ASTFunc* func, bool parse = false, bool isConstructor = false);
		TOKEN ValidateExpr(ASTExpr* expr);
		TOKEN ValidateLiteral(ASTLiteral* lit);
		TOKEN ValidateBinary(ASTBinaryExpr* expr);
		TOKEN ValidateIdent(ASTLiteral* expr);
		TOKEN ValidateFuncCall(ASTFuncCall* expr, bool isConstructor = false);
		TOKEN ValidateArray(ASTArray* expr);
		TOKEN ValidateObjectChain(ASTBinaryExpr* expr);
		void ExpectNumber(ASTLiteral* lit);
		void ValidateObject(ASTObject* obj);
		void ValidateObjectInit(ASTVar* var);
		void ValidateExtend(ASTObject* base, ASTObject* extend);
		ASTObject* GetObject(ASTNode* node);
		ASTObject* GetObjectFromScope(Scope* scope);
		TOKEN ValidateCast(ASTExpr* expr);
		void OpenScope(Scope* scope){scopes.insert(scopes.begin(), scope);}
		void CloseScope(){scopes.erase(scopes.begin());}
		Scope* GetCurrentScope();
		Scope* GetPreviousScope();
		void SwapScopes();
		void ValidateBaseAndAssignment(ASTVar* var);
		TOKEN ValidateWhile(ASTWhileExpr* expr);
		Scope* Parse(Scope* scope);
		TOKEN ValidateInternal(ASTFuncCall* call);
		void ValidateSwitch(ASTSwitch* expr);
		void ValidateCase(ASTCase* expr);
		std::string GetBaseName(ASTExpr* expr);
		TOKEN GetCastType(ASTNode* node);

	public:
		Semantics(Isolate* iso);
		~Semantics();
		static Semantics* New(Isolate* iso, Parser* parser);
		int row;
		int col;
		void Evaluate(Scope* scope);
		std::string* GetSource();
		Scope* ParseAndScan(Scope* scope);
		
	};

} // namespace internal
}

#endif // SEMANTICS_H_
