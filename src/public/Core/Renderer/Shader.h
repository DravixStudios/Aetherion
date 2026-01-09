#pragma once
#include <iostream>
#include "Core/Containers.h"
#include "Utils.h"
#include "Core/Logger.h"

enum class EShaderStage {
	VERTEX = 1 << 0,
	FRAGMENT = 1 << 1,
	COMPUTE = 1 << 2,
	GEOMETRY = 1 << 3,
	TESSELATION_CONTROL = 1 << 4,
	TESSELATION_EVALUATION = 1 << 5,
	GRAPHICS_ALL = VERTEX | FRAGMENT | GEOMETRY | TESSELATION_CONTROL | TESSELATION_EVALUATION,
	ALL = 0x7FFFFFFF
};

inline EShaderStage operator|(EShaderStage a, EShaderStage b) {
	return static_cast<EShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EShaderStage operator&(EShaderStage a, EShaderStage b) {
	return static_cast<EShaderStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}


enum class EShaderLanguage {
	GLSL, // OpenGL
	HLSL, // DirectX
	MSL, // Metal
	SPIRV // Vulkan
};

class Shader {
public:
	using Ptr = Ref<Shader>;

	static constexpr const char* CLASS_NAME = "Shader";

	explicit Shader();
	~Shader();
	void LoadFromGLSL(const String& path, EShaderStage shaderStage);
	void LoadFromGLSLSource(const String& source, const String& name, EShaderStage shaderStage);
	
	/**
	* 
	* Returns the SPIR-V bytecode of the shader
	* 
	* @return Constant reference to the SPIR-V bytecode
	* 
	*/
	const Vector<uint32_t>& GetSPIRV() const { return this->m_spirvCode; }
	String GetGLSL(uint32_t nVersion = 450);

	Ptr
	CreateShared() {
		return CreateRef<Shader>();
	}
private:
	Vector<uint32_t> m_spirvCode;

	String m_sourceGLSL;
	String m_filename;
	EShaderStage m_stage;

	Vector<uint32_t> CompileGLSLToSPIRV(const String& source, const String& name, EShaderStage stage);
	String TranspileSPIRVToGLSL(const Vector<uint32_t>& spirv, uint32_t nVersion);
};