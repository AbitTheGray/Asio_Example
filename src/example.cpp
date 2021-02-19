#include "example.hpp"

asio::awaitable<void> listener(asio::ip::tcp::acceptor acceptor)
{
    chat_room room;

    while(true)
    {
        std::make_shared<chat_session>(
                co_await acceptor.async_accept(asio::use_awaitable),
                room
                                      )->start();
    }
}

int main_call(int argc, const char** argv)
{
    try
    {
        if(argc < 2)
        {
            std::cerr << "Usage: chat_server <port> [<port> ...]\n";
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
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
