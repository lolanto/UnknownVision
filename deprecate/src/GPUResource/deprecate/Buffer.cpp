#include "Buffer.h"

BEG_NAME_SPACE

std::unordered_map<StaticVertexBuffer*, std::unique_ptr<StaticVertexBuffer>> StaticVertexBuffer::BufferPointers;

END_NAME_SPACE
