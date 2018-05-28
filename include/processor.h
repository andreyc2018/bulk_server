#pragma once

#include "command.h"
#include "parser.h"
#include "asyncqueue.h"
#include "writerfactory.h"
#include "counters.h"
#include <vector>
#include <string>
#include <atomic>
#include <memory>

/**
 * @brief The Processor class
 * Reads and process the input commands according to the rules
 */
class Processor
{
    public:
        using reporters_t = std::vector<ReporterUPtr>;

        Processor(int size, WriterFactoryUPtr&& factory);
        ~Processor();

        virtual void add_character(char c);
        virtual void add_string(const std::string& input);
        virtual void add_token(const std::string& token);
        virtual void end_of_stream();
        virtual void run();
        virtual void add_command(const std::string& input);
        virtual bool block_complete() const;
        virtual void start_block();
        virtual bool is_dynamic_block() const;


        void report(std::ostream& out) const;
        void report(Counters& counters) const;

    private:
        size_t full_block_size_;
        std::string token_;
        Parser parser_;
        Block block_;
        reporters_t writers_;
        WriterFactoryUPtr writer_factory_;
        Counters counters_;
        static std::atomic_size_t next_id_;

        void destroy_writers();
        bool collect_token(std::string& token, char l);
};

using ProcessorUPtr = std::unique_ptr<Processor>;
