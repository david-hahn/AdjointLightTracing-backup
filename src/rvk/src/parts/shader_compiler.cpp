#include "shader_compiler.hpp"
#include <rvk/shader_compiler.hpp>
#include <rvk/parts/logger.hpp>
#include <sstream>
#include <fstream>

#include <codecvt>
#include <locale>


#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <StandAlone/DirStackFileIncluder.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/disassemble.h>


namespace {
    EShLanguage stageToEShLanguage(const rvk::Shader::Stage aStage)
    {
        switch (aStage) {
        
        case rvk::Shader::Stage::VERTEX: return EShLangVertex;
        case rvk::Shader::Stage::TESS_CONTROL: return EShLangTessControl;
        case rvk::Shader::Stage::TESS_EVALUATION: return EShLangTessEvaluation;
        case rvk::Shader::Stage::GEOMETRY: return EShLangGeometry;
        case rvk::Shader::Stage::FRAGMENT: return EShLangFragment;
        
        case rvk::Shader::Stage::COMPUTE: return EShLangCompute;
        
        case rvk::Shader::Stage::RAYGEN: return EShLangRayGen;
        case rvk::Shader::Stage::ANY_HIT: return EShLangAnyHit;
        case rvk::Shader::Stage::CLOSEST_HIT: return EShLangClosestHit;
        case rvk::Shader::Stage::MISS: return EShLangMiss;
        case rvk::Shader::Stage::INTERSECTION: return EShLangIntersect;
        case rvk::Shader::Stage::CALLABLE: return EShLangCallable;
        }
        throw std::runtime_error("Unsupported Shader Stage for glslang");
    }

    glslang::EShSource sourceToEShSourceGlsl(const rvk::Shader::Source aSource)
    {
        switch (aSource) {
        case rvk::Shader::Source::GLSL: return glslang::EShSourceGlsl;
        case rvk::Shader::Source::HLSL: return glslang::EShSourceHlsl;
        case rvk::Shader::Source::SPV: break;
        }
        throw std::runtime_error("Unsupported Source Type for glslang");
    }

    std::string_view getFilePath(std::string_view aStr)
    {
	    const size_t found = aStr.find_last_of("/\\");
        return aStr.substr(0, found);
        
    }

    

    
    void OutputSpvBin(std::string_view spirv, const char* baseName)
    {
        std::ofstream out;
        out.open(baseName, std::ios::binary | std::ios::out);
        if (out.fail())
            printf("ERROR: Failed to open file: %s\n", baseName);
        out.write(spirv.data(), static_cast<std::streamsize>(spirv.size()));
        out.close();
    }
}

namespace {
    
    int ClientInputSemanticsVersion = 100;
    glslang::EShTargetClientVersion ClientVersion;
    glslang::EShClient Client = glslang::EShClientVulkan;
    glslang::EShTargetLanguage TargetLanguage = glslang::EShTargetSpv;
    glslang::EShTargetLanguageVersion TargetVersion;

    EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules | EShMsgReadHlsl);
    constexpr int defaultVersion = 100;

    
    std::string preamble;                   
    std::vector<std::string> processes;     

    glslang::TShader* shader = nullptr;
    EShLanguage stage;
    DirStackFileIncluder includer;
    std::size_t hash;
    std::string preprocessedCode;
}


void scomp::cleanup() {
    if (shader != nullptr) {
        delete shader;
        shader = nullptr;
    }
    includer = DirStackFileIncluder();
    hash = 0;
    preprocessedCode = "";
    preamble = "";
    processes.clear();

    ClientVersion = glslang::EShTargetVulkan_1_2;
    TargetVersion = glslang::EShTargetSpv_1_5;

    
}


void scomp::getGlslangVersion(int& aMajor, int& aMinor, int& aPatch)
{
	const glslang::Version glslangVersion = glslang::GetVersion();
    aMajor = glslangVersion.major;
	aMinor = glslangVersion.minor;
	aPatch = glslangVersion.patch;
}

void scomp::init()
{
    glslang::InitializeProcess();
}

void scomp::finalize()
{
    cleanup();
    glslang::FinalizeProcess();
}

bool scomp::preprocess(const rvk::Shader::Source aRvkSource, const rvk::Shader::Stage aRvkStage, std::string_view aPath, std::string_view aCode, std::vector<std::string> const& aDefines, std::string_view aEntryPoint) {
    cleanup();
    const glslang::EShSource lang = sourceToEShSourceGlsl(aRvkSource);
    if (lang == glslang::EShSourceHlsl) {
        ClientVersion = glslang::EShTargetVulkan_1_2;
        TargetVersion = glslang::EShTargetSpv_1_3;
    }

    stage = stageToEShLanguage(aRvkStage);
    shader = new glslang::TShader(stage);
    const char* ccontent;
    ccontent = aCode.data();
    shader->setStrings(&ccontent, 1);
    shader->setEnvInput(lang, stage, Client, ClientInputSemanticsVersion);
    shader->setEnvClient(Client, ClientVersion);
    shader->setEnvTarget(TargetLanguage, TargetVersion);
    
    shader->setEntryPoint(aEntryPoint.data());
    
    processes.reserve(aDefines.size());
    for (const std::string& s : aDefines) {
        if (s.empty()) continue;
        preamble.append("#define " + s + "\n");
        processes.emplace_back("define-macro " + s);
    }
    shader->setPreamble(preamble.c_str());
    shader->addProcesses(processes);

    
    includer.pushExternalLocalDirectory(std::string{getFilePath(aPath)});

    
    if (!shader->preprocess(GetDefaultResources(), defaultVersion, ENoProfile, false, false, messages, &preprocessedCode, includer))
    {
        if (strlen(shader->getInfoLog()) > 0) rvk::Logger::error("\n" + std::string(shader->getInfoLog()));
        if (strlen(shader->getInfoDebugLog()) > 0) rvk::Logger::error("\n" + std::string(shader->getInfoDebugLog()));
        return false;
    }

    hash = std::hash<std::string>{}(preprocessedCode);
    return true;
}

std::size_t scomp::getHash()
{
    return hash;
}

bool scomp::compile(std::string& aSpv, const bool aCache, std::string_view aCacheDir) {
    const char* cpreprocessedCode = preprocessedCode.c_str();
    shader->setStrings(&cpreprocessedCode, 1);

    if (!shader->parse(GetDefaultResources(), defaultVersion, false, messages, includer)) {
        if (strlen(shader->getInfoLog()) > 0) rvk::Logger::error("\n" + std::string(shader->getInfoLog()));
        if (strlen(shader->getInfoDebugLog()) > 0) rvk::Logger::error("\n" + std::string(shader->getInfoDebugLog()));
        return false;
    }
    glslang::TProgram program;
    program.addShader(shader);
    if (!program.link(messages)) {
        if (strlen(shader->getInfoLog()) > 0) rvk::Logger::error("\n" + std::string(shader->getInfoLog()));
        if (strlen(shader->getInfoDebugLog()) > 0) rvk::Logger::error("\n" + std::string(shader->getInfoDebugLog()));
        return false;
    }
    if (!program.mapIO()) {
        if (strlen(program.getInfoLog()) > 0) rvk::Logger::error("\n" + std::string(program.getInfoLog()));
        if (strlen(program.getInfoDebugLog()) > 0) rvk::Logger::error("\n" + std::string(program.getInfoDebugLog()));
        return false;
    }

    std::vector<unsigned int> spirv;
    if (program.getIntermediate(stage)) {

        std::string warningsErrors;
        spv::SpvBuildLogger logger;
        glslang::SpvOptions spvOptions;
        spvOptions.disableOptimizer = false;
        spvOptions.optimizeSize = false;
        spvOptions.disassemble = false;
        spvOptions.validate = false;
        glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &logger, &spvOptions);

        
        

        
        if (aCache) {
            std::ostringstream oss;
            oss << aCacheDir << "/" << hash << ".spv";
            glslang::OutputSpvBin(spirv, oss.str().c_str());
        }
    }
    aSpv.clear();
    aSpv.reserve(spirv.size() * 4);
    for (auto i : spirv) {
        aSpv.push_back(reinterpret_cast<int8_t*>(&i)[0]);
        aSpv.push_back(reinterpret_cast<int8_t*>(&i)[1]);
        aSpv.push_back(reinterpret_cast<int8_t*>(&i)[2]);
        aSpv.push_back(reinterpret_cast<int8_t*>(&i)[3]);
    }
    return true;
}
