#pragma once
#include "../UVConfig.h"
#include <vector>

BEG_NAME_SPACE

	ALIAS_INDEX(uint64_t, BufferHandle);
	ALIAS_INDEX(uint16_t, TextureHandle);
	ALIAS_INDEX(uint16_t, ProgramHandle);

	struct Handle {
		enum Type: uint8_t {
			HANDLE_TYPE_INVALID = 0x00U,
			HANDLE_TYPE_BUFFER = 0x01U,
			HANDLE_TYPE_TEXTURE = 0x02U,
			HANDLE_TYPE_PROGRAM = 0x04U
		};
		Type type;
		union
		{
			BufferHandle buffer;
			TextureHandle texture;
			ProgramHandle program;
		};
		Handle() : type(HANDLE_TYPE_INVALID) {}
		Handle(BufferHandle handle) : type(HANDLE_TYPE_BUFFER), buffer(handle) {}
		Handle(TextureHandle handle) : type(HANDLE_TYPE_TEXTURE), texture(handle) {}
		Handle(ProgramHandle handle) : type(HANDLE_TYPE_PROGRAM), program(handle) {}
	};

	enum ParameterType : uint8_t {
		PARAMETER_TYPE_INVALID = 0x00U,
		PARAMETER_TYPE_BUFFER = 0x01U,
		PARAMETER_TYPE_TEXTURE = 0x02U
	};

	struct Parameter {
		union
		{
			BufferHandle buffer;
			TextureHandle texture;
		};
		ParameterType type = PARAMETER_TYPE_INVALID;
		bool IsBuffer() const { return type == PARAMETER_TYPE_BUFFER; }
		bool IsTexture() const { return type == PARAMETER_TYPE_TEXTURE; }
	};

	using ProgramParameters = std::vector<Parameter>;
	//ALIAS_INDEX(uint32_t, SlotIdx);

END_NAME_SPACE
