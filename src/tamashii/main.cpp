#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <tamashii/tamashii.hpp>

#include "renderer.hpp"

int main(int argc, char* argv[]) {
    tamashii::registerBackend(std::make_shared<VulkanRenderBackendDefault>());
	tamashii::run(argc, argv);
	return 0;
}
