#include "example_impl.hpp"
#include <tamashii/tamashii.hpp>
#include <tamashii/core/common/common.hpp>
#include <rvk/rvk.hpp>
#include <imgui.h>

T_USE_NAMESPACE
RVK_USE_NAMESPACE


#include <tamashii/core/common/vars.hpp>
ccli::Var example_var("", "example_var", "100", ccli::CliOnly, "Set a var, and init it with --example_var value");


#include <tamashii/core/platform/filewatcher.hpp>





#include <tamashii/core/common/input.hpp>




#include <tamashii/renderer_vk/convenience/rvk_type_converter.hpp>

ExampleComputeImpl::ExampleComputeImpl(const tamashii::VulkanRenderRoot& aRoot) : mRoot{ aRoot } {}


void ExampleComputeImpl::windowSizeChanged(const uint32_t aWidth, const uint32_t aHeight) {

}


void ExampleComputeImpl::prepare(tamashii::RenderInfo_s& aRenderInfo) {
	
}
void ExampleComputeImpl::destroy() {
	
}


void ExampleComputeImpl::sceneLoad(SceneBackendData aScene) {
	
}
void ExampleComputeImpl::sceneUnload(SceneBackendData aScene) {
	
}


void ExampleComputeImpl::drawView(ViewDef_s* aViewDef) {
	
	const CommandBuffer& cb = mRoot.currentCmdBuffer();

	if (mRoot.instance.mSwapchainData.mSwapchain) {
		
		const glm::vec3 cc = glm::vec3{ var::varToVec(var::bg) } / 255.0f;
		cb.cmdBeginRendering(
			{ { &mRoot.currentImage(),
			{{cc.x, cc.y, cc.z, 1.0f}} , RVK_LC, RVK_S2 } },
			{ &mRoot.currentDepthImage(),
			{0.0f, 0}, RVK_LC, RVK_S2 }
		);
		cb.cmdEndRendering();
	}
}
void ExampleComputeImpl::drawUI(UiConf_s* aUiConf) {
	if (ImGui::Begin("Settings", nullptr, 0))
	{
		ImGui::Separator();
		ImGui::TextWrapped("Add to the default UI or create your own widget");
		ImGui::Separator();
		ImGui::End();
	}
}

void ExampleComputeImpl::doCompute()
{
	rvk::SingleTimeCommand stc = mRoot.singleTimeCommand();
	
	uint32_t dataArrayIn[] = { 99, 100, 101, 1 };
	const rvk::Buffer dataBuffer(&mRoot.device, rvk::Buffer::Use::STORAGE | rvk::Buffer::Use::UPLOAD | rvk::Buffer::Use::DOWNLOAD, sizeof(dataArrayIn), rvk::Buffer::Location::DEVICE);
	dataBuffer.STC_UploadData(&stc, &dataArrayIn[0], sizeof(dataArrayIn));
	
	rvk::CShader cshader(&mRoot.device);
	
	cshader.addStage(rvk::Shader::Source::GLSL, rvk::Shader::Stage::COMPUTE, "assets/shader/example_impl/simple_compute_glsl.comp");
	
	
	cshader.finish();
	
	int multiplier = 5;
	cshader.addConstant(0, 0, sizeof(multiplier), 0);
	cshader.setConstantData(0, &multiplier, 4);
	
	rvk::Descriptor descriptor(&mRoot.device);
	descriptor.addStorageBuffer(0, rvk::Shader::Stage::COMPUTE);
	descriptor.setBuffer(0, &dataBuffer);
	descriptor.finish();
	
	rvk::CPipeline cpipe(&mRoot.device);
	cpipe.addDescriptorSet({ &descriptor });
	cpipe.setShader(&cshader);
	cpipe.finish();

	
	stc.begin();
	cpipe.CMD_BindDescriptorSets(stc.buffer(), { &descriptor });
	cpipe.CMD_BindPipeline(stc.buffer());
	cpipe.CMD_Dispatch(stc.buffer(), 4);
	stc.end();

	
	uint32_t dataArrayOut[4];
	dataBuffer.STC_DownloadData(&stc, &dataArrayOut[0], sizeof(dataArrayOut));
	spdlog::info("	Compute shader example: multiply each value of input array with {}", multiplier);
	spdlog::info("	input: {} {} {} {}", dataArrayIn[0], dataArrayIn[1], dataArrayIn[2], dataArrayIn[3]);
	spdlog::info("	result: {} {} {} {}", dataArrayOut[0], dataArrayOut[1], dataArrayOut[2], dataArrayOut[3]);

	EventSystem::queueEvent(EventType::ACTION, Input::A_EXIT);
}

void ExampleComputeImpl::runComputation() {
	auto* exampleImpl = Common::getInstance().getRenderSystem()->getCurrentBackendImplementations();
	if (std::strcmp(exampleImpl->getName(), EXAMPLE_IMPLEMENTATION_NAME) != 0) {
		throw std::runtime_error{ "Expected example shader to be currently active" };
	}

	dynamic_cast<ExampleComputeImpl*>(exampleImpl)->doCompute();
}


#include <tamashii/implementations/default_rasterizer.hpp>
#include <tamashii/implementations/default_path_tracer.hpp>

std::vector<tamashii::RenderBackendImplementation*> VulkanRenderBackendExampleCompute::initImplementations(VulkanInstance* aInstance)
{
	return {
		new tamashii::DefaultRasterizer{tamashii::VulkanRenderRoot {*aInstance->mDevice, *aInstance }},
		new tamashii::DefaultPathTracer{tamashii::VulkanRenderRoot {*aInstance->mDevice, *aInstance }},
		new ExampleComputeImpl{tamashii::VulkanRenderRoot { *aInstance->mDevice, *aInstance }}
	};
}
