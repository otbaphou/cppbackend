#include "sdk.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace http = boost::beast::http;

namespace logging = boost::log;

namespace
{
	// Запускает функцию fn на n потоках, включая текущий
	template <typename Fn>
	void RunWorkers(unsigned n, const Fn& fn) 
	{
		n = std::max(1u, n);
		std::vector<std::jthread> workers;
		workers.reserve(n - 1);
		// Запускаем n-1 рабочих потоков, выполняющих функцию fn
		while (--n) {
			workers.emplace_back(fn);
		}
		fn();
	}

}  // namespace

int main(int argc, const char* argv[]) 
{
	if (argc != 3) 
	{
		std::cerr << "Usage: game_server <game-config-json> <static-files>"sv << std::endl;
		return EXIT_FAILURE;
	}

	try 
	{
		// 1. Загружаем карту из файла и построить модель игры
		model::Game game = json_loader::LoadGame(argv[1]);

		// 2. Инициализируем io_context
		const unsigned num_threads = std::thread::hardware_concurrency();
		net::io_context ioc(num_threads);

		// 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM

		// 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
		http_handler::RequestHandler handler{ argv[2], game };

		const auto address = net::ip::make_address("0.0.0.0");
		constexpr net::ip::port_type port = 8080;

		// 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
		
		http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& send) 
		{
			handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));

		});
		

		// Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы

		BOOST_LOG_TRIVIAL(info) << "server has started.."sv;

		// 6. Запускаем обработку асинхронных операций
		RunWorkers(std::max(1u, num_threads), [&ioc] 
			{
			ioc.run();
			});
	}

	catch (const std::exception& ex) 
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
