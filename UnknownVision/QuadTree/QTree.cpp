#include "QTree.h"

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   QTree   ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

QTree::QTree() {}

QTree::QTree(RectF area, int level) {
	m_level = level;
	m_area = area;
	m_root = std::vector<QTrunk>((pow(4, m_level) - 1) / 3);
	m_root[0].range = m_area;
	createTreeHelper(0, m_level - 1, m_area);
}

///////////////////
// public function
///////////////////

void QTree::Insert(QLeave* leave) {
	checkoutLeave(leave, [this](CHECK_OUT_LEAVE_PARAM) {
		m_root[node].leaves.push_back(leave);
	});
}

bool QTree::CollisionDetect(RectF& r, std::vector<QLeave*>& leaves) {
	unsigned int orgSize = leaves.size();
	rangeCollisionHelper(0, r, leaves);
	if (leaves.size() != orgSize) return true;
	return false;
}

bool QTree::CollisionDetect(PntF& p, std::vector<QLeave*>& leaves) {
	int cur = 0;
	unsigned int orgSize = leaves.size();
	while (true) {
		for (auto iter : m_root[cur].leaves)
			if (GeoRect::isInside(iter->area, p))
				leaves.push_back(iter);
		if (!(4 * cur + 1 < m_root.size())) break;
		for (auto i = 1; i < 5; ++i)
			if (GeoRect::isInside(m_root[4 * cur + i].range, p)) {
				cur = 4 * cur + i;
				break;
			}
	}
	if (leaves.size() != orgSize) return true;
	return false;
}

void QTree::Remove(QLeave* l) {
	checkoutLeave(l, [this](CHECK_OUT_LEAVE_PARAM) {
		m_root[node].leaves.erase(*iter);
	}, CheckOutLeave_Exist);
}

void QTree::Update(QLeave* l) {
	Remove(l);
	Insert(l);
}

///////////////////
// private function
///////////////////

void QTree::createTreeHelper(int root, int level, RectF area) {
	if (!level) return;
	float dw = (area.right - area.left) / 2.0f;
	float dh = (area.bottom - area.top) / 2.0f;
	m_root[4 * root + 1].range = {
		area.left, area.top, area.left + dw, area.top + dh
	};
	m_root[4 * root + 2].range = {
		area.left, area.top + dh, area.left + dw, area.bottom
	};
	m_root[4 * root + 3].range = {
		area.left + dw, area.top + dh, area.right, area.bottom
	};
	m_root[4 * root + 4].range = {
		area.left + dw, area.top, area.right, area.top + dh
	};
	--level;
	for (auto i = 1; i < 5; ++i)
		createTreeHelper(4 * root + i, level, m_root[4 * root + i].range);
}

void QTree::rangeCollisionHelper(int curNode, RectF& area, std::vector<QLeave*>& leaves) {
	// 与当前节点内的叶子做相交测试
	for (auto& iter : m_root[curNode].leaves) {
		if (GeoRect::isIntersect(iter->area, area)) leaves.push_back(iter);
	}
	// 检查当前是否已经到达底层
	if (!(4 * curNode + 1 < m_root.size())) return;
	// 检查当前选框与哪些子区域相交，并与相交区域中的叶子进行相交测试
	for (auto i = 1; i < 5; ++i) {
		if (GeoRect::isIntersect(m_root[4 * curNode + i].range, area))
			rangeCollisionHelper(4 * curNode + i, area, leaves);
	}
}

void QTree::checkoutLeave(QLeave* leave,
	std::function<void(CHECK_OUT_LEAVE_PARAM)> cb,
	QTree::CheckOutLeaveFlag flag) {
	int last = 0;
	unsigned int i = 0;
	while (4 * last + 1 < m_root.size()) {
		for (i = 1; i < 5; ++i)
			if (GeoRect::isInside(m_root[4 * last + i].range, leave->area)) {
				last = 4 * last + i;
				break;
			}
		if (i == 5) break;
	}
	switch (flag)
	{
	case QTree::CheckOutLeave_Always:
		cb(leave, last, nullptr);
		break;
	case QTree::CheckOutLeave_Exist:
	case QTree::CheckOutLeave_NotExist:
		for (auto iter = m_root[last].leaves.begin();
			iter != m_root[last].leaves.end();
			++iter, ++i) {
			if ((*iter) == leave
				&& flag == CheckOutLeave_Exist) {
				cb(leave, last, &iter);
				break;
			}
		}
		if (i == m_root[last].leaves.size()
			&& flag == CheckOutLeave_NotExist)
			cb(leave, last, nullptr);
		break;
	default:
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   GeoRect   /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

bool GeoRect::isInside(RectF& outside, RectF& inside) {
	if (inside.left >= outside.left &&
		inside.top >= outside.top &&
		inside.right <= outside.right &&
		inside.bottom <= outside.bottom)
		return true;
	return false;
}

bool GeoRect::isInside(RectF& outside, PntF& inside) {
	if (inside.x >= outside.left &&
		inside.x <= outside.right &&
		inside.y >= outside.top &&
		inside.y <= outside.bottom)
		return true;
	return false;
}

bool GeoRect::isIntersect(RectF& a, RectF& b, _Out_ RectF* res) {
	if (a.right < b.left) return false;
	if (a.left > b.right) return false;
	if (a.bottom < b.top) return false;
	if (a.top > b.bottom) return false;
	if (!res) return true;
	res->left = a.left > b.left ? a.left : b.left;
	res->right = a.right > b.right ? b.right : a.right;
	res->top = a.top > b.top ? a.top : b.top;
	res->bottom = a.bottom > b.bottom ? b.bottom : a.bottom;
	return true;
}