#include "UnknownObject.h"

UINT UnknownObject::NextAssetID = 0;
UINT UnknownObject::GetNextAssetID() {
	return ++NextAssetID;
}

UnknownObject::UnknownObject()
	:AssetID(GetNextAssetID()) {}

bool ConstantBuffersTest(ConstBufferDesc* cbd1, ConstBufferDesc* cbd2) {
	if (cbd1->size != cbd2->size) return false;
	if (cbd1->slot != cbd2->slot) return false;
	return true;
}

