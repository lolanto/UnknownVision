#pragma once

#include <InfoLog/InfoLog.h>
#include <GraphicsInterface/Shader.h>
#include <functional>

/**
 * 替换Shader代码中的占位符字符串
 * @param code 要修改的Shader代码
 * @param from 要替换的占位符字符串
 * @param to 替换后的Shader代码
 * @return 如果成功替换返回true，否则返回false
 */
static bool ReplaceShaderSourceCode(std::string& code, const char* from, const char* to) {
	size_t start_pos = code.find(from);
	if (start_pos == std::string::npos)
	{
		LOG_ERROR("Cannot find the placement string %s in shader code", from);
		return false;
	}
	code.replace(start_pos, strlen(from), to);
	return true;
}

template<typename ConcreateParameter>
class TShaderParameterSlot {
public:
	constexpr static size_t GetBindingSlotIndex(size_t offset = 0) {
		return ConcreateParameter
			::GetBindingSlotIndex(offset);
	}
	constexpr static const char* GetShaderCodePlacementString() {
		return ConcreateParameter
			::ShaderCodePlacementString;
	}
	static const char* GetShaderCode() {
		return ConcreateParameter
			::ShaderCodeOfBufferDeclaration;
	}
	static std::vector<UnknownVision::ShaderParameterSlotDesc> GetSlotDesc() { return ConcreateParameter::GetSlotDesc(); }
};


template<typename... TArgs>
struct TShaderParameterSlotTrait
{
	/** 
	 * 当前ShaderParameterSlotTrait包含的所有参数类型的总数量
	 */
	static constexpr size_t TotalShaderParameterSlot = sizeof...(TArgs);

	/**
	 * 获取所有Parameter共同组成的Slot描述符号
	 */
	static std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetSlotDesc() {
		return { TArgs::GetSlotDesc()... };
	}

	/** 
	 * 遍历所有的类型，并分别调用回调函数，用以进行一系列的Shader代码替换操作
	 * @param callback 回调函数，接受两个参数，第一个是Shader代码的占位符字符串，第二个是实际的Shader代码
	 */
	static void LoopShaderModificationWithCallback(const std::function<void(const char*, const char*)>& callback) {
		// powered by AI....
		//(std::apply([&](const auto&... args) {
		//	((callback(args::GetShaderCodePlacementString(), args::GetShaderCode())), ...);
		//	}, std::tuple<TArgs...>()));

		(void)std::initializer_list<int>{
			(callback(TArgs::GetShaderCodePlacementString(), TArgs::GetShaderCode()), 0)...
		};
	}

	/**
	 * 获取指定类型的ShaderParameterSlot的绑定槽索引
	 * @param offset 偏移量，一般默认为0。假如非VertexShader，就考虑Pipeline前其它Shader所占用的槽位
	 */
	template<typename TargetParameterType>
	static constexpr size_t GetBindingSlotIndex(size_t offset = 0) {
		// check if TArgs include TargetParameterType
		static_assert((std::is_base_of_v<TShaderParameterSlot<TArgs>, TargetParameterType> || ...),
			"TargetParameterType must be one of the TArgs types");
		return TargetParameterType::GetBindingSlotIndex(offset);
	}

	/**
	 * 获取指定索引的Slot类型
	 */
	template<size_t Index>
	struct SlotType {
		using Type = std::tuple_element_t<Index, std::tuple<TArgs...>>;
	};
};
