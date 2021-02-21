#include "example.hpp"

[[nodiscard]] static asio::awaitable<void> listener(asio::ip::tcp::acceptor acceptor)
{
    ChatRoom_ptr room = std::make_shared<ChatRoom>();

    while(true)
    {
        std::make_shared<ChatSession>(
                co_await acceptor.async_accept(asio::use_awaitable),
                room
                                     )->Start();
    }
}

int main(int argc, const char** argv)
{
    try
    {
        if(argc < 2)
        {
            std::cerr << "Usage: " << argv[0] << " <port> [<port> ...]" << std::endl;
            return 1;
        }

        asio::io_context io_context(1);

        for(int i = 1; i < argc; ++i)
        {
            uint16_t port = std::atoi(argv[i]);
            std::cout << "Listening on port " << port << std::endl;
            co_spawn(io_context,
                     listener(asio::ip::tcp::acceptor(io_context, {asio::ip::tcp::v4(), port})),
                     asio::detached
                    );
        }

        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) -> void { io_context.stop(); });

        io_context.run();
    }
    catch(std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
