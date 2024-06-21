#include <boost/asio.hpp>
#include "audio.h"
#include <iostream>

using namespace std::literals;

namespace net = boost::asio;
using net::ip::udp;

static const size_t max_buffer_size = 1024;

void StartServer(uint16_t port)
{
    Player player(ma_format_u8, 1);

    try 
    {
        std::cout << "Server Started!" << std::endl;

        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        // Запускаем сервер в цикле, чтобы можно было работать со многими клиентами
        for (;;) 
        {
            // Создаём буфер достаточного размера, чтобы вместить датаграмму.
            std::array<char, max_buffer_size> recv_buf;
            udp::endpoint remote_endpoint;

            // Получаем не только данные, но и endpoint клиента
            auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

            player.PlayBuffer(recv_buf.data(), recv_buf.size() / player.GetFrameSize(), 1.5s);

            std::cout << "Received The Message From Client!" << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

void StartClient(uint16_t port)
{
    Recorder recorder(ma_format_u8, 1);

    while (true)
    {
        std::string dest_ip;

        std::cout << "Enter Server Ip To Record And Send The Message..." << std::endl;
        std::getline(std::cin, dest_ip);

        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording Done. Sending.." << std::endl;

        try
        {
            net::io_context io_context;

            // Перед отправкой данных нужно открыть сокет. 
            // При открытии указываем протокол (IPv4 или IPv6) вместо endpoint.
            udp::socket socket(io_context, udp::v4());

            boost::system::error_code ec;
            auto endpoint = udp::endpoint(net::ip::make_address(dest_ip, ec), port);

            socket.send_to(boost::asio::buffer(rec_result.data), endpoint);
        }
        catch (std::exception& e) 
        {
            std::cerr << e.what() << std::endl;
        }
    }
}

int main(int argc, char** argv) 
{
    int port = 3333;


    //#####Checks#####
    if (argc != 3) 
    {
       std::cout << "Usage: "sv << argv[0] << " <Instance Type (\"server\" or \"client\")> <Server Port>"sv << std::endl;
       return 1;
    }

    if (argv[1] != "server"sv && argv[1] != "client"sv)
    {
        std::cout << "Error! Invalid Instance Type!"sv << std::endl;
        std::cout << "Debug! \""sv << argv[1] << '"' << std::endl;
        if (argv[1] == "server")
        {
            std::cout << "WTF?" << std::endl;
        }
        return 2;
    }

    std::string s = argv[2];

    for (char c : s)
    {
        if (!isdigit(c))
        {
            std::cout << "Error! Invalid Port! (Invalid Characters)"sv << std::endl;
            return 3;
        }
    }

    try
    {
        port = std::stoi(s);
    }
    catch (...)
    {
        std::cout << "Error! Invalid Port! (Conversion Error)"sv << std::endl;
        return 4;
    }

    if (port <= 0 || port > 65535)
    {
        std::cout << "Error! Invalid Port! (Invalid Range)"sv << std::endl;
        return 5;
    }

    //#####~Checks#####

    if (argv[1] == "server"sv)
    {
        StartServer(port);
    }
    else
    {
        StartClient(port);
    }

    return 0;
}
