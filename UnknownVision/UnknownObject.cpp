#include "UnknownObject.h"

UINT UnknownObject::NextAssetID = 0;
UINT UnknownObject::GetNextAssetID() {
	return ++NextAssetID;
}

UnknownObject::UnknownObject()
	:AssetID(GetNextAssetID()) {}


