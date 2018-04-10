#pragma once

#include "messagequeue.h"
#include "counters.h"
#include "logger.h"
#include <thread>
#include <memory>
#include <fstream>

template<typename Q>
class IListener
{
    public:
        IListener(Q& q) : q_(q) {}
        virtual ~IListener()
        {
            wait();
        }

        virtual void listen() = 0;
        virtual void report(std::ostream&) = 0;

        void run()
        {
            thread_ = std::make_unique<std::thread>(&IListener::listen, this);
        }

        void wait()
        {
            if (thread_->joinable()) {
                thread_->join();
            }
        }

        Q& queue() { return q_; }

    protected:
        Q& q_;
        Counters counters_;
        std::unique_ptr<std::thread> thread_;

        void update_counters(const Message& msg)
        {
            counters_.commands += msg.block.commands;
            ++counters_.blocks;
        }
};

class ConsoleListener : public IListener<MessageQueue>
{
    public:
        ConsoleListener(MessageQueue& q) : IListener(q) {}

        void listen() override
        {
            Message msg;
            do
            {
                msg = q_.pop();
                if (msg.id == MessageId::Data) {
                    std::cout << msg.block.data;
                    update_counters(msg);
                }
            } while (msg.id != MessageId::EndOfStream);
        }

        void report(std::ostream& out) override
        {
            out << "log поток - "
                << counters_.blocks << " блок, "
                << counters_.commands << " команд\n";
        }
};

class FileListener : public IListener<MessageQueue>
{
    public:
        FileListener(MessageQueue& q, int id) : IListener(q), id_(id) {}

        void listen() override
        {
            Message msg;
            do
            {
                msg = q_.pop();
                if (msg.id == MessageId::Data) {
                    std::ofstream file(msg.filename);
                    file << msg.block.data;
                    file.close();
                    update_counters(msg);
                }
            } while (msg.id != MessageId::EndOfStream);
        }

        void report(std::ostream& out) override
        {
            out << "file " << id_ << " поток - "
                << counters_.blocks << " блок, "
                << counters_.commands << " команд\n";
        }

    private:
        const int id_;
};
