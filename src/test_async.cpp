#include "command.h"
#include "interpreter.h"
#include "processor.h"
#include "parserstate.h"
#include "preprocessor.h"
#include "async.h"
#include "async_internal.h"
#include "logger.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::_;

TEST(Bulk, Interpreter)
{
    expr_t open_kw = std::make_shared<term_t>("\\{");
    expr_t close_kw = std::make_shared<term_t>("\\}");
    expr_t command = std::make_shared<term_t>("^$|[^{}]+");

    EXPECT_TRUE(open_kw->interpret("{"));
    EXPECT_TRUE(!open_kw->interpret("{{"));
    EXPECT_TRUE(!open_kw->interpret("}"));
    EXPECT_TRUE(!open_kw->interpret("zxcv"));
    EXPECT_TRUE(close_kw->interpret("}"));
    EXPECT_TRUE(!close_kw->interpret("}}"));
    EXPECT_TRUE(!close_kw->interpret("{"));
    EXPECT_TRUE(!close_kw->interpret("world"));
    EXPECT_TRUE(!command->interpret("{"));
    EXPECT_TRUE(command->interpret("hello"));
    EXPECT_TRUE(command->interpret(""));

    expr_t start_block = std::make_shared<start_block_t>(open_kw, command);
    EXPECT_TRUE(start_block->interpret("{"));
    EXPECT_TRUE(!start_block->interpret("{{"));
    EXPECT_TRUE(!start_block->interpret("}"));
    EXPECT_TRUE(start_block->interpret("qwerty"));

    expr_t end_block = std::make_shared<end_block_t>(open_kw, command, close_kw);
    EXPECT_TRUE(end_block->interpret("{"));
    EXPECT_TRUE(end_block->interpret("}"));
    EXPECT_TRUE(end_block->interpret("asdfg"));
}

TEST(Bulk, Block)
{
    auto b1 = Block::create();

    auto c1 = Command::create("c1");
    b1.add_command(c1);

    auto c2 = Command::create("c2");
    b1.add_command(c2);

    std::stringstream ss;
    b1.run(ss);

    std::cout << ss.str();

    EXPECT_EQ("bulk: c1, c2\n", ss.str());
}

TEST(Bulk, Observer)
{
    LocalWriterFactory factory;
    std::vector<ReporterUPtr> writers;
    auto f1 = factory.create_file_writer("test1.txt");
    auto f2 = factory.create_file_writer("test2.txt");
    auto c1 = factory.create_console_writer();
    auto c2 = factory.create_console_writer();
    writers.push_back(std::make_unique<Reporter>(f1));
    writers.push_back(std::make_unique<Reporter>(c1));
    writers.push_back(std::make_unique<Reporter>(c2));
    writers.push_back(std::make_unique<Reporter>(f2));

    BlockMessage msg { "hello\n", 1 };
    for (const auto& o : writers) {
        o->update(msg);
    }

    std::fstream file1("test1.txt");
    std::string result;
    file1 >> result;
    EXPECT_EQ("hello", result);
    unlink("test1.txt");

    std::fstream file2("test1.txt");
    file2 >> result;
    EXPECT_EQ("hello", result);
    unlink("test2.txt");
}

class MockProcessor : public Processor
{
    public:
        MockProcessor() : Processor(0, std::make_unique<NonWriterFactory>()) {}

        MOCK_METHOD0(run, void());
        MOCK_METHOD1(add_command, void(const std::string&));
        MOCK_CONST_METHOD0(block_complete, bool());
        MOCK_METHOD0(start_block, void());
};

TEST(Processor, StartingToCollectingRun)
{
    MockProcessor proc;
    EXPECT_CALL(proc, add_command(_)).Times(2);
    EXPECT_CALL(proc, block_complete()).WillRepeatedly(Return(false));
    EXPECT_CALL(proc, start_block());
    Parser p(proc);
    p.handle_token("cmd1");
    p.handle_token("cmd2");
    EXPECT_EQ("CollectingStaticBlock", p.state().name());
    EXPECT_EQ(0, p.dynamic_level());
}

TEST(Processor, StartingToExpectingToCollecting)
{
    MockProcessor proc;
    EXPECT_CALL(proc, add_command("cmd1"));
    EXPECT_CALL(proc, block_complete()).WillRepeatedly(Return(false));
    EXPECT_CALL(proc, start_block());
    Parser p(proc);
    p.handle_token("{");
    EXPECT_EQ("ExpectingDynamicCommand", p.state().name());
    p.handle_token("cmd1");
    EXPECT_EQ("CollectingDynamicBlock", p.state().name());
    EXPECT_EQ(1, p.dynamic_level());
}

TEST(Processor, StaticBlockUntilRun)
{
    MockProcessor proc;
    EXPECT_CALL(proc, add_command("cmd1"));
    EXPECT_CALL(proc, block_complete())
            .WillOnce(Return(false))
            .WillOnce(Return(true));
    EXPECT_CALL(proc, add_command("cmd2"));
    EXPECT_CALL(proc, run());
    EXPECT_CALL(proc, start_block());
    Parser p(proc);
    p.handle_token("cmd1");
    EXPECT_EQ("CollectingStaticBlock", p.state().name());
    p.handle_token("cmd2");
    EXPECT_EQ("StartingBlock", p.state().name());
}

TEST(Processor, DynamicBlockUntilRun)
{
    MockProcessor proc;
    EXPECT_CALL(proc, add_command("cmd1"));
    EXPECT_CALL(proc, block_complete())
            .WillRepeatedly(Return(false));
    EXPECT_CALL(proc, add_command("cmd2"));
    EXPECT_CALL(proc, run());
    EXPECT_CALL(proc, start_block());
    Parser p(proc);
    p.handle_token("{");
    EXPECT_EQ("ExpectingDynamicCommand", p.state().name());
    EXPECT_EQ(0, p.dynamic_level());
    p.handle_token("cmd1");
    EXPECT_EQ("CollectingDynamicBlock", p.state().name());
    EXPECT_EQ(1, p.dynamic_level());
    p.handle_token("cmd2");
    EXPECT_EQ("CollectingDynamicBlock", p.state().name());
    p.handle_token("}");
    EXPECT_EQ("StartingBlock", p.state().name());
    EXPECT_EQ(0, p.dynamic_level());
}

TEST(Processor, EmptyDynamicBlock)
{
    MockProcessor proc;
    Parser p(proc);
    p.handle_token("{");
    EXPECT_EQ("ExpectingDynamicCommand", p.state().name());
    EXPECT_EQ(0, p.dynamic_level());
    p.handle_token("}");
    EXPECT_EQ("StartingBlock", p.state().name());
    EXPECT_EQ(0, p.dynamic_level());
}

TEST(Processor, StaticInterrupted)
{
    MockProcessor proc;
    EXPECT_CALL(proc, add_command("cmd1"));
    EXPECT_CALL(proc, block_complete())
            .WillOnce(Return(false))
            .WillOnce(Return(false))
            .WillOnce(Return(false))
            .WillOnce(Return(false))
            .WillOnce(Return(true));
    EXPECT_CALL(proc, add_command("cmd2"));
    EXPECT_CALL(proc, add_command("cmd3"));
    EXPECT_CALL(proc, add_command("cmd4"));
    EXPECT_CALL(proc, add_command("cmd5"));
    EXPECT_CALL(proc, add_command("cmd6"));
    EXPECT_CALL(proc, run()).Times(3);
    EXPECT_CALL(proc, start_block()).Times(3);
    Parser p(proc);
    p.handle_token("cmd1");
    EXPECT_EQ("CollectingStaticBlock", p.state().name());
    p.handle_token("cmd2");
    EXPECT_EQ(0, p.dynamic_level());
    EXPECT_EQ("CollectingStaticBlock", p.state().name());
    p.handle_token("{");
    EXPECT_EQ("ExpectingDynamicCommand", p.state().name());
    p.handle_token("cmd3");
    EXPECT_EQ(1, p.dynamic_level());
    p.handle_token("}");
    EXPECT_EQ("StartingBlock", p.state().name());
    EXPECT_EQ(0, p.dynamic_level());
    p.handle_token("cmd4");
    EXPECT_EQ("CollectingStaticBlock", p.state().name());
    p.handle_token("cmd5");
    EXPECT_EQ("CollectingStaticBlock", p.state().name());
    EXPECT_EQ(0, p.dynamic_level());
    p.handle_token("cmd6");
    EXPECT_EQ("StartingBlock", p.state().name());
}

TEST(Processor, NestedBlocks)
{
    MockProcessor proc;
    EXPECT_CALL(proc, add_command("cmd1"));
    EXPECT_CALL(proc, block_complete())
            .WillRepeatedly(Return(false));
    EXPECT_CALL(proc, add_command("cmd2"));
    EXPECT_CALL(proc, add_command("cmd3"));
    EXPECT_CALL(proc, add_command("cmd4"));
    EXPECT_CALL(proc, add_command("cmd5"));
    EXPECT_CALL(proc, run());
    EXPECT_CALL(proc, start_block());
    Parser p(proc);
    p.handle_token("{");
    EXPECT_EQ(0, p.dynamic_level());
    EXPECT_EQ("ExpectingDynamicCommand", p.state().name());
    p.handle_token("cmd1");
    EXPECT_EQ(1, p.dynamic_level());
    EXPECT_EQ("CollectingDynamicBlock", p.state().name());
    p.handle_token("cmd2");
    EXPECT_EQ("CollectingDynamicBlock", p.state().name());
    p.handle_token("{");
    EXPECT_EQ(2, p.dynamic_level());
    p.handle_token("cmd3");
    p.handle_token("cmd4");
    p.handle_token("}");
    EXPECT_EQ(1, p.dynamic_level());
    p.handle_token("cmd5");
    p.handle_token("}");
    EXPECT_EQ("StartingBlock", p.state().name());
    EXPECT_EQ(0, p.dynamic_level());
}

TEST(Processor, BreakStaticBlock)
{
    MockProcessor proc;
    EXPECT_CALL(proc, add_command("cmd1"));
    EXPECT_CALL(proc, block_complete())
            .WillOnce(Return(false));
    EXPECT_CALL(proc, run());
    EXPECT_CALL(proc, start_block());
    Parser p(proc);
    p.handle_token("cmd1");
    EXPECT_EQ("CollectingStaticBlock", p.state().name());
    p.end_of_stream();
    EXPECT_EQ("StartingBlock", p.state().name());
}

TEST(Processor, BreakDynamicBlock)
{
    MockProcessor proc;
    EXPECT_CALL(proc, add_command("cmd1"));
    EXPECT_CALL(proc, block_complete())
            .WillRepeatedly(Return(false));
    EXPECT_CALL(proc, start_block());
    Parser p(proc);
    p.handle_token("{");
    EXPECT_EQ("ExpectingDynamicCommand", p.state().name());
    EXPECT_EQ(0, p.dynamic_level());
    p.handle_token("cmd1");
    EXPECT_EQ("CollectingDynamicBlock", p.state().name());
    EXPECT_EQ(1, p.dynamic_level());
    p.end_of_stream();
    EXPECT_EQ("CollectingDynamicBlock", p.state().name());
    EXPECT_EQ(1, p.dynamic_level());
}

TEST(Async, NextHandle)
{
    std::atomic<async::handle_t> next_id { reinterpret_cast<async::handle_t>(1) };
    async::handle_t id = next_id.fetch_add(1, std::memory_order_relaxed);
    std::cout << "id = " << id << "\n";
    async::handle_t expected = (async::handle_t)1;
    EXPECT_EQ(expected, id);
}

TEST(PreProc, ParseInput)
{
    std::string input = "";
    auto start_pos = input.find_first_of("{");
    std::cout << "input = " << input
              << " start = " << start_pos << "\n";

    input = "a\nb\nc\nd";
    start_pos = input.find_first_of("{");
    std::cout << "input = " << input
              << " start = " << start_pos << "\n";

    input = "a\nb\n{\nc\nd\n}\ne";
    start_pos = input.find_first_of('{');
    auto end_pos = input.find_last_of('}');
    std::cout << "input = " << input
              << " start = " << start_pos
              << " end = " << end_pos
              << " before = " << input.substr(0, start_pos-1)
              << " substr = " << input.substr(start_pos, end_pos-start_pos+1)
              << "\n";
}

namespace async {
namespace details {
extern async::details::AsyncLibrary* g_library;
void set_library(async::details::AsyncLibrary* library);
void reset_library();
}
}

class Library_Test : public ::testing::Test
{
    protected:
        void SetUp() override
        {
            library = std::make_unique<decltype (library)::element_type>();
            async::details::set_library(library.get());
        }
        void TearDown() override
        {
            async::details::reset_library();
            library.reset();
        }

        async::details::AsyncLibraryUPtr library;
};

TEST(Library, SetLibrary)
{
    EXPECT_EQ(&(async::details::Async::instance()), async::details::g_library);
    std::cout << "instance: " << (void*)(&(async::details::Async::instance())) << "\n";
    std::cout << "global: " << (void*)(async::details::g_library) << "\n";

    async::details::AsyncLibrary l;
    std::cout << "local: " << (void*)(&l) << "\n";

    async::details::set_library(&l);
    std::cout << "global: " << (void*)(async::details::g_library) << "\n";

    EXPECT_EQ(&l, async::details::g_library);
}

TEST_F(Library_Test, FailConnect)
{
    async::handle_t h = async::connect(0);
    EXPECT_EQ(0, h);
    std::cout << library->n_procs() << "\n";
}

TEST_F(Library_Test, SuccessConnect)
{
    std::cout << "on enter: " << library->n_procs() << "\n";
    async::handle_t h = async::connect(3);
    EXPECT_EQ(reinterpret_cast<async::handle_t>(1), h);
    std::cout << library->n_procs() << "\n";
    async::disconnect(h);
    std::cout << library->n_procs() << "\n";
    h = async::connect(3);
    EXPECT_EQ(reinterpret_cast<async::handle_t>(2), h);
    async::disconnect(h);
    std::cout << "on exit: " << library->n_procs() << "\n";
}

TEST_F(Library_Test, SendData)
{
    async::handle_t h1 = async::connect(3);
    EXPECT_EQ(reinterpret_cast<async::handle_t>(1), h1);
    async::receive(h1, "1", 1);
    async::receive(h1, "\n", 1);
    async::receive(h1, "2", 1);
    async::receive(h1, "\n", 1);
    async::receive(h1, "3", 1);
    async::receive(h1, "\n", 1);

    async::handle_t h2 = async::connect(3);
    EXPECT_EQ(reinterpret_cast<async::handle_t>(2), h2);
    async::receive(h2, "1", 1);
    async::receive(h2, "{\n", 2);
    async::receive(h2, "2", 1);
    async::receive(h2, "}\n", 2);
    async::receive(h2, "3", 1);
    async::receive(h2, "\n", 1);

    EXPECT_EQ(3, library->n_procs());

    async::disconnect(h1);
    async::disconnect(h2);
}
