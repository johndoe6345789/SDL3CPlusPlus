#pragma once

namespace sdl3cpp::di {

/**
 * @brief Interface for services that require initialization.
 *
 * Similar to Spring's @PostConstruct, this interface allows services
 * to perform initialization logic after construction and dependency injection.
 *
 * The ServiceRegistry will call Initialize() on all registered services
 * that implement this interface when InitializeAll() is invoked.
 */
class IInitializable {
public:
    virtual ~IInitializable() = default;

    /**
     * @brief Initialize the service.
     *
     * Called once after the service is constructed and all dependencies
     * are injected. This is where you should perform initialization logic
     * such as loading resources, connecting to external services, etc.
     *
     * @throws std::runtime_error if initialization fails
     */
    virtual void Initialize() = 0;
};

/**
 * @brief Interface for services that require cleanup on shutdown.
 *
 * Similar to Spring's @PreDestroy, this interface allows services
 * to perform cleanup logic before destruction.
 *
 * The ServiceRegistry will call Shutdown() on all registered services
 * that implement this interface when ShutdownAll() is invoked,
 * in reverse order of registration.
 */
class IShutdownable {
public:
    virtual ~IShutdownable() = default;

    /**
     * @brief Shutdown the service and release resources.
     *
     * Called once before the service is destroyed. This is where you
     * should perform cleanup logic such as closing connections, releasing
     * resources, saving state, etc.
     *
     * This method should not throw exceptions.
     */
    virtual void Shutdown() noexcept = 0;
};

}  // namespace sdl3cpp::di
