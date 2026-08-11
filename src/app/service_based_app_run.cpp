#include "app/service_based_app.hpp"
#include "services/interfaces/i_window_service.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_workflow_executor.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"

namespace sdl3cpp::app {

void ServiceBasedApp::Run() {
    logger_->Trace("ServiceBasedApp", "Run", "", "Entering");

    try {
        // Initialize all services
        lifecycleService_->InitializeAll();

        // Create the window
        auto windowService = registry_.GetService<services::IWindowService>();
        auto configService = registry_.GetService<services::IConfigService>();
        if (windowService) {
            services::WindowConfig config;
            if (configService) {
                config.width = configService->GetWindowWidth();
                config.height = configService->GetWindowHeight();
                config.title = configService->GetWindowTitle();
                config.mouseGrab = configService->GetMouseGrabConfig();
            } else {
                config.width = runtimeConfig_.width;
                config.height = runtimeConfig_.height;
                config.title = runtimeConfig_.windowTitle;
                config.mouseGrab = runtimeConfig_.mouseGrab;
            }
            config.resizable = true;
            windowService->CreateWindow(config);
        }

        // Execute bootstrap workflow (graphics init: gpu.init_viewport → gpu.init_renderer → gpu.init)
        if (bootstrapWorkflowLoaded_) {
            logger_->Info("ServiceBasedApp::Run: Executing bootstrap workflow");

            auto workflowExecutor = registry_.GetService<services::IWorkflowExecutor>();
            if (workflowExecutor) {
                services::WorkflowContext bootContext;
                workflowExecutor->Execute(bootstrapWorkflow_, bootContext);
                logger_->Info("ServiceBasedApp::Run: Bootstrap workflow executed successfully");
            } else {
                logger_->Error("ServiceBasedApp::Run: No WorkflowExecutor available for bootstrap");
            }
        } else {
            logger_->Error("ServiceBasedApp::Run: Bootstrap workflow not loaded");
        }

        // NOTE: Graphics initialization already handled by bootstrap workflow
        // (gpu.init_viewport + gpu.init_renderer + gpu.init)
        // Do NOT call InitializeDevice again - would corrupt GPU state!

        // Run the main application loop
        logger_->Info("ServiceBasedApp::Run: About to start main application loop");
        logger_->Info("ServiceBasedApp::Run: applicationLoopService_ is " + std::string(applicationLoopService_ ? "valid" : "NULL"));
        // Note: On macOS, SDL3 Cocoa backend requires event loop on main thread.
        // CrashRecoveryService::ExecuteWithTimeout creates a worker thread, which
        // violates Cocoa's threading requirement. On macOS, we skip the timeout wrapper
        // and rely on heartbeat monitoring (RecordFrameHeartbeat is called by ApplicationLoopService).
        #if defined(__APPLE__)
            // macOS: Run event loop on main thread only (Cocoa requirement)
            logger_->Info("ServiceBasedApp::Run: Calling applicationLoopService_->Run() on macOS");
            if (applicationLoopService_) {
                applicationLoopService_->Run();
                logger_->Info("ServiceBasedApp::Run: applicationLoopService_->Run() returned");
            } else {
                logger_->Error("ServiceBasedApp::Run: applicationLoopService_ is NULL!");
            }
        #else
            // Other platforms: Use full timeout wrapper with worker thread for robustness
            if (crashRecoveryService_) {
                constexpr int kMainLoopTimeoutMs = 24 * 60 * 60 * 1000; // Safety net; heartbeat monitor handles hangs.
                bool success = crashRecoveryService_->ExecuteWithTimeout(
                    [this]() { applicationLoopService_->Run(); },
                    kMainLoopTimeoutMs,
                    "Main Application Loop"
                );

                if (!success) {
                    logger_->Warn("ServiceBasedApp::Run: Main loop stopped by crash recovery, attempting recovery");
                    if (crashRecoveryService_->AttemptRecovery()) {
                        logger_->Info("ServiceBasedApp::Run: Recovery successful, restarting main loop");
                        applicationLoopService_->Run(); // Try again
                    }
                }
            } else {
                // Fallback if no crash recovery service
                applicationLoopService_->Run();
            }
        #endif

        // Shutdown all services
        lifecycleService_->ShutdownAll();

        logger_->Trace("ServiceBasedApp", "Run", "", "Exiting");

    } catch (const std::exception& e) {
        logger_->Error("ServiceBasedApp::Run: Application error: " + std::string(e.what()));

        // Attempt recovery on exception
        if (crashRecoveryService_ && crashRecoveryService_->AttemptRecovery()) {
            logger_->Info("ServiceBasedApp::Run: Recovered from exception");
        } else {
            throw;
        }
    }
}

}  // namespace sdl3cpp::app
