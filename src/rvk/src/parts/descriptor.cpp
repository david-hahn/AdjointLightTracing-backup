#include <rvk/parts/descriptor.hpp>
#include "rvk_private.hpp"
RVK_USE_NAMESPACE

Descriptor::Descriptor(LogicalDevice* aDevice) : mBindings({}), mBindingFlags({}), mDevice(aDevice),
mLayout(VK_NULL_HANDLE), mPool(VK_NULL_HANDLE), mSet(VK_NULL_HANDLE)
{
}

Descriptor::~Descriptor() {
	destroy();
}

void Descriptor::reserve(const int aReserve)
{ mBindings.reserve(aReserve); mBindingFlags.reserve(aReserve); }

void Descriptor::addImageSampler(const uint32_t aBinding, const VkShaderStageFlags aStages, const uint32_t aCount) {
	pushImage(aBinding, aStages, aCount, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
void Descriptor::addStorageImage(const uint32_t aBinding, const VkShaderStageFlags aStages, const uint32_t aCount) {
	pushImage(aBinding, aStages, aCount, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_IMAGE_LAYOUT_GENERAL);
}
void Descriptor::addUniformBuffer(const uint32_t aBinding, const VkShaderStageFlags aStages, const uint32_t aCount) {
	pushBuffer(aBinding, aStages, aCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}
void Descriptor::addStorageBuffer(const uint32_t aBinding, const VkShaderStageFlags aStages, const uint32_t aCount) {
	pushBuffer(aBinding, aStages, aCount, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
}
void Descriptor::addAccelerationStructureKHR(const uint32_t aBinding, const VkShaderStageFlags aStages, const uint32_t aCount) {
	bindings_s b = {};
	b.binding = { aBinding, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, aCount, static_cast<VkShaderStageFlags>(aStages), VK_NULL_HANDLE };
	b.updateSize = aCount;
	b.descASInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	b.descASInfo.accelerationStructureCount = aCount;
	b.accelerationStructures.resize(aCount);
	mBindings.push_back(b);
	mBindingFlags.resize(mBindings.size(), 0);
}
void Descriptor::setImage(const uint32_t aBinding, const Image* const aImage, const uint32_t aPosition)
{
	for (uint32_t i = 0; i < mBindings.size(); ++i) {
		if (mBindings[i].binding.binding == aBinding) {
			mBindings[i].descImageInfos[aPosition].sampler = aImage->mSampler;
			mBindings[i].descImageInfos[aPosition].imageView = aImage->mImageView;
			return;
		}
	}
}
void Descriptor::setBuffer(const uint32_t aBinding, const Buffer* const aBuffer, const uint32_t aPosition) {
	for (uint32_t i = 0; i < mBindings.size(); ++i) {
		if (mBindings[i].binding.binding == aBinding) {
			mBindings[i].descBufferInfos[aPosition].buffer = aBuffer->mBuffer;
			mBindings[i].descBufferInfos[aPosition].offset = 0;
			mBindings[i].descBufferInfos[aPosition].range = VK_WHOLE_SIZE;
			return;
		}
	}
}
void Descriptor::setAccelerationStructureKHR(const uint32_t aBinding, const TopLevelAS* const aTlas, const uint32_t aPosition)
{
	for (uint32_t i = 0; i < mBindings.size(); ++i) {
		if (mBindings[i].binding.binding == aBinding) {
			mBindings[i].accelerationStructures[aPosition] = aTlas->mAs;
			mBindings[i].descASInfo.pAccelerationStructures = &mBindings[i].accelerationStructures[0];
			return;
		}
	}
}
void Descriptor::setUpdateSize(const uint32_t aBinding, const uint32_t aUpdateSize) {
	for (uint32_t i = 0; i < mBindings.size(); ++i) {
		if (mBindings[i].binding.binding == aBinding) {
			mBindings[i].updateSize = aUpdateSize;
			return;
		}
	}
}
void Descriptor::setBindingFlags(const uint32_t aBinding, const VkDescriptorBindingFlags aFlags) {
	for (uint32_t i = 0; i < mBindings.size(); ++i) {
		if (mBindings[i].binding.binding == aBinding) {
			mBindingFlags[i] = aFlags;
		}
	}
}
void Descriptor::update() const
{
	std::vector<VkWriteDescriptorSet> descWrite(mBindings.size());

	for (uint32_t j = 0; j < descWrite.size(); ++j) {
		descWrite[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descWrite[j].dstSet = mSet;
		descWrite[j].dstBinding = mBindings[j].binding.binding;
		descWrite[j].descriptorCount = mBindings[j].updateSize;
		descWrite[j].descriptorType = mBindings[j].binding.descriptorType;

		switch (mBindings[j].binding.descriptorType) {
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			descWrite[j].pImageInfo = &mBindings[j].descImageInfos[0];
			assert(mBindings[j].descImageInfos[0].imageView != NULL);
			break;
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			descWrite[j].pBufferInfo = &mBindings[j].descBufferInfos[0];
			assert(mBindings[j].descBufferInfos[0].buffer != NULL);
			break;
		case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
			descWrite[j].pNext = &mBindings[j].descASInfo;
			assert(descWrite[j].pNext != NULL);
			break;
		}
	}
	mDevice->vk.UpdateDescriptorSets(mDevice->getHandle(), static_cast<uint32_t>(descWrite.size()), &descWrite[0], 0, VK_NULL_HANDLE);
}

VkDescriptorSet Descriptor::getHandle() const
{	return mSet; }

VkDescriptorSetLayout Descriptor::getLayout() const
{	return mLayout;	}


void Descriptor::finish(const bool aWithUpdate) {
	SET_DEVICE(mDevice)
	createDescriptorSetLayout();
	createDescriptorPool();
	createDescriptorSet();
	if (aWithUpdate) update();
}

void Descriptor::destroy()
{
	if (mSet != VK_NULL_HANDLE) {
		mDevice->vk.FreeDescriptorSets(mDevice->getHandle(), mPool, 1, &mSet);
		mSet = VK_NULL_HANDLE;
	}
	if (mLayout != VK_NULL_HANDLE) {
		mDevice->vk.DestroyDescriptorSetLayout(mDevice->getHandle(), mLayout, VK_NULL_HANDLE);
		mLayout = VK_NULL_HANDLE;
	}
	if (mPool != VK_NULL_HANDLE) {
		mDevice->vk.DestroyDescriptorPool(mDevice->getHandle(), mPool, VK_NULL_HANDLE);
		mPool = VK_NULL_HANDLE;
	}

	/*for (bindings_s& b : mBindings) {
		b.accelerationStructures.clear();
		b = {};
		b.descASInfo = {};
		b.descBufferInfos.clear();
		b.descImageInfos.clear();
		b.updateSize = 0;
	}*/

	mBindings.clear();
	mBindings = {};
	mBindingFlags.clear();
	mBindingFlags = {};
}
/**
* Private
**/
void Descriptor::createDescriptorSetLayout() {
	if (mLayout != VK_NULL_HANDLE) {
		mDevice->vk.DestroyDescriptorSetLayout(mDevice->getHandle(), mLayout, VK_NULL_HANDLE);
		mLayout = VK_NULL_HANDLE;
	}
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	layoutCreateInfo.pNext = VK_NULL_HANDLE;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(mBindingFlags.size());
	layoutCreateInfo.pBindingFlags = toArrayPointer(mBindingFlags);

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings(mBindings.size());
	for (uint32_t i = 0; i < mBindings.size(); i++) { layoutBindings[i] = mBindings[i].binding; }

	VkDescriptorSetLayoutCreateInfo descLayoutInfo = {};
	descLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descLayoutInfo.pNext = mBindingFlags.empty() ? VK_NULL_HANDLE : &layoutCreateInfo;
	descLayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	descLayoutInfo.pBindings = toArrayPointer(layoutBindings);
	VK_CHECK(mDevice->vk.CreateDescriptorSetLayout(mDevice->getHandle(), &descLayoutInfo, NULL, &mLayout), " failed to create Descriptor Set Layout!");
}
void Descriptor::createDescriptorPool() {
	if (mPool != VK_NULL_HANDLE) {
		mDevice->vk.DestroyDescriptorPool(mDevice->getHandle(), mPool, VK_NULL_HANDLE);
		mPool = VK_NULL_HANDLE;
	}
	std::vector<VkDescriptorPoolSize> poolSizes(mBindings.size());
	for (uint32_t i = 0; i < mBindings.size(); i++) {
		poolSizes[i].type = mBindings[i].binding.descriptorType;
		poolSizes[i].descriptorCount = mBindings[i].binding.descriptorCount;
	}

	VkDescriptorPoolCreateInfo descPoolInfo = {};
	descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descPoolInfo.maxSets = 1;
	descPoolInfo.poolSizeCount = static_cast<uint32_t>(mBindings.size());
	descPoolInfo.pPoolSizes = toArrayPointer(poolSizes);
	VK_CHECK(mDevice->vk.CreateDescriptorPool(mDevice->getHandle(), &descPoolInfo, NULL, &mPool), " failed to create Descriptor Pool!");
}
void Descriptor::createDescriptorSet() {
	if (mSet != VK_NULL_HANDLE) {
		mDevice->vk.FreeDescriptorSets(mDevice->getHandle(), mPool, 1, &mSet);
		mSet = VK_NULL_HANDLE;
	}
	
	uint32_t count = 0;
	for (uint32_t i = 0; i < mBindings.size(); ++i) {
		if ((mBindingFlags[i] & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT) == VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT) {
			count = mBindings[i].binding.descriptorCount;
		}
	}
	const VkDescriptorSetVariableDescriptorCountAllocateInfo varDescCountAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO, VK_NULL_HANDLE, 1, &count };

	VkDescriptorSetAllocateInfo descSetAllocInfo = {};
	descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocInfo.pNext = count > 0 ? &varDescCountAllocInfo : VK_NULL_HANDLE;
	descSetAllocInfo.descriptorPool = mPool;
	descSetAllocInfo.descriptorSetCount = 1;
	descSetAllocInfo.pSetLayouts = &mLayout;
	VK_CHECK(mDevice->vk.AllocateDescriptorSets(mDevice->getHandle(), &descSetAllocInfo, &mSet), " failed to allocate Descriptor Set!");
}

void Descriptor::pushImage(const uint32_t aBinding, const VkShaderStageFlags aStages, const uint32_t aCount, const VkDescriptorType aType, const VkImageLayout aLayout)
{
	for (const auto& binding : mBindings) {
		if (binding.binding.binding == aBinding) {
			Logger::critical("Vulkan Describtor: binding " + std::to_string(aBinding) + " aalready used for different attachment");
			return;
		}
	}

	const VkDescriptorImageInfo imageInfo = { VK_NULL_HANDLE, VK_NULL_HANDLE, aLayout };
	bindings_s b = {};
	b.binding = { aBinding, aType, aCount, static_cast<VkShaderStageFlags>(aStages), VK_NULL_HANDLE };
	b.descImageInfos.reserve(aCount);
	for (uint32_t i = 0; i < aCount; i++) b.descImageInfos.push_back(imageInfo);
	b.updateSize = aCount;
	mBindings.push_back(b);
	mBindingFlags.resize(mBindings.size(), 0);
}

void Descriptor::pushBuffer(const uint32_t aBinding, const VkShaderStageFlags aStages, const uint32_t aCount, const VkDescriptorType aType)
{
	for (const auto& binding : mBindings) {
		if (binding.binding.binding == aBinding) {
			Logger::critical("Vulkan Describtor: binding " + std::to_string(aBinding) + " already used for different attachment");
			return;
		}
	}

	const VkDescriptorBufferInfo bufferInfo = { VK_NULL_HANDLE, 0, 0 };
	bindings_s b = {};
	b.binding = { aBinding, aType, aCount, static_cast<VkShaderStageFlags>(aStages), VK_NULL_HANDLE };
	b.descBufferInfos.reserve(aCount);
	for (int i = 0; i < aCount; i++) b.descBufferInfos.push_back(bufferInfo);
	b.updateSize = aCount;
	mBindings.push_back(b);
	mBindingFlags.resize(mBindings.size(), 0);
}
