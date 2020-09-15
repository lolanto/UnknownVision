#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <cstdint>
#include <vector>

/** 命令基类，负责提供调用接口()，以及偏移 */
class BaseCommand {
public:
	BaseCommand(uint32_t commandSize)
		: CommandSize(commandSize) {}
	virtual ~BaseCommand() = default;
	/** 命令调用接口，不同的指令对象有不同的实现 */
	virtual void operator()(void* par1) = 0;
	/** 该指令对象在内存中的大小，单位字节 */
	const uint32_t CommandSize;
};

/** 专门存储各种指令
 * @remark 暂不支持指令删除 */
class CommandList {
public:
	CommandList() : m_byteOffset(0), m_commandPointer(nullptr) {}
	/** 需要显式调用所有存储的指令的析构函数 */
	~CommandList() {
		if (m_commandBuffer.size() == 0) return; /**< 假如缓冲没有指令，直接结束 */
		uint32_t byteOffset = 0;
		uint8_t* cmdPtr = m_commandBuffer.data();
		while (byteOffset < m_commandBuffer.size()) {
			BaseCommand* temp = reinterpret_cast<BaseCommand*>(cmdPtr);
			cmdPtr += temp->CommandSize; /**< 获得下一个指令地址 */
			byteOffset += temp->CommandSize; /**< 计算总偏移量，防止出界 */
			temp->~BaseCommand();
		}
	}
	/** 向指令缓冲中添加新的指令
	 * @param cmd 待插入的指令 */
	void InsertCommand(BaseCommand* cmd) {
		m_commandBuffer.resize(m_commandBuffer.size() + cmd->CommandSize);
		uint8_t* dest = (&m_commandBuffer.back()) - cmd->CommandSize + 1; /**< 留意偏移的值为(CommandSize - 1) 因为back本身已经占了1个字节 */
		memcpy(dest, cmd, cmd->CommandSize);
	}
	/** 清空指令缓存 */
	void ClearCommandBuffer() { m_commandBuffer.clear(); }
	/** 重置当前执行的指令指针位置到最开始的指令
	 * @remark 在执行过程中若插入新指令，则当前指令指针失效
	 * 需要调用该函数进行重置 */
	void ResetCommandPointer() { 
		m_byteOffset = 0;
		m_commandPointer = m_commandBuffer.data();
	}
	/** 执行指令缓存中的所有指令
	 * @param para1 执行的指令的第一个参数
	 * @return 若当前缓冲中还有指令则返回true，否则返回false，
	 * 并重置指令指针到第一条指令处 */
	bool Execute(void* para1) {
		if (m_commandPointer == nullptr) m_commandPointer = m_commandBuffer.data();
		if (m_commandBuffer.size() == 0) return false; /**< 当前没有指令 */
		BaseCommand* cmd = reinterpret_cast<BaseCommand*>(m_commandPointer);
		cmd->operator()(para1);
		m_byteOffset += cmd->CommandSize;
		m_commandPointer += cmd->CommandSize;
		if (m_byteOffset < m_commandBuffer.size()) { /**< 还有指令在缓冲中 */
			return true;
		}
		else { /**< 缓冲没有指令 */
			m_byteOffset = 0;
			m_commandPointer = m_commandBuffer.data();
			return false;
		}
	}
private:
	uint32_t m_byteOffset;
	std::vector<uint8_t> m_commandBuffer;
	uint8_t* m_commandPointer;
};

#endif // COMMAND_BUFFER_H
