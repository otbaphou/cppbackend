#include "http_server.h"
#include "sdk.h"

#include <boost/program_options.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>
#include <fstream>
#include <optional>

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

//class Ticker : public std::enable_shared_from_this<Ticker> 
//{
//public:
//	using Strand = net::strand<net::io_context::executor_type>;
//	using Handler = std::function<void(std::chrono::milliseconds delta)>;
//
//	// Функция handler будет вызываться внутри strand с интервалом period
//	Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
//		: strand_{ strand }
//		, period_{ period }
//		, handler_{ std::move(handler) } {
//	}
//
//	void Start() {
//		net::dispatch(strand_, [self = shared_from_this()] {
//			last_tick_ = Clock::now();
//			self->ScheduleTick();
//			});
//	}
//
//private:
//	void ScheduleTick() {
//		assert(strand_.running_in_this_thread());
//		timer_.expires_after(period_);
//		timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
//			self->OnTick(ec);
//			});
//	}
//
//	void OnTick(sys::error_code ec) {
//		using namespace std::chrono;
//		assert(strand_.running_in_this_thread());
//
//		if (!ec) {
//			auto this_tick = Clock::now();
//			auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
//			last_tick_ = this_tick;
//			try {
//				handler_(delta);
//			}
//			catch (...) {
//			}
//			ScheduleTick();
//		}
//	}
//
//	using Clock = std::chrono::steady_clock;
//
//	Strand strand_;
//	std::chrono::milliseconds period_;
//	net::steady_timer timer_{ strand_ };
//	Handler handler_;
//	std::chrono::steady_clock::time_point last_tick_;
//};

struct Args
{
	std::string config_file;
	std::string static_dir;

	int tick_period;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) 
{
	namespace po = boost::program_options;

	po::options_description desc{ "Allowed options"s };

	Args args;

	desc.add_options()
		("help,h", "produce help message")
		("tick-period,t", po::value<int>(&args.tick_period), "set tick period")
		("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")
		("www-root,w", po::value(&args.static_dir)->value_name("dir"s), "set static files root")
		("randomize-spawn-points", "spawn dogs at random positions");	

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.contains("help"s))
	{
		std::cout << desc;
		return std::nullopt;
	}

	if (!vm.contains("config-file"s)) 
	{
		throw std::runtime_error("Config file has not been specified"s);
	}
	else
	{
		std::cout << "CONFIG PATH " << args.config_file << "\n";
	}

	if (!vm.contains("www-root"s)) 
	{
		throw std::runtime_error("Static dir path isn't specified"s);
	}
	else
	{
		std::cout << "STATIC PATH " << args.static_dir << "\n";;
	}

	return args;
}

int main(int argc, const char* argv[])
{
	std::filesystem::path config_path;
	std::filesystem::path static_path;
	try
	{
		if (auto args = ParseCommandLine(argc, argv))
		{
			if (args.has_value())
			{
				Args val = args.value();

				config_path = val.config_file;
				static_path = val.static_dir;
			}
			else
			{
				return EXIT_FAILURE;
			}
		}
		/* Если копировать файлы не нужно, то попадём сюда */
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	/*
	if (argc != 3)
	{
		std::cerr << "Usage: game_server <game-config-json> <static-files>"sv << std::endl;
		return EXIT_FAILURE;
	}
	*/

	try
	{
		// 1. Загружаем карту из файла и построить модель игры
		model::Game game = json_loader::LoadGame(config_path);

		// 2. Инициализируем io_context
		const unsigned num_threads = std::thread::hardware_concurrency();
		net::io_context ioc(num_threads);

		// 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM

		// 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
		http_handler::RequestHandler handler{ static_path, game };

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