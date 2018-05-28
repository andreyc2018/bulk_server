/**
 * @file parserstate.h
 * @image html parser_state_machine.png
 */

#pragma once
#include "interpreter.h"
#include <iostream>
#include <memory>
#include <string>

/**
 * Parser has a state and calls Expression according the state
 * in a state handler.
 */

class Parser;

class ParserState
{
    public:
        enum class Result : bool { Stop, Continue };
        ParserState(const std::string& name) : name_(name) {}
        virtual ~ParserState() {}

        const std::string& name() const { return name_; }
        virtual Result handle(Parser& ctx, const std::string& input) = 0;
        virtual Result end_of_stream(Parser&) { return Result::Stop; };
        virtual bool is_dynamic_block() const { return false; }

    protected:
        const std::string name_;
};

using ParserStateUPtr = std::unique_ptr<ParserState>;

class StartingBlock : public ParserState
{
    public:
        StartingBlock() : ParserState(__func__) {}

        Result handle(Parser& ctx, const std::string& input) override;
};

class ExpectingDynamicCommand : public ParserState
{
    public:
        ExpectingDynamicCommand() : ParserState(__func__) {}

        Result handle(Parser& ctx, const std::string& input) override;
        bool is_dynamic_block() const override { return true; }
};

class ExpectingStaticCommand : public ParserState
{
    public:
        ExpectingStaticCommand() : ParserState(__func__) {}

        Result handle(Parser& ctx, const std::string& input) override;
};

class CollectingDynamicBlock : public ParserState
{
    public:
        CollectingDynamicBlock() : ParserState(__func__) {}

        Result handle(Parser& ctx, const std::string& input) override;
        bool is_dynamic_block() const override { return true; }
};

class CollectingStaticBlock : public ParserState
{
    public:
        CollectingStaticBlock() : ParserState(__func__) {}

        Result handle(Parser& ctx, const std::string& input) override;
        Result end_of_stream(Parser& ctx) override;
};

class DoneBlock : public ParserState
{
    public:
        DoneBlock() : ParserState("DoneBlock") {}

        Result handle(Parser& ctx, const std::string& input) override;
};
