#pragma once

//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <asio/awaitable.hpp>
#include <asio/detached.hpp>
#include <asio/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read_until.hpp>
#include <asio/redirect_error.hpp>
#include <asio/signal_set.hpp>
#include <asio/steady_timer.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

//----------------------------------------------------------------------

class chat_participant
{
public:
    virtual ~chat_participant() = default;

    virtual void deliver(const std::string& msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
    void join(const chat_participant_ptr& participant)
    {
        participants_.insert(participant);
        for(const auto& msg: recent_msgs_)
            participant->deliver(msg);
    }

    void leave(const chat_participant_ptr& participant)
    {
        participants_.erase(participant);
    }

    void deliver(const std::string& msg)
    {
        recent_msgs_.push_back(msg);
        while(recent_msgs_.size() > max_recent_msgs)
            recent_msgs_.pop_front();

        for(const auto& participant: participants_)
            participant->deliver(msg);
    }

private:
    std::set<chat_participant_ptr> participants_;
    enum
    {
        max_recent_msgs = 100
    };
    std::deque<std::string> recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
        : public chat_participant,
          public std::enable_shared_from_this<chat_session>
{
public:
    chat_session(asio::ip::tcp::socket socket, chat_room& room)
            : socket_(std::move(socket)),
              timer_(socket_.get_executor()),
              room_(room)
    {
        timer_.expires_at(std::chrono::steady_clock::time_point::max());
    }

    void start()
    {
        room_.join(shared_from_this());

        co_spawn(socket_.get_executor(),
                 [self = shared_from_this()]
                 { return self->reader(); },
                 asio::detached
                );

        co_spawn(socket_.get_executor(),
                 [self = shared_from_this()]
                 { return self->writer(); },
                 asio::detached
                );
    }

    void deliver(const std::string& msg)
    {
        write_msgs_.push_back(msg);
        timer_.cancel_one();
    }

private:
    asio::awaitable<void> reader()
    {
        try
        {
            for(std::string read_msg;;)
            {
                std::size_t n = co_await asio::async_read_until(
                        socket_,
                        asio::dynamic_buffer(read_msg, 1024),
                        "\n",
                        asio::use_awaitable
                                                               );

                room_.deliver(read_msg.substr(0, n));
                read_msg.erase(0, n);
            }
        }
        catch(std::exception&)
        {
            stop();
        }
    }

    asio::awaitable<void> writer()
    {
        try
        {
            while(socket_.is_open())
            {
                if(write_msgs_.empty())
                {
                    asio::error_code ec;
                    co_await timer_.async_wait(redirect_error(asio::use_awaitable, ec));
                }
                else
                {
                    co_await asio::async_write(
                            socket_,
                            asio::buffer(write_msgs_.front()),
                            asio::use_awaitable
                                              );
                    write_msgs_.pop_front();
                }
            }
        }
        catch(std::exception&)
        {
            stop();
        }
    }

    void stop()
    {
        room_.leave(shared_from_this());
        socket_.close();
        timer_.cancel();
    }

    asio::ip::tcp::socket socket_;
    asio::steady_timer timer_;
    chat_room& room_;
    std::deque<std::string> write_msgs_;
};

//----------------------------------------------------------------------

int main_call(int argc, const char* argv[]);
