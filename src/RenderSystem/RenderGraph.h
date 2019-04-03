#ifndef UV_RENDER_GRAPH_H
#define UV_RENDER_GRAPH_H

#include "RenderSystemConfig.h"

#include <vector>
#include <set>
#include <map>
#include <string>
#include <cstdint>
#include <list>

namespace UnknownVision {

	class Pass;
	class ResourceRecordContainer;
	struct PassRawData;
	struct ResourceRawOperation;

	struct ResourceOperationInfo {
		ResourceOperationInfo() : container(nullptr) {}
		ResourceOperationInfo(const ResourceOperationInfo&) = delete;
		ResourceOperationInfo(ResourceOperationInfo&& rhs)
			: type(rhs.type), usage(rhs.usage), width(rhs.width), height(rhs.height),
			depth(rhs.depth), container(rhs.container) {
			initData.swap(rhs.initData);
		}
		uint64_t hashHelper() const {
			uint64_t hashValue = reinterpret_cast<uintptr_t>(container);
			return hashValue;
		}

		bool operator<(const ResourceOperationInfo& rhs) const {
			return hashHelper() < rhs.hashHelper();
		}
		ResourceType type; /**< 资源的大致类型 */
		ResourceUsage usage; /**< 资源的使用情况(/可以被如何使用) */
		uint32_t width, height, depth;
		std::vector<uint8_t> initData; /**< 可选! 资源的初始数据 */
		ResourceRecordContainer* container;
	};

	struct ResourceRecordNode {
		ResourceRecordNode(ResourceRecordType type,
			Pass* pass = nullptr,
			ResourceOperationInfo* info = nullptr,
			ResourceRecordNode* next = nullptr,
			ResourceRecordNode* last = nullptr) : type(type), pass(pass), info(info), next(next), last(last) {}

		Pass* Remove();

		ResourceRecordNode* last;
		ResourceRecordNode* next;
		const ResourceOperationInfo* info;
		Pass* pass;
		ResourceRecordType type;
	};

	class ResourceRecordContainer {
	public:
		ResourceRecordContainer(const char* name, const bool isTransient)
			: ResourceRecordContainer(isTransient) { m_name = name; }
		~ResourceRecordContainer() { CleanUpNodes(); }
		ResourceRecordContainer(ResourceRecordContainer&& rhs)
			: ResourceRecordContainer(rhs.m_isTransient) { moveHelper(std::move(rhs)); }
		const ResourceRecordContainer& operator=(ResourceRecordContainer&& rhs) {
			moveHelper(std::move(rhs));
			return *this;
		}
		const ResourceRecordContainer& operator=(const ResourceRecordContainer& rhs) = delete;
		ResourceRecordContainer(const ResourceRecordContainer& rhs) = delete;

	public:
		/** 向容器中插入新的操作信息
		 * @param type 需要插入的记录类型
		 * @param pass 产生操作的pass
		 * @param opInfo 操作记录的具体信息 
		 * @return 假如记录类型是permanent或create则返回的是conainter本身，否则返回RecordNode*/
		void* Push(ResourceRecordType type, Pass* pass, ResourceOperationInfo&& opInfo);
		void CleanUpNodes();
		bool IsNecessary();
		bool IsTransient() const { return m_isTransient; }
	private:
	private:
		ResourceRecordContainer(const bool isTransient) 
			: m_head(nullptr), m_creator(nullptr), m_permanentInfo(nullptr), m_isTransient(isTransient) {}
		void moveHelper(ResourceRecordContainer&& rhs);
	private:
		const bool m_isTransient; /**< 该资源是否短暂存在 */
		std::string m_name; /**< 资源的名称 */
		ResourceRecordNode* m_head; /**< 资源的操作记录链表 */
		union {
			Pass* m_creator; /**< 资源创建者 */
			const ResourceOperationInfo* m_permanentInfo; /**< 资源长期存在，没有创建者，只有初始状态 */
		};
		/** value一旦放入set中则不可以再被修改 */
		std::set<ResourceOperationInfo> m_opInfo; /**< 资源的具体使用方式 */
	};

	class Pass {
	public:
		Pass(const char* name)
			: m_name(name) {}
		Pass(Pass&& rhs) { moveHelper(std::move(rhs)); }
		const Pass& operator=(Pass&& rhs) { moveHelper(std::move(rhs)); return *this; }
	public:
		void Read(ResourceRecordNode* resource) { m_readRecords.push_back(resource); }
		void Write(ResourceRecordNode* resource) { m_writeRecords.push_back(resource); }
		void Create(ResourceRecordContainer* resource) { m_createRecords.push_back(resource); }
		void RemoveCreateRecord(ResourceRecordContainer* record) {
			for (decltype(m_createRecords)::iterator iter = m_createRecords.begin();
				iter != m_createRecords.end(); ++iter) {
				if (*iter == record) {
					m_createRecords.erase(iter);
					break;
				}
			}
		}
		bool IsNecessary() {
			return m_readRecords.size() + m_writeRecords.size() + m_createRecords.size();
		}
	public:
		const std::vector<ResourceRecordNode*>& ReadRecords() const { return m_readRecords; }
		const std::vector<ResourceRecordNode*>& WriteRecords() const { return m_writeRecords; }
		std::set<Pass*> CleanUpReadAndWriteRecords();
	private:
		void moveHelper(Pass&& rhs);
	private:
		std::string m_name;
		std::vector<ResourceRecordNode*> m_readRecords;
		std::vector<ResourceRecordNode*> m_writeRecords;
		std::vector<ResourceRecordContainer*> m_createRecords;
	};

	class Group {
	public:
		Group() = default;
		Group(Group&& rhs) {
			m_passes.swap(rhs.m_passes);
			m_resources.swap(rhs.m_resources);
		}
		Group(const Group&) = delete;
		const Group& operator=(Group&& rhs) {
			m_passes.swap(rhs.m_passes);
			m_resources.swap(rhs.m_resources);
			return *this;
		}
		const Group& operator=(const Group&) = delete;
	public:
		bool Push(const ResourceRawOperation* resources, uint32_t size);
		bool Push(const PassRawData* passes, uint32_t size);
		void SortOut();

	private:
		ResourceOperationInfo constructResourceOperationInfo (const ResourceRawOperation* res);
	private:
		std::list<Pass> m_passes;
		std::map<std::string, ResourceRecordContainer> m_resources;
	};
}

#endif // UV_RENDER_GRAPH_H
