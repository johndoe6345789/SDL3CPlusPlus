// The fs2024 loader threads: every job comes back, a failing build
// comes back marked failed, queued work can be dropped, and shutting
// down never waits on the queue.

#include "services/interfaces/workflow/fs2024/tiles/fs2024_load_pool.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <limits>
#include <stdexcept>
#include <thread>

namespace impl = sdl3cpp::services::impl;

namespace {

constexpr std::size_t kAll = std::numeric_limits<std::size_t>::max();

}  // namespace

TEST(Fs2024LoadPool, EveryJobComesBack) {
    impl::Fs2024LoadPool pool(
        [](const impl::Fs2024TileKey& key) {
            impl::Fs2024PreparedTile tile;
            tile.classSize = key.x;
            return tile;
        },
        4);
    for (int i = 0; i < 40; ++i) pool.Enqueue({i, 0, 14});
    pool.WaitIdle();
    const auto done = pool.Take(kAll);
    ASSERT_EQ(done.size(), 40u);
    for (const auto& tile : done) EXPECT_EQ(tile.classSize, tile.key.x);
}

TEST(Fs2024LoadPool, AFailingBuildComesBackMarkedFailed) {
    impl::Fs2024LoadPool pool(
        [](const impl::Fs2024TileKey&) -> impl::Fs2024PreparedTile {
            throw std::runtime_error("no DEM here");
        },
        1);
    pool.Enqueue({7, 3, 12});
    pool.WaitIdle();
    const auto done = pool.Take(kAll);
    ASSERT_EQ(done.size(), 1u);
    EXPECT_EQ(done[0].key, (impl::Fs2024TileKey{7, 3, 12}));
    EXPECT_EQ(done[0].error, "no DEM here");
}

TEST(Fs2024LoadPool, DropRemovesOnlyWhatHasNotStarted) {
    std::promise<void> release;
    std::shared_future<void> gate = release.get_future().share();
    std::atomic<bool> started{false};
    impl::Fs2024LoadPool pool(
        [&](const impl::Fs2024TileKey&) {
            started = true;
            gate.wait();
            return impl::Fs2024PreparedTile{};
        },
        1);
    for (int i = 0; i < 5; ++i) pool.Enqueue({i, 0, 14});
    while (!started) std::this_thread::yield();
    const auto dropped = pool.Drop(
        [](const impl::Fs2024TileKey& key) { return key.x == 4; });
    EXPECT_EQ(dropped.size(), 3u);  // 1, 2, 3; 0 is running, 4 is kept
    release.set_value();
    pool.WaitIdle();
    EXPECT_EQ(pool.Take(kAll).size(), 2u);
}

TEST(Fs2024LoadPool, ShuttingDownDoesNotWaitOnTheQueue) {
    std::atomic<int> built{0};
    {
        impl::Fs2024LoadPool pool(
            [&](const impl::Fs2024TileKey&) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                ++built;
                return impl::Fs2024PreparedTile{};
            },
            2);
        for (int i = 0; i < 200; ++i) pool.Enqueue({i, 0, 14});
    }
    EXPECT_LT(built.load(), 200);
}
