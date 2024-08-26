#pragma once
#include <rvk/parts/rvk_public.hpp>

RVK_BEGIN_NAMESPACE
class Image;
class Buffer;
class TopLevelAS;
class LogicalDevice;
class Descriptor {
public:
	enum BindingFlag {
		PARTIALLY_BOUND = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT,
		VARIABLE_DESCRIPTOR_COUNT = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
	};

													Descriptor(LogicalDevice* aDevice);
													~Descriptor();

													
	void											reserve(int aReserve);

													
													
	void											addImageSampler(uint32_t aBinding, VkShaderStageFlags aStages, uint32_t aCount = 1);
	void											addStorageImage(uint32_t aBinding, VkShaderStageFlags aStages, uint32_t aCount = 1);
	void											addUniformBuffer(uint32_t aBinding, VkShaderStageFlags aStages, uint32_t aCount = 1);
	void											addStorageBuffer(uint32_t aBinding, VkShaderStageFlags aStages, uint32_t aCount = 1);
	void											addAccelerationStructureKHR(uint32_t aBinding, VkShaderStageFlags aStages, uint32_t aCount = 1);
													
													
	void											setImage(uint32_t aBinding, const Image* aImage, uint32_t aPosition = 0);
	void											setBuffer(uint32_t aBinding, const Buffer* aBuffer, uint32_t aPosition = 0);
	void											setAccelerationStructureKHR(uint32_t aBinding, const TopLevelAS* aTlas, uint32_t aPosition = 0);
													
													
													
	void											setUpdateSize(uint32_t aBinding, uint32_t aUpdateSize);

													
	void											setBindingFlags(uint32_t aBinding, VkDescriptorBindingFlags aFlags);

													
													
													
													
	void											finish(bool aWithUpdate = true);
	void											update() const;

	VkDescriptorSet									getHandle() const;
	VkDescriptorSetLayout							getLayout() const;

	void											destroy();
private:
	friend class									Pipeline;
	friend class									RPipeline;
	friend class									CRipeline;
	friend class									RTPipeline;

	void											createDescriptorSetLayout();
	void											createDescriptorPool();
	void											createDescriptorSet();

	
	void											pushImage(uint32_t aBinding, VkShaderStageFlags aStages, uint32_t aCount, VkDescriptorType aType, VkImageLayout aLayout);
	void											pushBuffer(uint32_t aBinding, VkShaderStageFlags aStages, uint32_t aCount, VkDescriptorType aType);

	struct bindings_s {
		VkDescriptorSetLayoutBinding					binding = {};
		std::vector<VkDescriptorImageInfo>				descImageInfos = {};
		std::vector<VkDescriptorBufferInfo>				descBufferInfos = {};
		
		VkWriteDescriptorSetAccelerationStructureKHR	descASInfo = {};
		std::vector<VkAccelerationStructureKHR>			accelerationStructures = {};
		uint32_t										updateSize = 0;
	};
	std::vector<bindings_s>							mBindings;
	std::vector<VkDescriptorBindingFlags>			mBindingFlags;

	LogicalDevice*									mDevice;
	VkDescriptorSetLayout							mLayout;
	VkDescriptorPool								mPool;
	VkDescriptorSet									mSet;
};

RVK_END_NAMESPACE
