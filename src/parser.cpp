#include "parser.h"
#include "parserstate.h"
#include "processor.h"
#include "logger.h"

Parser::Parser(Processor& processor)
    : dynamic_level_(0)
    , state_(std::make_unique<StartingBlock>())
    , processor_(processor)
    , open_kw_(std::make_shared<term_t>("\\{"))
    , close_kw_(std::make_shared<term_t>("\\}"))
    , command_(std::make_shared<term_t>("^$|[^{}]+"))
{
}

Parser::~Parser()
{
}

void Parser::handle_token(const std::string& token)
{
    while (state_->handle(*this, token) == ParserState::Result::Continue) {}
}

void Parser::end_of_stream()
{
    if (state_->end_of_stream(*this) == ParserState::Result::Continue) {
        while (state_->handle(*this, "") == ParserState::Result::Continue) {}
    }
}

void Parser::set_state(ParserStateUPtr&& state)
{
    std::string from = state_->name();

    state_.swap(state);

    gLogger->debug("from state {} to state {}", from, state_->name());
}

void Parser::add_command(const std::string& token)
{
    processor_.add_command(token);
}

bool Parser::block_complete() const
{
    return processor_.block_complete();
}

void Parser::start_block()
{
    processor_.start_block();
}

void Parser::notify_run()
{
    processor_.run();
}

bool Parser::is_dynamic_block() const
{
    return state_->is_dynamic_block();
}
