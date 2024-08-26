#pragma once
#include <vector>
#include <string>
#include <memory>

namespace tamashii {
	class RenderBackend;
	class RenderBackendImplementation;
	/*
	*	Either use init and then frame on your own,
	*	in case you want to do stuff in between.
	*/
	void init(int argc, char* argv[]);
	void shutdown();
	void whileFrame();
	/*
	*	Or call run which is just init + while(1) {frame}
	*/
	void run(int argc, char* argv[]);

    void registerBackend(const std::shared_ptr<RenderBackend>& aRenderBackend);
	RenderBackendImplementation* getCurrentRenderBackendImplementation();
	std::vector<RenderBackendImplementation*>& getAvailableRenderBackendImplementations();
	RenderBackendImplementation* findBackendImplementation(const std::string& aName);
}

#if defined( _WIN32 )

#elif defined(__APPLE__)

#elif defined( __linux__)
#endif
