#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <map>
#include "cobra/mem/isolate.h"
#include "cobra/ast/ast.h"
#include "cobra/types/try_catch/exception.h"
#include <vector>
#include <algorithm>
#include "cobra/types/array/array.h"

namespace Cobra {
namespace internal{

	class Isolate;
	class Script;
	
	class Context
	{
	private:
		std::map<Isolate*, std::map<size_t, Script*> > scripts;
		std::vector<std::string> inProgress;
		std::vector<std::string> imported;
	public:
		Context();
		~Context();
		void SetIsolate(Isolate* isolate);
		void AddScript(Script* script);
		Script* GetScriptByString(Isolate* iso, std::string code);
		bool IsIncluded(Isolate* iso, const char* path);
		bool IsImported(Isolate* iso, std::string name);
		void SetImport(std::string name);
		void AddToInProgress(std::string str);
		void RemoveFromInProgress(std::string str);
		ASTNode* GetExportedNode(Isolate* iso, std::string name);
		void PrintExported(Isolate* iso);
	};

} // namespace internal
}

#endif // CONTEXT_H_
