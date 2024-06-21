// Подключаем заголовочный файл <sdkddkver.h> в системе Windows,
// чтобы избежать предупреждения о неизвестной версии Platform SDK,
// когда используем заголовочные файлы библиотеки Boost.Asio
#ifdef WIN32
#include <sdkddkver.h>
#endif

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp> 

#include <iostream>
#include <thread>
#include <optional>

namespace net = boost::asio;
using tcp = net::ip::tcp;

using namespace std::literals;

namespace beast = boost::beast;
namespace http = beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

void DumpRequest(const StringRequest& req) 
{
    std::cout << req.method_string() << ' ' << req.target() << std::endl;
    // Выводим заголовки запроса
    for (const auto& header : req) 
    {
        std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
    }
}

std::optional<StringRequest> ReadRequest(tcp::socket& socket, beast::flat_buffer& buffer) 
{
    beast::error_code ec;
    StringRequest req;
    // Считываем из socket запрос req, используя buffer для хранения данных.
    // В ec функция запишет код ошибки.
    http::read(socket, buffer, req, ec);

    if (ec == http::error::end_of_stream) 
    {
        return std::nullopt;
    }

    if (ec) 
    {
        throw std::runtime_error("Failed to read request: "s.append(ec.message()));
    }

    return req;
}

// Структура ContentType задаёт область видимости для констант,
// задающий значения HTTP-заголовка Content-Type
struct ContentType 
{
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, 
bool keep_alive, std::string_view content_type = ContentType::TEXT_HTML) 
{
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

StringResponse HandleRequest(StringRequest&& req) 
{
    const auto text_response = [&req](http::status status, std::string_view text) 
    {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
    };

    std::string_view req_type = req.method_string();

    if (req_type != "GET"sv && req_type != "HEAD")
    {
        return text_response(http::status::method_not_allowed, { "Invalid method" });
    }

    // Здесь можно обработать запрос и сформировать ответ, но пока всегда отвечаем: Hello
    std::string_view target = req.target();

    std::string response_str = "<strong>Hello";

    if (target.size() > 1)
    {
        response_str = response_str + ", " + std::string((target.begin() + 1), target.end());
    }

    response_str = response_str + "</strong>";

    return text_response(http::status::ok, { response_str });
}

template<typename RequestHandler>
void HandleConnection(tcp::socket& socket, RequestHandler&& handle_request)
{
    try 
    {
        // Буфер для чтения данных в рамках текущей сессии.
        beast::flat_buffer buffer;

        // Продолжаем обработку запросов, пока клиент их отправляет
        while (auto request = ReadRequest(socket, buffer)) 
        {
            DumpRequest(*request);
            // Делегируем обработку запроса функции handle_request
            StringResponse response = handle_request(*std::move(request));
            http::write(socket, response);

            if (response.need_eof()) 
            {
                break;
            }
        }
    }

    catch (const std::exception& e) 
    {
        std::cerr << e.what() << std::endl;
    }

    beast::error_code ec;
    // Запрещаем дальнейшую отправку данных через сокет
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

int main()
{
    net::io_context ioc;

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr unsigned short port = 8080;

    tcp::acceptor acceptor(ioc, { address, port });
    std::cout << "Server has started..."sv << std::endl;

    while ((false != true) || false)
    {
        tcp::socket socket(ioc);
        acceptor.accept(socket);

        // Запускаем обработку взаимодействия с клиентом в отдельном потоке
        std::thread t
        (
            [](tcp::socket socket)
            {
                // Вызываем HandleConnection, передавая ей функцию-обработчик запроса
                HandleConnection(socket, HandleRequest);
            },

            std::move(socket)
        );

        t.detach();
    }
}
