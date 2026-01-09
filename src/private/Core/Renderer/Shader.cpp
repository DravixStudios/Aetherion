#include "Core/Renderer/Shader.h"
#include <fstream>
#include <sstream>
#include <shaderc/shaderc.hpp>

Shader::Shader() {

}

Shader::~Shader() {

}

/**
 * Loads a GLSL shader from a file 
 * 
 * @param path Path to the shader (absolute)
 * @param shaderStage Shader's stage
 * 
 * @throws std::runtime_error and console error when file not found
 */
void 
Shader::LoadFromGLSL(const String& path, EShaderStage shaderStage) {
    /* Convert path to absolute path if is not absolute */
    std::filesystem::path absPath = path;
    if(!absPath.is_absolute()) {
        String executableDir = GetExecutableDir();
        absPath = std::filesystem::path(executableDir) / path;
    }
    
    std::ifstream file(absPath);
    if(!file.is_open()) {
        Logger::Error("Shader::LoadFromGLSL: Cannot open file: {}", path);
        throw std::runtime_error("Shader::LoadFromGLSL: Cannot open file");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    this->LoadFromGLSLSource(buffer.str(), path, shaderStage);
}

/**
 * Loads a GLSL shader from source
 * 
 * This method will normally be called by Shader::LoadFromGLSL.
 * It is not recommended to use this method with hard-coded
 * shader code.
 * 
 * @param source Source shader code
 * @param name Shader name (normally the shader file name)
 * @param shaderStage Shader's stage
 */
void 
Shader::LoadFromGLSLSource(const String& source, const String& name, EShaderStage shaderStage) {
    this->m_sourceGLSL = source;
    this->m_filename = name;
    this->m_stage = shaderStage;

    /* Compile SPIR-V */
    this->m_spirvCode = this->CompileGLSLToSPIRV(source, name, shaderStage);
    Logger::Debug(
        "Shader::LoadGLSLSource: Loaded {} as {}", 
        name, 
        shaderStage == EShaderStage::VERTEX ? "vertex" : 
        shaderStage == EShaderStage::FRAGMENT ? "fragment" : "compute"
    );
}

/** 
 * Compiles GLSL into SPIR-V with shaderc
 * 
 * @param source Source shader code
 * @param name Shader name (normally the shader file name)
 * @param shaderStage Shader's stage
 * 
 * @returns The compiled SPIR-V shader in a uint32_t vector.
 * 
 * @throws std::runtime_error if shader preprocessing or compilation fails 
 */
Vector<uint32_t> 
Shader::CompileGLSLToSPIRV(const String& source, const String& name, EShaderStage stage) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    options.SetTargetSpirv(shaderc_spirv_version_1_5);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    options.SetForcedVersionProfile(450, shaderc_profile_none);

    shaderc_shader_kind kind;
    switch(stage) {
        case EShaderStage::VERTEX: kind = shaderc_vertex_shader; break;
        case EShaderStage::FRAGMENT: kind = shaderc_fragment_shader; break;
        case EShaderStage::COMPUTE: kind = shaderc_compute_shader; break;
        default: kind = shaderc_vertex_shader; break;
    }

    /* Preprocess GLSL */
    shaderc::PreprocessedSourceCompilationResult preprocessedResult = 
        compiler.PreprocessGlsl(
            source, 
            kind, 
            name.c_str(), 
            options
        );

    if(preprocessedResult.GetCompilationStatus() != shaderc_compilation_status_success) {
        Logger::Error("Shader::CompileGLSLToSPIRV: {}", preprocessedResult.GetErrorMessage());
        throw std::runtime_error("Shader::CompileGLSLToSPIRV: Failed preprocessing shader");
    }

    /* Compile preprocessed GLSL to SPIR-V */
    shaderc::SpvCompilationResult result = 
        compiler.CompileGlslToSpv(
            preprocessedResult.cbegin(), 
            kind, 
            name.c_str(), 
            options
        );
        
    if(result.GetCompilationStatus() != shaderc_compilation_status_success) {
        Logger::Error("Shader::CompileGLSLToSPIRV: {}", result.GetErrorMessage());
        throw std::runtime_error("Shader::CompileGLSLToSPIRV: Failed compiling shader");
    }

    return Vector<uint32_t>(result.cbegin(), result.cend());
}