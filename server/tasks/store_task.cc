#include "store_task.hh"
#include "rust/src/lib.rs.h"
#include "seastar/core/sleep.hh"

namespace rust {

void StoreTask::schedule_me() {
    if (!_scheduled) {
        seastar::schedule(this);
        _scheduled = true;
    }
}

void StoreTask::run_and_dispose() noexcept {
    _scheduled = false;
    if (rust::poll_store_future(*this)) {
        this->_pr.set_value();
        delete this;
    }
}

StoreFuture& StoreTask::get_store_fut() {
    return *_rfut;
}

StoreTask::StoreTask(RustStorage* rust_storage, std::string& key, std::string& val) : continuation_base_with_promise(seastar::promise<>()) {
    printf("Here I am: %p\n", this);
    _rfut = rust::create_store_future(rust_storage, key, val);
}

StoreTask::~StoreTask() {
    rust::delete_store_future(_rfut);
}

seastar::future<> StoreTask::get_future() {
    return _pr.get_future();
}

void wake_rust_task(StoreTask& task) {
    printf("Task: %p\n", &task);
    task.schedule_me();
}

void schedule_callback_after_one_second(rust::Fn<void(StoreFuture*)> fn, StoreFuture* data) {
    (void)seastar::sleep(std::chrono::seconds(1)).then([fn, data] {
        fn(data);
    });
}

} // namespace rust
