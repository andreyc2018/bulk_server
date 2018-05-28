#include "async.h"
#include "async_internal.h"
#include "logger.h"
#include <mutex>

namespace async {

std::mutex mutex;

namespace details {

async::details::AsyncLibrary* g_library = &details::Async::instance();

void set_library(AsyncLibrary* library)
{
    g_library = library;
}

void reset_library()
{
    g_library = &details::Async::instance();
}

}

handle_t connect(std::size_t bulk)
{
    if (bulk < 1) {
        return details::InvalidHandle;
    }

    std::lock_guard<std::mutex> g(mutex);
    return details::g_library->open_processor(bulk);
}

void receive(handle_t handle, const char *data, std::size_t size)
{
    if (handle == details::InvalidHandle || data == nullptr || size == 0) {
        return;
    }
    std::string input(data, size);
    details::g_library->process_input(handle, input);
}

void disconnect(handle_t handle)
{
    if (handle == details::InvalidHandle) {
        return;
    }
    std::lock_guard<std::mutex> g(mutex);
    details::g_library->close_processor(handle);
}

}
