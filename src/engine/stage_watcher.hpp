#ifndef ENGINE_STAGE_WATCHER_HPP
#define ENGINE_STAGE_WATCHER_HPP

#include <atomic>
#include <functional>
#include <string>
#include <thread>

/* Linux inotify watcher for stagedir. callback runs on watcher thread —
 * GUI must CallAfter into the UI thread. */
class SopStageWatcher {
public:
    using NotifyFn = std::function<void()>;

    SopStageWatcher() = default;
    ~SopStageWatcher();

    SopStageWatcher(const SopStageWatcher &) = delete;
    SopStageWatcher &operator=(const SopStageWatcher &) = delete;

    void start(const std::string &stage_dir, NotifyFn on_change);
    void stop();
    bool running() const { return running_.load(); }

private:
    void loop();

    std::string stage_dir_;
    NotifyFn on_change_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_{false};
    std::atomic<int> inotify_fd_{-1};
    int watch_wd_ = -1;
};

#endif /* ENGINE_STAGE_WATCHER_HPP */
