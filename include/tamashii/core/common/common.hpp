#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/render/render_system.hpp>
#include <tamashii/core/common/window_thread.hpp>
#include <tamashii/core/platform/system.hpp>

#include <thread>
#include <map>

T_BEGIN_NAMESPACE
struct screenshotInfo_s;
class Common
{
public:
                                static Common& getInstance()
                                {
                                    static Common instance;
                                    return instance;
                                }
                                Common(Common const&) = delete;
    void                        operator=(Common const&) = delete;

	void				        init(int aArgc, char* aArgv[], const char* aCmdline);
	void				        shutdown();
    void                        queueShutdown();

    bool                        frame();
    
    ScreenshotInfo_s            screenshot();

                                
    void                        newScene();
    void                        openScene(const std::string& aFile);
    void                        addScene(const std::string& aFile) const;
    void                        addModel(const std::string& aFile) const;
    void                        addLight(const std::string& aFile) const;
    void                        exportScene(const std::string& aOutputFile, uint32_t aSettings) const;
    void                        reloadBackendImplementation() const;
    void                        changeBackendImplementation(int aIdx);
    bool                        changeBackendImplementation(const char* name);

    void                        clearCache() const;
    void                        queueScreenshot(const std::filesystem::path& aOut, uint32_t aScreenshotSettings = 0);

								
    void                        intersectScene(IntersectionSettings aSettings, Intersection* aHitInfo);

	RenderSystem*               getRenderSystem();

	WindowThread*               getMainWindow();
    void                        openMainWindow();
    void                        closeMainWindow();
    IntersectionSettings&       intersectionSettings();

								
    static void                 openFileDialogOpenScene();
    static void                 openFileDialogAddScene();
    static void                 openFileDialogOpenModel();
    static void                 openFileDialogOpenLight();
    static void                 openFileDialogExportScene(uint32_t aSettings);

    void                        initLogger();
private:
								Common();
                                ~Common() = default;

	void				        shutdownIntern();
    bool                        frameIntern();

    void                        setStartupVariables(const std::string& aMatch = "");

    void                        processInputs();

    void                        paintOnMesh(const Intersection *aHitInfo) const;

    SystemInfo_s                mSystemInfo;
    std::map<std::string, std::string> mConfigCache;

    std::thread                 mFileWatcherThread;
    std::thread                 mFrameThread;
    std::thread                 mRenderThread;
    WindowThread                mWindow;
    bool                        mShutdown;

    struct Inter
    {
        bool                    mShutdown;
        bool                    mOpenMainWindow;
        bool                    mCloseMainWindow;
    } mIntern;

    RenderSystem                mRenderSystem;

    IntersectionSettings        mIntersectionSettings;
    DrawInfo			        mDrawInfo;
    std::filesystem::path       mScreenshotPath;
    uint32_t                    mScreenshotSettings;

    
    std::mutex                  mRenderFrameMutex;

    static std::atomic_bool     mFileDialogRunning;

    bool didInitLogger{ false };
};

T_END_NAMESPACE