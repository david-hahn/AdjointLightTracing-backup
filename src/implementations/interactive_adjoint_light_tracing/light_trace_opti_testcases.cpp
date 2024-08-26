#include "light_trace_opti.hpp"
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/input.hpp>
#include <tamashii/core/scene/ref_entities.hpp>

#include "../../../assets/shader/ialt/defines.h"


#include <tamashii/core/platform/system.hpp>

#include <thread>
#include <fstream>
#include <iostream>

#include "ialt.hpp"

T_USE_NAMESPACE

void LightTraceOptimizer::runPredefinedTestCase(InteractiveAdjointLightTracing* ialt, const SceneBackendData& aScene, const std::string& aCaseName, rvk::Buffer* aRadianceBufferOut){
	
}
