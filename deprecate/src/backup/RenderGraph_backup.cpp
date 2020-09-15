#include "RenderGraphRaw.h"
#include "RenderGraph.h"

#include <stack>
#include <algorithm>
#include <map>
#include <cassert>

namespace UnknownVision {
	void* ResourceRecordContainer::Push(ResourceRecordType type, PassRecordContainer * pass, ResourceOperationInfo&& opInfo)
	{
		/** 向RecordContainer中插入operationInfo
		 * @param opinfo 需要插入的operation Info
		 * @return 一旦对应的info已经存在，则返回已存在的指针；若不存在，则返回插入后的Info指针 */
		auto insertResourceOperationInfoLambda = [this](ResourceOperationInfo&& opInfo) -> const ResourceOperationInfo*
		{
			opInfo.container = this;
			{
				auto& curIter = m_opInfo.find(opInfo);
				if (curIter == m_opInfo.end()) {
					/** 该操作记录尚未存入m_opInfo中 */
					auto [iter, isSucceed] = m_opInfo.insert(std::move(opInfo));
					if (!isSucceed) {
						/** 插入失败 */
						return nullptr;
					}
					return (&iter.operator*());
				}
				else {
					return (&curIter.operator*());
				}
			}
		};
		switch (type) {
		case RESOURCE_RECORD_TYPE_PERMANENT:
			if (IsTransient() /**< 该资源是短暂资源，不允许有长期状态 */
				|| m_permanentInfo != nullptr /**< 该资源已经有初始状态了 */
				) {
				return nullptr;
			}
			m_permanentInfo = insertResourceOperationInfoLambda(std::move(opInfo));
			return this;
		case RESOURCE_RECORD_TYPE_CREATE:
			if (m_creator != nullptr /** 该资源已经有创建记录了 */
				|| IsTransient() == false /**< 该资源为长期存在，不允许创建记录 */
				) {
				return nullptr;
			}
			m_creator = pass;
			return this;
		case RESOURCE_RECORD_TYPE_WRITE:
		case RESOURCE_RECORD_TYPE_READ:
		{
			/** 设置读写记录 */
			ResourceRecordNode* end = new ResourceRecordNode(type, pass, nullptr, nullptr);
			/** 通过构造函数已经将end->next设置为nullptr */
			if (!m_head) {
				m_head = end;
				end->last = nullptr;
			}
			else {
				ResourceRecordNode* tmp = m_head;
				while (tmp->next) { tmp = tmp->next; }
				tmp->next = end;
				end->last = tmp;
			}
			end->info = insertResourceOperationInfoLambda(std::move(opInfo));
			return end;
		}
		default:
			return nullptr;
		}
		return nullptr;
	}
	void ResourceRecordContainer::CleanUpNodes()
	{
		while (m_head) {
			ResourceRecordNode* tmp = m_head;
			m_head = m_head->next;
			delete tmp;
		}
	}
	bool ResourceRecordContainer::IsNecessary()
	{
		/** 该资源仍然有读写记录，或者是长期存在资源 */
		if (m_head || IsTransient() == false) return true;
		if (m_creator) {
			m_creator->RemoveCreateRecord(this);
			m_creator = nullptr;
		}
		return false;
	}
	ResourceOrder ResourceRecordContainer::GenOrder() const
	{
		ResourceOrder order;
		order.type = m_type;
		order.usage = RESOURCE_USAGE_AUTO;
		order.width = order.height = order.depth = 0;
		/** 遍历该资源的所有使用方式 */
		for (const auto& info : m_opInfo) {
			order.usage = static_cast<ResourceUsage>(
				static_cast<uint8_t>(order.usage) | static_cast<uint8_t>(info.usage));
			order.width = order.width > info.width ? order.width : info.width;
			order.height = order.height > info.height ? order.height : info.height;
			order.depth = order.depth > info.depth ? order.height : info.depth;
			/** TODO: 暂时只考虑最后一个操作的初始数据 */
			if (!info.initData.empty())
				order.initData = info.initData;
		}
		m_hash = order.Hash();
		return order;
	}
	void ResourceRecordContainer::moveHelper(ResourceRecordContainer && rhs)
	{
		m_name.swap(rhs.m_name);
		CleanUpNodes();
		m_head = rhs.m_head;
		rhs.m_head = nullptr;
		if (IsTransient())
			m_creator = rhs.m_creator;
		else
			m_permanentInfo = rhs.m_permanentInfo;
		m_opInfo.swap(rhs.m_opInfo);
	}
	PassOrder PassRecordContainer::GenOrder() const
	{
		PassOrder order;
		for (const auto& cr : m_createRecords)
			order.create.push_back(cr->GetFirstRecord()->info->BriefInfo());
		for (const auto& rr : m_readRecords)
			order.read.push_back(rr->info->BriefInfo());
		for (const auto& wr : m_writeRecords)
			order.write.push_back(wr->info->BriefInfo());
		order.type = m_type;
		return order;
	}
	std::set<PassRecordContainer*> PassRecordContainer::CleanUpReadAndWriteRecords()
	{
		std::set<PassRecordContainer*> affectedPasses;
		for (auto& readRecord : m_readRecords) {
			PassRecordContainer* ap = readRecord->Remove();
			if (ap) affectedPasses.insert(ap);
		}
		for (auto& writeRecord : m_writeRecords) {
			PassRecordContainer* ap = writeRecord->Remove();
			if (ap) affectedPasses.insert(ap);
		}
		m_readRecords.clear();
		m_readRecords.shrink_to_fit();
		m_writeRecords.clear();
		m_writeRecords.shrink_to_fit();
		return affectedPasses;
	}
	void PassRecordContainer::moveHelper(PassRecordContainer && rhs)
	{
		m_name.swap(rhs.m_name);
		m_type = rhs.m_type;
		m_readRecords.swap(rhs.m_readRecords);
		m_writeRecords.swap(rhs.m_writeRecords);
		m_createRecords.swap(rhs.m_createRecords);
	}
	bool Group::Push(const ResourceRawOperation * resources, uint32_t size)
	{
		const ResourceRawOperation* resource = resources;
		for (uint32_t index = 0; index < size; ++index, ++resource) {
			if (m_resources.find(resource->name) != m_resources.end()) {
				/** 重复的资源使用记录 */
				return false;
			}
			auto [iter, isSucceed] = m_resources.insert(std::make_pair(resource->name,
				ResourceRecordContainer(resource->name.c_str(), 
					static_cast<ResourceType>(resource->type & (~RESOURCE_TYPE_TRANSIENT)) )));
			if (!isSucceed) {
				/** 新资源数据插入失败 */
				return false;
			}
			if (!iter->second.Push(RESOURCE_RECORD_TYPE_PERMANENT, nullptr, 
				constructResourceOperationInfo(resource))) {
				/** 记录插入失败 */
				return false;
			}
		}
		return true;
	}
	bool Group::Push(const PassRawData * passes, uint32_t size)
	{
		const PassRawData* passData = passes;
		for (uint32_t index = 0; index < size; ++index, ++passData) {
			m_passes.push_back(PassRecordContainer(passData->name.c_str(), passData->type));
			PassRecordContainer& newPass = m_passes.back();
			{
				/** 处理所有的资源创建记录 */
				for (const auto& create : passData->createRecords) {
					auto& resIter = m_resources.find(create.name); /**< 尝试在已有资源中寻找当前所需资源 */
					if (resIter != m_resources.end()) {
						/** 重复的资源创建指令 */
						return false;
					}
					/** 加入新的资源操作记录容器
					 * @remark 暂时认为pass主动创建的资源都是临时资源，在group外不可访问*/
					auto [iter, isSucceed] = m_resources.insert(std::make_pair(create.name,
						ResourceRecordContainer(create.name.c_str(), 
							static_cast<ResourceType>(create.type | RESOURCE_TYPE_TRANSIENT)) ));
					if (!isSucceed) {
						/** 插入新资源记录结构体失败 */
						return false;
					}
					ResourceRecordContainer* newRec = reinterpret_cast<ResourceRecordContainer*>(
						iter->second.Push(RESOURCE_RECORD_TYPE_CREATE, &newPass,
							constructResourceOperationInfo(&create)));
					if (newRec == nullptr) {
						/** “创建”记录插入失败，清理刚插入的pass并返回 */
						m_passes.pop_back();
						return false;
					}
					newPass.Create(newRec);
				}
			}
			{
				/** 处理所有的资源 读写 记录 */
				auto readWriteProcessLambda =
					[this, &newPass](const ResourceRawOperation& opt, ResourceRecordType type)->bool
				{
					auto& resIter = m_resources.find(opt.name); /**< 尝试在已有资源中寻找当前所需资源 */
					if (resIter == m_resources.end()) {
						/** 资源尚未被创建，清理刚插入的pass并返回 */
						m_passes.pop_back();
						return false;
					}
					ResourceRecordNode* newRec = reinterpret_cast<ResourceRecordNode*>(
						resIter->second.Push(type, &newPass,
							constructResourceOperationInfo(&opt)));
					if (newRec == nullptr) {
						/** 记录插入失败，清理刚才插入的pass并返回 */
						m_passes.pop_back();
						return false;
					}
					if (type == RESOURCE_RECORD_TYPE_WRITE)
						newPass.Write(newRec);
					else newPass.Read(newRec);
					return true;
				};
				for (const auto& read : passData->readRecords)
					if (!readWriteProcessLambda(read, RESOURCE_RECORD_TYPE_READ))
						return false;
				for (const auto& write : passData->writeRecords)
					if (!readWriteProcessLambda(write, RESOURCE_RECORD_TYPE_WRITE))
						return false;
			}
		}
			return true;
	}

	void Group::SortOut()
	{
		std::stack<PassRecordContainer*> pendingList;
		for (auto& pass : m_passes)
			pendingList.push(&pass);
		while (!pendingList.empty()) {
			PassRecordContainer* pass = pendingList.top();
			pendingList.pop();
			bool mark = true; /**< 假如当前pass的所有write操作都可省略，则该标志为true，否则为false */
			for (const auto& writeRecord : pass->WriteRecords()) {
				if (writeRecord->next != nullptr /**< 当一个资源被写后，还有其它操作跟随，则该写操作不可省略 */
					|| writeRecord->info->container->IsTransient() == false /**< 当被写的资源是长期存在的资源，则该写操作不可省略 */
					) {
					mark = false;
					break;
				}
			}
			if (mark == true) {
				std::set<PassRecordContainer*>&& affectedPass = pass->CleanUpReadAndWriteRecords();
				for (auto& ap : affectedPass) pendingList.push(ap);
			}
		}
		for (auto resource = m_resources.begin(); resource != m_resources.end();) {
			if (!resource->second.IsNecessary() && resource->second.IsTransient()) {
				/** 资源不是必须的，同时也是短暂存在的，则可删除 */
				resource = m_resources.erase(resource);
			}
			else ++resource;
		}
		for (auto pass = m_passes.begin(); pass != m_passes.end();) {
			if (!pass->IsNecessary()) {
				pass = m_passes.erase(pass);
			}
			else ++pass;
		}
	}

	void Group::Analyse()
	{
		/** 创建 资源创建订单 */
		std::vector<ResourceOrder>&& resourceOrders = genResourceOrders();
		/** 创建 pass创建订单 */
		std::vector<PassOrder>&& passOrders = genPassOrders();
		/** 创建 调度顺序列表 */
		/** pass的执行，资源状态切换，Queue的同步 */
		
	}

	ResourceOperationInfo Group::constructResourceOperationInfo(const ResourceRawOperation * res)
	{
		ResourceOperationInfo info(res->usage, res->width, res->height, res->depth);
		if (res->initData.empty() == false) info.initData = res->initData; /**< 拥有初始数据 */
		return info;
	}

	std::vector<ResourceOrder> Group::genResourceOrders()
	{
		std::vector<ResourceOrder> orders;
		for (const auto& recordContainer : m_resources) {
			orders.push_back(recordContainer.second.GenOrder());
		}
		return orders;
	}

	std::vector<PassOrder> Group::genPassOrders()
	{
		std::vector<PassOrder> orders;
		for (const auto& pass : m_passes) {
			orders.push_back(pass.GenOrder());
		}
		return orders;
	}

	struct ResourceStateInfo {
		uint8_t queueID;
		ResourceUsage usage;
		ResourceStateInfo() : queueID(0xffU), usage(RESOURCE_USAGE_AUTO) {}
	};

	std::vector<std::vector<Task>> Group::genTaskQueues(const std::vector<PassOrder>& passes)
	{
		/** 输入是已经经过拓扑排序的pass节点，pass节点中包含
		 * 该pass的读写创建，需要根据pass的依赖关系，尽可能输出
		 * 并行度更高的pass的执行队列。需要注意资源的状态切换，
		 * 以及多个队列之间的同步 */
		std::map<OrderHash, ResourceStateInfo> resStateMap;
		for (const auto& resRecord : m_resources) {
			resStateMap.insert(std::make_pair(resRecord.second.GetHash(), ResourceStateInfo()));
		}
		uint8_t graphicQueue = 0;
		uint8_t computeQueue = 0;
		for (const auto& pass : passes) {
			uint8_t& cqueue = pass.type == PASS_TYPE_GRAPHIC ? graphicQueue : computeQueue;
			for (const auto& cr : pass.create) {
				auto& state = resStateMap.find(cr.hash);
				state->second.queueID = cqueue;
				state->second.usage = cr.usage;
			}
			for (const auto& wr : pass.write) {
				auto& state = resStateMap.find(wr.hash);
				if ()
				
			}
			for (const auto& rr : pass.read) {

			}
		}
	}

	PassRecordContainer * ResourceRecordNode::Remove()
	{
		if (last == nullptr) {
			/** 头节点 */
			if (next == nullptr) {
				/** 单一节点 */
				info->container->CleanUpNodes();
			}
			else {
				/** 将下一个节点的内容拷贝为头节点，删除“下一个”节点 */
				ResourceRecordNode* node = next;
				memcpy(this, node, sizeof(ResourceRecordNode));
				last = nullptr;
				if (next != nullptr) {
					next->last = this;
				}
				delete node;
			}
			return nullptr;
		}
		else if (next == nullptr) {
			/** 尾节点 */
			ResourceRecordNode* node = last;
			delete this;
			node->next = nullptr;
			if (node->type == RESOURCE_RECORD_TYPE_READ)
				return nullptr;
			return node->pass;
		}
		else {
			last->next = next;
			next->last = last;
			delete this;
			return nullptr;
		}
	}
}
