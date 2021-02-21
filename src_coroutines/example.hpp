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

class ChatParticipant;

typedef std::shared_ptr<ChatParticipant> ChatParticipant_ptr;

class ChatParticipant
{
public:
    virtual ~ChatParticipant() = default;

    virtual void deliver(const std::string& msg) = 0;
};

//----------------------------------------------------------------------

class ChatRoom;

typedef std::shared_ptr<ChatRoom> ChatRoom_ptr;

class ChatRoom
{
public:
    void Join(const ChatParticipant_ptr& participant)
    {
        m_Participants.insert(participant);
        for(const auto& msg: m_RecentMessages)
            participant->deliver(msg);
    }

    void Leave(const ChatParticipant_ptr& participant)
    {
        m_Participants.erase(participant);
    }

    void Broadcast(const std::string& msg)
    {
        m_RecentMessages.push_back(msg);
        while(m_RecentMessages.size() > MaxRecentMessages)
            m_RecentMessages.pop_front();

        for(const auto& participant: m_Participants)
            participant->deliver(msg);
    }

private:
    std::set<ChatParticipant_ptr> m_Participants;
    static const std::size_t MaxRecentMessages = 100;
    std::deque<std::string> m_RecentMessages;
};

//----------------------------------------------------------------------

class ChatSession
        : public ChatParticipant,
          public std::enable_shared_from_this<ChatSession>
{
public:
    ChatSession(asio::ip::tcp::socket socket, ChatRoom_ptr room)
            : m_Socket(std::move(socket)),
              m_Timer(m_Socket.get_executor()),
              m_Room(std::move(room))
    {
        m_Timer.expires_at(std::chrono::steady_clock::time_point::max());
    }

    void Start()
    {
        m_Room->Join(shared_from_this());

        co_spawn(
                m_Socket.get_executor(),
                [self = shared_from_this()] { return self->Reader(); },
                asio::detached
                );

        co_spawn(
                m_Socket.get_executor(),
                [self = shared_from_this()] { return self->Writer(); },
                asio::detached
                );
    }

    void deliver(const std::string& msg) override
    {
        m_WriteMessages.push_back(msg);
        m_Timer.cancel_one();
    }

private:
    [[nodiscard]] asio::awaitable<void> Reader()
    {
        try
        {
            for(std::string read_msg;;)
            {
                std::size_t n = co_await asio::async_read_until(
                        m_Socket,
                        asio::dynamic_buffer(read_msg, 1024),
                        "\n",
                        asio::use_awaitable
                                                               );

                m_Room->Broadcast(read_msg.substr(0, n));
                read_msg.erase(0, n);
            }
        }
        catch(std::exception&)
        {
            Stop();
        }
    }

    [[nodiscard]] asio::awaitable<void> Writer()
    {
        try
        {
            while(m_Socket.is_open())
            {
                if(m_WriteMessages.empty())
                {
                    asio::error_code ec;
                    co_await m_Timer.async_wait(redirect_error(asio::use_awaitable, ec));
                }
                else
                {
                    co_await asio::async_write(
                            m_Socket,
                            asio::buffer(m_WriteMessages.front()),
                            asio::use_awaitable
                                              );
                    m_WriteMessages.pop_front();
                }
            }
        }
        catch(std::exception&)
        {
            Stop();
        }
    }

    void Stop()
    {
        m_Room->Leave(shared_from_this());
        m_Socket.close();
        m_Timer.cancel();
    }

    asio::ip::tcp::socket m_Socket;
    asio::steady_timer m_Timer;
    ChatRoom_ptr m_Room;
    std::deque<std::string> m_WriteMessages;
};
