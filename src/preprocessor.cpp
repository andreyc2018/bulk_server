#include "preprocessor.h"
#include "async_internal.h"

Preprocessor::Preprocessor(async::details::AsyncLibrary& lib)
    : library_(lib)
{
}

void Preprocessor::open_processor(async::handle_t id, size_t bulk,
                                  MessageQueue& console_q, MessageQueue& file_q)
{
    auto twf = std::make_unique<ThreadWriterFactory>(console_q, file_q);
    auto p = std::make_unique<Processor>(bulk, std::move(twf));
    processors_.emplace(id, std::move(p));
}

void Preprocessor::close_processor(async::handle_t id, Counters& counters)
{
    auto it = processors_.find(id);
    if (it != processors_.end()) {
        if (processors_[id]) {
            processors_[id]->end_of_stream();
            processors_[id]->report(counters);
            it = processors_.erase(it);
        }
    }
}

void Preprocessor::parse_input(async::handle_t id,
                               const std::string& data)
{
    async::handle_t h = async::details::CommonProcessor;
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i] == '{') {
            processors_[id]->add_character(data[i]);
            ++i;
            processors_[id]->add_character(data[i]);
        }
        else if (processors_[id]->is_dynamic_block()) {
            processors_[id]->add_character(data[i]);
        }
        else {
            processors_[h]->add_character(data[i]);
        }
    }
}
