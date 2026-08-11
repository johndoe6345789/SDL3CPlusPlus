#pragma once

#include "di/lifecycle.hpp"
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::di {

/**
 * @brief Manual dependency injection container (similar to Spring's ApplicationContext).
 *
 * ServiceRegistry manages service lifecycle and provides dependency injection
 * functionality. Services are registered by interface type and retrieved by
 * interface, allowing for loose coupling and testability.
 *
 * Example usage:
 * @code
 * ServiceRegistry registry;
 *
 * // Register services with their dependencies
 * registry.RegisterService<IConfigService, JsonConfigService>("config.json");
 * registry.RegisterService<IWindowService, SdlWindowService>(
 *     registry.GetService<IConfigService>()
 * );
 *
 * // Initialize all services in dependency order
 * registry.InitializeAll();
 *
 * // Use services
 * auto window = registry.GetService<IWindowService>();
 * window->CreateWindow({800, 600, "My App", true});
 *
 * // Shutdown all services in reverse order
 * registry.ShutdownAll();
 * @endcode
 */
class ServiceRegistry {
public:
    ServiceRegistry() = default;
    ~ServiceRegistry() = default;

    // Non-copyable, non-movable
    ServiceRegistry(const ServiceRegistry&) = delete;
    ServiceRegistry& operator=(const ServiceRegistry&) = delete;
    ServiceRegistry(ServiceRegistry&&) = delete;
    ServiceRegistry& operator=(ServiceRegistry&&) = delete;

    /**
     * @brief Register a service implementation by interface type.
     *
     * Creates an instance of Implementation and stores it as Interface.
     * All constructor arguments are forwarded to the implementation.
     *
     * If the implementation inherits from IInitializable or IShutdownable,
     * the corresponding lifecycle methods will be called during
     * InitializeAll() and ShutdownAll().
     *
     * @tparam Interface The interface type (pure virtual base class)
     * @tparam Implementation The concrete implementation type
     * @tparam Args Constructor argument types (deduced)
     * @param args Constructor arguments for the implementation
     * @throws std::runtime_error if a service of this interface type is already registered
     */
    template<typename Interface, typename Implementation, typename... Args>
    void RegisterService(Args&&... args);

    /**
     * @brief Get a service by interface type.
     *
     * @tparam Interface The interface type to retrieve
     * @return std::shared_ptr<Interface> Shared pointer to the service
     * @throws std::runtime_error if no service of this type is registered
     */
    template<typename Interface>
    std::shared_ptr<Interface> GetService();

    /**
     * @brief Get a service by interface type (const version).
     *
     * @tparam Interface The interface type to retrieve
     * @return std::shared_ptr<const Interface> Shared pointer to the service
     * @throws std::runtime_error if no service of this type is registered
     */
    template<typename Interface>
    std::shared_ptr<const Interface> GetService() const;

    /**
     * @brief Check if a service of the given interface type is registered.
     *
     * @tparam Interface The interface type to check
     * @return true if registered, false otherwise
     */
    template<typename Interface>
    bool HasService() const;

    /**
     * @brief Initialize all registered services in registration order.
     *
     * Calls Initialize() on all services that implement IInitializable.
     * Services should be registered in dependency order (dependencies first).
     *
     * @throws std::runtime_error if already initialized
     * @throws Any exception thrown by service Initialize() methods
     */
    void InitializeAll();

    /**
     * @brief Shutdown all registered services in reverse registration order.
     *
     * Calls Shutdown() on all services that implement IShutdownable.
     * This method does not throw exceptions (shutdown methods must be noexcept).
     */
    void ShutdownAll() noexcept;

    /**
     * @brief Check if services have been initialized.
     *
     * @return true if InitializeAll() has been called, false otherwise
     */
    bool IsInitialized() const noexcept { return initialized_; }

private:
    // Type-erased service storage (void* actually holds std::shared_ptr<T>)
    std::unordered_map<std::type_index, std::shared_ptr<void>> services_;

    // Initialization functions (called in registration order)
    std::vector<std::function<void()>> initFunctions_;

    // Shutdown functions (called in reverse registration order)
    std::vector<std::function<void()>> shutdownFunctions_;

    // Initialization state
    bool initialized_ = false;
};

// Template implementation

template<typename Interface, typename Implementation, typename... Args>
void ServiceRegistry::RegisterService(Args&&... args) {
    const std::type_index typeIndex(typeid(Interface));

    // Check if already registered
    if (services_.find(typeIndex) != services_.end()) {
        throw std::runtime_error(
            std::string("Service already registered: ") + typeid(Interface).name()
        );
    }

    // Create the implementation instance
    auto implementation = std::make_shared<Implementation>(std::forward<Args>(args)...);

    // Store as interface type (type-erased as void*)
    services_[typeIndex] = std::static_pointer_cast<void>(
        std::static_pointer_cast<Interface>(implementation)
    );

    // Register initialization if implements IInitializable
    if (auto initializable = std::dynamic_pointer_cast<IInitializable>(implementation)) {
        initFunctions_.push_back([initializable]() {
            initializable->Initialize();
        });
    }

    // Register shutdown if implements IShutdownable
    if (auto shutdownable = std::dynamic_pointer_cast<IShutdownable>(implementation)) {
        shutdownFunctions_.push_back([shutdownable]() {
            shutdownable->Shutdown();
        });
    }
}

template<typename Interface>
std::shared_ptr<Interface> ServiceRegistry::GetService() {
    const std::type_index typeIndex(typeid(Interface));

    auto it = services_.find(typeIndex);
    if (it == services_.end()) {
        throw std::runtime_error(
            std::string("Service not found: ") + typeid(Interface).name()
        );
    }

    return std::static_pointer_cast<Interface>(it->second);
}

template<typename Interface>
std::shared_ptr<const Interface> ServiceRegistry::GetService() const {
    const std::type_index typeIndex(typeid(Interface));

    auto it = services_.find(typeIndex);
    if (it == services_.end()) {
        throw std::runtime_error(
            std::string("Service not found: ") + typeid(Interface).name()
        );
    }

    return std::static_pointer_cast<const Interface>(it->second);
}

template<typename Interface>
bool ServiceRegistry::HasService() const {
    const std::type_index typeIndex(typeid(Interface));
    return services_.find(typeIndex) != services_.end();
}

}  // namespace sdl3cpp::di
