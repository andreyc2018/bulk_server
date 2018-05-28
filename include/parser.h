#pragma once

#include "interpreter.h"
#include "parserstate.h"
#include "logger.h"

class Processor;

class Parser
{
    public:
        Parser(Processor& processor);
        ~Parser();

        void handle_token(const std::string& token);
        void end_of_stream();

        const expr_t& open_kw() const { return open_kw_; }
        const expr_t& close_kw() const { return close_kw_; }
        const expr_t& command() const { return command_; }

        void set_state(ParserStateUPtr&& state);
        const ParserState& state() const { return *state_.get(); }

        void add_command(const std::string& token);
        bool block_complete() const;
        void increase_level() { ++dynamic_level_; }
        void decrease_level() { --dynamic_level_; }
        int dynamic_level() const { return dynamic_level_; }
        void start_block();
        void notify_run();
        bool is_dynamic_block() const;

    private:
        int dynamic_level_;
        ParserStateUPtr state_;
        Processor& processor_;

        const expr_t open_kw_;
        const expr_t close_kw_;
        const expr_t command_;
};
