#ifndef UV_RENDER_GRAPH_H
#define UV_RENDER_GRAPH_H

#include "RenderSystemConfig.h"
#include "Order.h"

#include <vector>
#include <set>
#include <map>
#include <string>
#include <cstdint>
#include <list>

namespace UnknownVision {

	class PassRecordContainer;
	class ResourceRecordContainer;
	struct PassRawData;
	struct ResourceRawOperation;

	const uint8_t NUMBER_OF_GRAPHIC_QUEUE = 1;
	const uint8_t NUMBER_OF_COMPUTE_QUEUE = 1;
	const uint8_t NUMBER_OF_COPY_QUEUE = 1;

	struct ResourceOperationInfo {
		using ResourceOptInfoHash = uint64_t;
		ResourceOperationInfo(ResourceUsage usage,
			uint32_t width, uint32_t height, uint32_t depth): usage(usage),
			width(width), height(height), depth(depth), container(nullptr) {}
		ResourceOperationInfo(const ResourceOperationInfo&) = delete;
		ResourceOperationInfo(ResourceOperationInfo&& rhs)
			: usage(rhs.usage), width(rhs.width), height(rhs.height),
			depth(rhs.depth), container(rhs.container) {
			initData.swap(rhs.initData);
		}
		ResourceOptInfoHash Hash() const {
			ResourceOptInfoHash hashValue = reinterpret_cast<uintptr_t>(container);
			return hashValue;
		}

		/** 返回该操作的基本信息
		 * @return 具体查看 ResourceUsageBriefInfo的定义
		 * @remark 该函数必须在对应的资源操作记录容器已经产生订单后方可调用
		 * 因为该方法依赖资源操作记录的哈希值 */
		ResourceUsageBriefInfo BriefInfo() const {
			ResourceUsageBriefInfo info;
			info.type = container->GetType();
			info.hash = container->GetHash();
			info.usage = usage;
			return info;
		}

		bool operator<(const ResourceOperationInfo& rhs) const {
			return Hash() < rhs.Hash();
		}
		ResourceUsage usage; /**< 资源的使用情况(/可以被如何使用) */
		uint32_t width, height, depth;
		std::vector<uint8_t> initData; /**< 可选! 资源的初始数据 */
		ResourceRecordContainer* container; /**< 指向该记录的容器 */
	};

	struct ResourceRecordNode {
		ResourceRecordNode(ResourceRecordType type,
			PassRecordContainer* pass = nullptr,
			ResourceOperationInfo* info = nullptr,
			ResourceRecordNode* next = nullptr,
			ResourceRecordNode* last = nullptr) : type(type), pass(pass), info(info), next(next), last(last) {}

		PassRecordContainer* Remove();

		ResourceRecordNode* last;
		ResourceRecordNode* next;
		const ResourceOperationInfo* info;
		PassRecordContainer* pass;
		ResourceRecordType type; /**< 该操作是读/写/创建 */
	};

	class ResourceRecordContainer {
	public:
		ResourceRecordContainer(const char* name, ResourceType type)
			: ResourceRecordContainer(type) { m_name = name; }
		~ResourceRecordContainer() { CleanUpNodes(); }
		ResourceRecordContainer(ResourceRecordContainer&& rhs)
			: ResourceRecordContainer(rhs.m_type) { moveHelper(std::move(rhs)); }
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
		void* Push(ResourceRecordType type, PassRecordContainer* pass, ResourceOperationInfo&& opInfo);
		void CleanUpNodes();
		bool IsNecessary();
		bool IsTransient() const { return m_type & RESOURCE_TYPE_TRANSIENT; }
		/** 根据自身记录，创建自身的 资源创建订单 */
		ResourceOrder GenOrder() const;
		/** 返回该资源记录容器所生成的 资源创建订单 的哈希码 */
		OrderHash GetHash() const { return m_hash; }
		/** 返回资源的大致类型 */
		ResourceType GetType() const { return m_type; }
		/** 返回第一条操作记录
		 * @return 返回指向第一条操作记录的指针，若该记录不存在，则返回null*/
		const ResourceRecordNode* GetFirstRecord() const { return m_head; }
	private:
		ResourceRecordContainer(ResourceType type) 
			: m_type(type), m_head(nullptr), m_creator(nullptr), m_permanentInfo(nullptr) {}
		void moveHelper(ResourceRecordContainer&& rhs);
	private:
		ResourceType m_type; /**< 资源的大致类型 */
		std::string m_name; /**< 资源的名称 */
		ResourceRecordNode* m_head; /**< 资源的操作记录链表 */
		union {
			PassRecordContainer* m_creator; /**< 资源创建者 */
			const ResourceOperationInfo* m_permanentInfo; /**< 资源长期存在，没有创建者，只有初始状态 */
		};
		mutable OrderHash m_hash; /**< 该资源 创建订单 的哈希值，由GenOrder产生 */
		/** value一旦放入set中则不可以再被修改 */
		std::set<ResourceOperationInfo> m_opInfo; /**< 资源的具体使用方式 */
	};

	class PassRecordContainer {
	public:
		PassRecordContainer(const char* name, PassType type)
			: m_name(name), m_type(type) {}
		PassRecordContainer(PassRecordContainer&& rhs) { moveHelper(std::move(rhs)); }
		const PassRecordContainer& operator=(PassRecordContainer&& rhs) { moveHelper(std::move(rhs)); return *this; }
	public:
		/** 压入pass的读取操作信息，由Group调用
		 * @param resource 需要压入的记录了操作的节点指针，节点由ResourceRecordContainer管理 */
		void Read(ResourceRecordNode* resource) { m_readRecords.push_back(resource); }
		/** 压入pass的写入操作信息，由Group调用
		 * @param resource 需要压入的记录了操作的节点指针，节点由ResourceRecordContainer管理*/
		void Write(ResourceRecordNode* resource) { m_writeRecords.push_back(resource); }
		/** 压入pass的创建操作信息，由Group调用
		 * @param resource 需要压入所创建的资源记录的指针，资源记录本身由Group存储管理*/
		void Create(ResourceRecordContainer* resource) { m_createRecords.push_back(resource); }
		/** 删除指定的资源创建记录
		 * @param record 需要删除的资源创建记录 */
		void RemoveCreateRecord(ResourceRecordContainer* record) {
			for (decltype(m_createRecords)::iterator iter = m_createRecords.begin();
				iter != m_createRecords.end(); ++iter) {
				if (*iter == record) {
					m_createRecords.erase(iter);
					break;
				}
			}
		}
		/** 判断该pass还需不需要继续存储于Group中
		 * @return 该pass还需要返回true，不需要返回false */
		bool IsNecessary() {
			return m_readRecords.size() + m_writeRecords.size() + m_createRecords.size();
		}
		/** 根据passRecordConatiner自身信息，创建 Pass创建订单
		 * @remark 需要等待该Pass所有资源都已经创建完 订单 后才调用
		 * 因为该方法需要依赖资源 */
		PassOrder GenOrder() const;
	public:
		const std::vector<ResourceRecordNode*>& ReadRecords() const { return m_readRecords; }
		const std::vector<ResourceRecordNode*>& WriteRecords() const { return m_writeRecords; }
		PassType GetType() const { return m_type; }
		std::set<PassRecordContainer*> CleanUpReadAndWriteRecords();
	private:
		void moveHelper(PassRecordContainer&& rhs);
	private:
		std::string m_name; /**< 该pass的名称 */
		/** 所有的操作都以节点的形式存在，节点由ResourceRecordContainer管理 */
		std::vector<ResourceRecordNode*> m_readRecords; /**< 存储该pass的读取操作的节点指针 */
		std::vector<ResourceRecordNode*> m_writeRecords; /**< 存储该pass的写入操作的节点指针 */
		std::vector<ResourceRecordContainer*> m_createRecords; /**< 存储该pass所创建的资源(以资源记录的形式存在)的指针 */
		PassType m_type; /**< Pass的类型 */
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
		/** 向Group中增加已经存在(由其它内容创建)，且会在Goup中长时间存在的资源
		 * @param resources 需要添加的资源的数组，资源来自于脚本的读取结果(raw)
		 * @param size 数组的长度
		 * @return 增加成功返回true，否则返回false 
		 * @remark 该方法必须在pass添加前调用，务必确保所有前置资源都已经设置完成方才设置pass*/
		bool Push(const ResourceRawOperation* resources, uint32_t size);
		/** 向Group中增加pass
		 * @param passes 记录pass的数组，pass来自于脚本的读取结果(raw)
		 * @param size passes数组的大小
		 * @return 增加成功返回true，否则返回false
		 * @remark pass必须保证有序，同时必须确保前置资源设置完成后才开始调用该方法*/
		bool Push(const PassRawData* passes, uint32_t size);
		/** 整理当前已经添加到Group中的所有资源和pass，剔除冗余内容 */
		void SortOut();
		/** 校验当前Group是否可行
		 * TODO: 暂时理解为一定可行 */
		bool Verification() { return true; }
		/** 分析当前Group，并生成指令
		 * 指令应该包括：一次性的资源创建类型的指令
		 * 以及循环执行的Command List内的指令 */
		void Analyse();
	private:
		ResourceOperationInfo constructResourceOperationInfo (const ResourceRawOperation* res);
		/** 根据当前group中的资源记录容器及其信息，创建 资源创建订单 */
		std::vector<ResourceOrder> genResourceOrders();
		/** 根据当前group中的pass记录信息创建 pass创建订单 */
		std::vector<PassOrder> genPassOrders();
		/** 通过分析Pass，创建 任务调度队列 计划 taskQueue
		 * @param passes 当前分析出来的所有即将被创建的Pass创建订单
		 * @return 返回多个queue，每个queue都包含若干个排序完成的任务 */
		std::vector< std::vector<Task> > genTaskQueues(const std::vector<PassOrder>& passes);
	private:
		std::list<PassRecordContainer> m_passes; /**< 图中的所有pass，这些Pass已经被实施了拓扑排序 */
		std::map<std::string, ResourceRecordContainer> m_resources;
	};
}

#endif // UV_RENDER_GRAPH_H
