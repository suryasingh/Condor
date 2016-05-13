// Copyright 2016 Chase Willden and The CondorLang Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "script.h"
#include <stdio.h> // printf doesn't work without this

namespace Condor {
namespace internal{

	Script* Script::New(Context* context, String* str){
		Isolate* iso = context->GetIsolate();
		void* p = (Script*) iso->GetMemory(sizeof(Script));
		Script* script = new(p) Script(iso, str, context);
		return script;
	}

	Script::Script(Isolate* isolate, String* str, Context* con){
		source = str;
		context = con;
		hasErr = false;
		this->isolate = isolate;
		compiled = false;
		parser = NULL;
		parsingTime = PARSING_TIME;
		compileTime = COMPILE_TIME;
		executionTime = EXECUTION_TIME;
		sourceCode = "";
		internal = str->IsInternal();
		sourceCode += str->GetValue();
		absolutePath = str->GetPath();
		if (absolutePath == "inline"){
			name = str->name;
		}
		else{
			name = absolutePath;
		}
		semantics = NULL;
		currentCode = NULL;
	}

	void Script::RunInternalScript(Isolate* isolate, std::string hex, std::string _name, std::string sub, bool isHex){
		if (isolate->GetContext()->IsIncluded(_name)) return;
		Condor::Isolate* iso = CAST(Condor::Isolate*, isolate);
		int len = hex.length();
		std::string newString;
		if (isHex){
			for (int i = 0; i < len; i += 2){
			    std::string byte = hex.substr(i, 2);
			    char chr = (char) (int) strtol(byte.c_str(), NULL, 16);
			    newString.push_back(chr);
			}
		}
		else{
			newString = hex;
		}
		Condor::String* str = Condor::String::New(iso, newString.c_str());
		String* iStr = CAST(String*, str);
		iStr->name = _name;
		iStr->SetInternal();

		Condor::Context* c = CAST(Condor::Context*, isolate->GetContext());
		Condor::Script* script = Condor::Script::Compile(c, str);
		if (!script->HasError()){
			Script* s = CAST(Script*, script);
			s->SetSub(sub);
			script->Run();
		}
	}

	void Script::Compile(){
		if (context->IsIncluded(name)) return;
		parser = Parser::New(isolate, &sourceCode);
		bool isInline = absolutePath == "inline";
		parser->SetInteral(internal);
		parser->SetInline(isInline);
		Clock* clock = NULL;
		Clock* compileClock = NULL;
		try {
			if (parsingTime) {
				clock = new Clock;
				clock->Start();
			}
			if (compileTime) {
				compileClock = new Clock;
				compileClock->Start();
			}

			parser->Parse();

			if (parsingTime) {
				clock->Stop();
				if (!parser->IsInternal()) 
					printf("Parsing:  %f sec | %s\n", clock->GetDuration(), absolutePath.c_str());
			}
			if (absolutePath != "inline"){
				context->AddToInProgress(absolutePath);
			}
			else{
				context->AddToInProgress(name);
			}
			LoadImports();
			LoadIncludes();
		}
		catch (Error::ERROR e){
			std::string msg = Error::String(e, parser->expected);
			currentCode = parser->GetSource();
			msg = std::to_string(parser->Row) + ":" + std::to_string(parser->Col) + " - " + msg + " - \n\t" + absolutePath.c_str() + "\n\n" + GetSourceRow(parser->Row, parser->Col);
			msgs.push_back(msg);
			printf("\nParser Error: \n%s\n", msg.c_str());
			hasErr = true;
			return;
		}

		try {
			semantics = Semantics::New(isolate, parser);
			semantics->Evaluate(parser->GetBaseScope());

			if (compileTime){
				compileClock->Stop();
				if (!parser->IsInternal()) 
					printf("Compile:  %f sec | %s\n", compileClock->GetDuration(), absolutePath.c_str());
			}
		}
		catch (Error::ERROR e){
			std::string msg = Error::String(e, NULL);
			currentCode = semantics->GetSource();
			msg = std::to_string(semantics->row) + ":" + std::to_string(semantics->col) + " - " + msg + " - \n\t" + absolutePath.c_str() + "\n\n" + GetSourceRow(semantics->row, semantics->col);
			printf("\nSemantic Error: \n%s\n", msg.c_str());
			msgs.push_back(msg);
			hasErr = true;
			return;
		}

		Scope* base = parser->GetBaseScope();
		base->name = name;
		context->AddScope(base);
	}

	void Script::Run(){
		if (hasErr) return;
		if (subModule.length() > 0) {
			if (subModule != "*"){
				Scope* s = parser->GetBaseScope();
				for (int i = 0; i < s->Size(); i++){
					if (s->Get(i)->name == subModule) s->Get(i)->isExport = true;
					else s->Get(i)->isExport = false;
				}
			}
		}
		executor = Execute::New(isolate, parser->GetBaseScope());
		executor->semantic = semantics;
		Clock* clock = NULL;

		try {
			if (executionTime) {
				clock = new Clock;
				clock->Start();
			}
			executor->Evaluate();
			if (executionTime) {
				clock->Stop();
				if (!parser->IsInternal())
					printf("Execution:  %f sec | %s\n", clock->GetDuration(), absolutePath.c_str());
			}
		}
		catch (Error::ERROR e){
			std::string msg = Error::String(e, NULL);
			std::string src = executor->GetSource();
			currentCode = &src;
			msg = std::to_string(executor->row) + ":" + std::to_string(executor->col) + " - " + msg + " - \n\t" + absolutePath.c_str() + "\n\n" + GetSourceRow(executor->row, executor->col);
			printf("\nRuntime Error: \n%s\n", msg.c_str());
			msgs.push_back(msg);
			return;
		}
	}

	std::string Script::GetSourceRow(int row, int col){
		std::string src = "";
		src += currentCode->c_str();
		std::string result = "";
		std::string u = UNDERLINE_START;
		std::string u2 = UNDERLINE_STOP;
		int r = 1;
		int c = 1;
		bool carrot = false;
		for (int i = 0; i < src.length(); i++){
			if (src[i] == '\t') src[i] = ' ';
			if (r == row - 1 || r == row){
				result += src[i];
			}
			else if (r > row) break;
			if (src[i] == '\n'){
				r++;
				c = 1;
			}
			c++;
		}
		result += '\n';
		for (int i = 0; i < col; i++){
			result += ' ';
		}
		result += "\033[1;32m^\033[0m";
		result += "\n";
		return result;
	}

	void Script::LoadImports(){
		// Load app by default
		std::string libsDir = Path::GetLibDir();
		if (!App::Included){
			App::Included = true;
			std::string path = libsDir + "app.cb";
			RunInternalScript(isolate, FS::ReadFile(path), "app", "", false);
		}

		for (int i = 0; i < parser->imports.size(); i++){
			ASTImport* import = parser->imports[i];
			std::vector<std::string> splits = String::Split(import->name, '.');
			std::string name = splits[0];
			std::string sub = "";
			if (splits.size() > 1) sub = splits[1];
			if (sub.length() > 0) {
				parser->Row = import->row;
				parser->Col = import->col;
				throw Error::NOT_IMPLEMENTED;
			}

			std::string path = libsDir + name + ".cb";
			if (!FS::FileExists(path)){
				parser->Row = import->row;
				parser->Col = import->col;
				throw Error::INVALID_IMPORT;
			}
			else{
				RunInternalScript(isolate, FS::ReadFile(path), name, sub, false);
			}
		}
	}

	void Script::LoadIncludes(){
		if (absolutePath == "inline") return; // can't include inline
		Path* path = Path::New(isolate);
		path->SetBase(absolutePath);

		for (int i = 0; i < parser->includes.size(); i++){
			ASTInclude* include = parser->includes[i];
			std::string c = path->GetFromBase(include->name);

			Condor::Isolate* iso = CAST(Condor::Isolate*, isolate);
			Condor::Context* ctxt = CAST(Condor::Context*, context);
			Condor::String* str = Condor::String::NewFromFile(iso, c.c_str());
			Condor::Script* script = Condor::Script::Compile(ctxt, str);
			script->Run();
		}
	}

} // namespace internal
}