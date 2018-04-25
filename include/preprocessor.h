#pragma once

#include "async.h"
#include "processor.h"
#include <set>
#include <regex>
#include <string>
#include <iostream>

using processors_t = std::map<async::handle_t, ProcessorUPtr>;
using dynamic_block_t = std::set<async::handle_t>;

namespace async {
namespace details {
class AsyncLibrary;
}}

class Preprocessor
{
    public:
        Preprocessor(async::details::AsyncLibrary& lib);

        void open_processor(async::handle_t id, size_t bulk,
                            MessageQueue& console_q, MessageQueue& file_q);
        void close_processor(async::handle_t id, Counters& counters);

        virtual void parse_input(async::handle_t id,
                                 const std::string& data);
    private:
        async::details::AsyncLibrary& library_;
        processors_t processors_;

        std::regex eol_re;

        void parse_block(async::handle_t id,
                         std::sregex_token_iterator& it);
};
