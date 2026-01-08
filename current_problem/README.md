▶ Running: build-ninja/sdl3_app -j config/seed_runtime.json

2026-01-07 16:17:43.041 [INFO] JsonConfigService initialized from config file: /home/rewrich/Documents/GitHub/SDL3CPlusPlus/config/seed_runtime.json

2026-01-07 16:17:43.041 [INFO] ServiceBasedApp::ServiceBasedApp: Setting up SDL
2026-01-07 16:17:43.041 [INFO] ServiceBasedApp::ServiceBasedApp: Registering services
2026-01-07 16:17:43.041 [INFO] JsonConfigService initialized with explicit configuration

2026-01-07 16:17:43.137 [INFO] CrashRecoveryService::SetupSignalHandlers: Signal handlers installed
2026-01-07 16:17:43.137 [INFO] CrashRecoveryService::Initialize: Crash recovery service initialized
2026-01-07 16:17:43.137 [INFO] ServiceBasedApp::ServiceBasedApp: Resolving lifecycle services
2026-01-07 16:17:43.137 [INFO] ServiceBasedApp::ServiceBasedApp: constructor completed

2026-01-07 16:17:43.137 [INFO] Application starting
2026-01-07 16:17:43.137 [INFO] LifecycleService::InitializeAll: Initializing all services

2026-01-07 16:17:43.149 [INFO] SDL audio service initialized successfully

2026-01-07 16:17:43.152 [INFO] Playing background audio: /home/rewrich/Documents/GitHub/SDL3CPlusPlus/scripts/piano.ogg (loop: 1)

2026-01-07 16:17:43.154 [INFO] Script engine service initialized

2026-01-07 16:17:43.156 [INFO] Physics service initialized
2026-01-07 16:17:43.156 [INFO] LifecycleService::InitializeAll: All services initialized

2026-01-07 16:17:43.170 [INFO] PlatformService::FeatureTable
feature	value
platform.pointerBits	64
platform.name	Linux
platform.sdl.version	3002020
platform.sdl.version.major	3
platform.sdl.version.minor	2
platform.sdl.version.micro	20
platform.sdl.revision	release-3.2.20-0-g96292a5b4
platform.cpu.count	12
platform.cpu.cacheLineSize	64
platform.systemRamMB	64198
platform.cpu.hasSSE	true
platform.cpu.hasSSE2	true
platform.cpu.hasSSE3	true
platform.cpu.hasSSE41	true
platform.cpu.hasSSE42	true
platform.cpu.hasAVX	true
platform.cpu.hasAVX2	true
platform.cpu.hasAVX512F	false
platform.cpu.hasNEON	false
platform.cpu.hasARMSIMD	false
platform.cpu.hasAltiVec	false
platform.cpu.hasLSX	false
platform.cpu.hasLASX	false
env.xdgSessionType	x11
env.waylandDisplay	unset
env.x11Display	:0
env.desktopSession	mate
env.xdgCurrentDesktop	MATE
env.xdgRuntimeDir	/run/user/1000
env.sdlVideoDriver	unset
env.sdlRenderDriver	unset
sdl.hint.videoDriver	unset
sdl.hint.renderDriver	unset
sdl.hint.waylandPreferLibdecor	unset
sdl.videoDriverCount	5
sdl.videoDrivers	wayland, x11, offscreen, dummy, evdev
sdl.videoInitialized	true
sdl.videoBackend.supportsWayland	true
sdl.videoBackend.supportsX11	true
sdl.videoBackend.supportsKmsdrm	false
sdl.videoBackend.supportsWindows	false
sdl.videoBackend.supportsCocoa	false
sdl.videoBackend.isWayland	false
sdl.videoBackend.isX11	true
sdl.videoBackend.isKmsdrm	false
sdl.videoBackend.isWindows	false
sdl.videoBackend.isCocoa	false
sdl.currentVideoDriver	x11
sdl.systemTheme	unknown
sdl.renderDriverCount	5
sdl.renderDrivers	opengl, opengles2, vulkan, gpu, software
sdl.render.supportsOpenGL	true
sdl.render.supportsOpenGLES2	true
sdl.render.supportsDirect3D11	false
sdl.render.supportsDirect3D12	false
sdl.render.supportsMetal	false
sdl.render.supportsSoftware	true
sdl.displayCount	1
sdl.primaryDisplayId	1
sdl.displaySummary	0:Odyssey G40B 27"@1920x1080+0+0
sdl.displayError	none
platform.uname.sysname	Linux
platform.uname.release	6.17.12-300.fc43.x86_64
platform.uname.version	#1 SMP PREEMPT_DYNAMIC Sat Dec 13 05:06:24 UTC 2025
platform.uname.machine	x86_64


2026-01-07 16:17:43.239 [INFO] SdlWindowService: Mouse grab config: enabled=true, grabOnClick=true, grabMouseButton=1, releaseKey=27

2026-01-07 16:17:43.295 [INFO] ApplicationLoopService::Run: Starting main loop

2026-01-07 16:17:44.455 [ERROR] BgfxGuiService::CreateProgram: bgfx::createProgram failed to link shaders
2026-01-07 16:17:44.455 [ERROR] BgfxGuiService::InitializeResources: Failed to create GUI shader program

2026-01-07 16:17:44.456 [WARN] BgfxGuiService::PrepareFrame: GUI resources not initialized

2026-01-07 16:17:45.952 [INFO] SdlWindowService: Mouse grab triggered by click (button=1)

2026-01-07 16:17:49.602 [INFO] SdlWindowService: Mouse grab triggered by click (button=1)

2026-01-07 16:17:52.268 [INFO] SdlWindowService: Mouse grab released by escape key

2026-01-07 16:17:53.718 [INFO] ApplicationLoopService::Run: Exiting main loop

2026-01-07 16:17:53.718 [INFO] LifecycleService::ShutdownAll: Shutting down all services
2026-01-07 16:17:53.719 [INFO] Physics service shutdown

2026-01-07 16:17:53.775 [INFO] Script engine service shutdown

2026-01-07 16:17:53.872 [INFO] SDL audio service shutdown

2026-01-07 16:17:53.937 [INFO] LifecycleService::ShutdownAll: All services shutdown
2026-01-07 16:17:53.937 [INFO] CrashRecoveryService::RemoveSignalHandlers: Signal handlers removed
2026-01-07 16:17:53.937 [INFO] CrashRecoveryService::Shutdown: Crash recovery service shutdown
2026-01-07 16:17:53.937 [INFO] CrashRecoveryService::Shutdown: Crash recovery service shutdown


✓ Process completed successfully