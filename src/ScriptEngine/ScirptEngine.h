#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H

namespace UnknownVision {
	class ScriptEnegine {
	public:
		ScriptEnegine() = default;
		virtual ~ScriptEnegine() = default;
	public:
		virtual void AnalyseScript(const char* script) = 0;
		virtual bool Initialize() { return false; }
	};
}

#endif // SCRIPT_ENGINE_H
