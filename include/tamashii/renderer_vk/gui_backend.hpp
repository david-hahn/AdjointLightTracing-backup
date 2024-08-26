#pragma once
#include <tamashii/public.hpp>
#include <rvk/rvk.hpp>

T_BEGIN_NAMESPACE

class VulkanInstance;
class GuiBackend
{
public:
	GuiBackend(VulkanInstance& aInstance, void* aSurfaceHandle, uint32_t aFrameCount, std::string_view aFontFilePath);
	~GuiBackend();
	GuiBackend(const GuiBackend&) = delete;
	GuiBackend& operator=(const GuiBackend&) = delete;
	GuiBackend(GuiBackend&&) = delete;
	GuiBackend& operator=(GuiBackend&&) = delete;

	void prepare(uint32_t aWidth, uint32_t aHeight);
	void draw(rvk::CommandBuffer* cb, uint32_t cFrameIdx) const;

private:
	void _buildPipeline() const;

	VulkanInstance& mInstance;
	rvk::LogicalDevice& mDevice;
	rvk::CommandPool& mCommandPool;
	void* mSurfaceHandle;
	uint32_t mFrameCount;

	struct Data {
		explicit Data(rvk::LogicalDevice& aDevice, uint32_t aFrameCount) : mImage{ &aDevice },
			mDescriptor{ &aDevice }, mShader{ &aDevice }, mPipeline{ &aDevice }, mVertex{ &aDevice },
			mIndex{ &aDevice }
		{
			mVertex.resize(aFrameCount, &aDevice);
			mIndex.resize(aFrameCount, &aDevice);
		}

		rvk::Image						mImage;
		rvk::Descriptor					mDescriptor;
		rvk::RShader					mShader;
		rvk::RPipeline					mPipeline;

		std::vector<rvk::Buffer>		mVertex;
		std::vector<rvk::Buffer>		mIndex;
	};
	std::shared_ptr<Data>			mImguiData;
};

T_END_NAMESPACE
