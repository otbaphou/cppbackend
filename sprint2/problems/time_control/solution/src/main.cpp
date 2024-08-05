#include "http_server.h"
#include "sdk.h"

#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace http = boost::beast::http;

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

//Once again, I don't know where else to place this stuff below. This file seemed suitable though..
void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
	json::object final_obj;
	//Parsing timestamp and inserting it into the final json::object
	final_obj.emplace("timestamp", pt::to_iso_extended_string(rec[timestamp].get()));

	//Inserting additional information into the final json::object
	final_obj.emplace("data", rec[additional_data].get());

	//Inserting message string into the final json::object
	final_obj.emplace("message", rec[logging::expressions::smessage].get());

	strm << json::serialize(final_obj);
}

void InitBoostLogFilter()
{
	logging::add_console_log(
		std::cout,
		keywords::format = &MyFormatter,
		keywords::auto_flush = true
	);

	/*logging::add_file_log(
		keywords::file_name = "game_server.log",
		keywords::format = &MyFormatter,
		keywords::open_mode = std::ios_base::app | std::ios_base::out
	);*/
}

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
		model::Players player_manager;
		model::Game game = json_loader::LoadGame(argv[1], player_manager);

		// 2. Инициализируем io_context
		const unsigned num_threads = std::thread::hardware_concurrency();
		net::io_context ioc(num_threads);

		// 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM

		// 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
		http_handler::RequestHandler handler{ argv[2], game };

		const auto address = net::ip::make_address("0.0.0.0");
		constexpr net::ip::port_type port = 8080;

		// 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов

		InitBoostLogFilter();

		http_server::ServeHttp(ioc, { address, port }, [&handler](auto&& req, auto&& send)
			{
				handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
			}); 

		// Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
		json::object logger_data{ {"port", static_cast<unsigned>(port)}, {"address", address.to_string()} };

		BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "Server has started..."sv;

		// 6. Запускаем обработку асинхронных операций
		RunWorkers(std::max(1u, num_threads), [&ioc]
			{
				ioc.run();
			});
	}

	catch (const std::exception& ex)
	{
		//Logging server exit with errors
		json::object logger_data{ {"code", EXIT_FAILURE}, {"exception", ex.what()} };

		BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "server exited"sv;

		return EXIT_FAILURE;
	}

	//Logging server exit without errors
	json::object logger_data{ {"code", 0} };

	BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "server exited"sv;
}