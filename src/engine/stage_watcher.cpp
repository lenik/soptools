/*
 * Copyright (C) 2026 Lenik <soptools@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 200809L

#include "engine/stage_watcher.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>

SopStageWatcher::~SopStageWatcher() { stop(); }

void SopStageWatcher::start(const std::string &stage_dir, NotifyFn on_change) {
    stop();
    if (stage_dir.empty() || !on_change) {
        return;
    }
    stage_dir_ = stage_dir;
    on_change_ = std::move(on_change);
    stop_ = false;
    running_ = true;
    thread_ = std::thread([this]() { loop(); });
}

void SopStageWatcher::stop() {
    stop_ = true;
    const int fd = inotify_fd_.exchange(-1);
    if (fd >= 0) {
        ::close(fd); /* unblocks select/read in worker */
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    watch_wd_ = -1;
    running_ = false;
    on_change_ = nullptr;
}

void SopStageWatcher::loop() {
    int fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (fd < 0) {
        running_ = false;
        return;
    }
    inotify_fd_ = fd;
    watch_wd_ = inotify_add_watch(fd, stage_dir_.c_str(),
                                  IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE |
                                      IN_MOVED_FROM);
    if (watch_wd_ < 0) {
        ::close(fd);
        inotify_fd_ = -1;
        running_ = false;
        return;
    }

    alignas(struct inotify_event) char buf[4096];
    bool pending = false;
    auto last_fire = std::chrono::steady_clock::now();

    while (!stop_.load()) {
        const int cur = inotify_fd_.load();
        if (cur < 0) {
            break;
        }
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(cur, &rfds);
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 200000;
        const int ready = select(cur + 1, &rfds, nullptr, nullptr, &tv);
        if (stop_.load() || inotify_fd_.load() < 0) {
            break;
        }
        if (ready > 0 && FD_ISSET(cur, &rfds)) {
            for (;;) {
                const ssize_t n = read(cur, buf, sizeof(buf));
                if (n <= 0) {
                    break;
                }
                pending = true;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (pending &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fire).count() >= 200) {
            pending = false;
            last_fire = now;
            if (on_change_) {
                on_change_();
            }
        }
    }

    const int left = inotify_fd_.exchange(-1);
    if (left >= 0) {
        if (watch_wd_ >= 0) {
            inotify_rm_watch(left, watch_wd_);
            watch_wd_ = -1;
        }
        ::close(left);
    }
    running_ = false;
}
