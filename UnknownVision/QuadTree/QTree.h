#pragma once
#include <vector>
#include <functional>
#include <dwrite.h>

typedef D2D_POINT_2F PntF;
typedef D2D_RECT_F RectF;

namespace GeoRect {
	bool isInside(RectF& outside, RectF& inside);
	bool isInside(RectF& outside, PntF& inside);
	// a,b是否相交，并返回相交结果
	bool isIntersect(RectF& a, RectF& b, _Out_ RectF* res = nullptr);
}

class QLeave {
public:
	QLeave(RectF _area) : area(_area) {}
	RectF area;
};

struct QTrunk {
	RectF range;
	std::vector<QLeave*> leaves;
};

class QTree {
#define CHECK_OUT_LEAVE_PARAM QLeave* leave, int node, std::vector<QLeave*>::iterator* iter
	enum CheckOutLeaveFlag {
		CheckOutLeave_Always,
		CheckOutLeave_Exist,
		CheckOutLeave_NotExist
	};
public:
	QTree();
	QTree(RectF area, int level);
	void Insert(QLeave*);
	void Update(QLeave*);
	void Remove(QLeave*);

	bool CollisionDetect(RectF&, std::vector<QLeave*>&);
	bool CollisionDetect(PntF&, std::vector<QLeave*>&);
private:
	void createTreeHelper(int root, int level, RectF area);
	void rangeCollisionHelper(int curNode, RectF& area, std::vector<QLeave*>&);
	void checkoutLeave(QLeave*,
		std::function<void(CHECK_OUT_LEAVE_PARAM)>,
		CheckOutLeaveFlag flag = CheckOutLeave_Always);
private:
	std::vector<QTrunk>			m_root;
	int							m_level;
	RectF						m_area;
};