#include "Core/Renderer/Shader.h"
#include <fstream>
#include <sstream>

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

    this->LoadFromGLSLSource(buffer.str(), shaderStage);
}

/**
 * Loads a GLSL shader from source
 * 
 * This method will normally be called by Shader::LoadFromGLSL.
 * It is not recommended to use this method with hard-coded
 * shader code.
 * 
 * @param source Source shader code
 * @param shaderStage Shader's stage
 */
void 
Shader::LoadFromGLSLSource(const String& source, EShaderStage shaderStage) {

}