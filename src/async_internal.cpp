#include "async_internal.h"

using namespace async;
using namespace async::details;

std::atomic<handle_t> AsyncLibrary::next_id_ { (handle_t)1 };

AsyncLibrary::AsyncLibrary()
    : preprocessor_(*this)
    , console_q_()
    , console_(console_q_)
    , file_q_()
    , file_1_(file_q_, 1)
    , file_2_(file_q_, 2)
    , bulk_(0)
{
    next_id_.store(reinterpret_cast<handle_t>(1), std::memory_order_relaxed);
    console_.run();
    file_1_.run();
    file_2_.run();
}

AsyncLibrary::~AsyncLibrary()
{
    close_processor(CommonProcessor);

    Message msg { MessageId::EndOfStream, "", { "", 0 } };
    console_.queue().push(msg);
    file_1_.queue().push(msg);
    file_2_.queue().push(msg);

    console_.wait();
    file_1_.wait();
    file_2_.wait();

    report(std::cout);
    console_.report(std::cout);
    file_1_.report(std::cout);
    file_2_.report(std::cout);
}

handle_t AsyncLibrary::open_processor(size_t bulk)
{
    if (bulk_ == 0) {
        preprocessor_.open_processor(CommonProcessor, bulk,
                                     console_q_, file_q_);
        bulk_ = bulk;
        ++async_counters_.procesors;
    }
    handle_t id = next_id_.fetch_add(1, std::memory_order_relaxed);
    preprocessor_.open_processor(id, bulk, console_q_, file_q_);
    ++async_counters_.procesors;
    return id;
}

void AsyncLibrary::process_input(handle_t id, const std::string& input)
{
    preprocessor_.parse_input(id, input);
}

void AsyncLibrary::close_processor(handle_t id)
{
    preprocessor_.close_processor(id, counters_);
}

void AsyncLibrary::timeout(handle_t id)
{
    preprocessor_.timeout(id);
}

void AsyncLibrary::set_bulk(std::size_t bulk)
{
    bulk_ = bulk;
}

void AsyncLibrary::report(std::ostream& out) const
{
    out << "processors - " << async_counters_.procesors << "\n";
    out << "main поток - "
        << counters_.lines << " строк, "
        << counters_.commands << " команд, "
        << counters_.blocks << " блок\n";
}
