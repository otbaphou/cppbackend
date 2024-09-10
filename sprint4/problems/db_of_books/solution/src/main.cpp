// main.cpp

#include <iostream>
#include <pqxx/pqxx>
#include <boost/json.hpp>

using namespace std::literals;

namespace json = boost::json;

// libpqxx использует zero-terminated символьные литералы вроде "abc"_zv;
using pqxx::operator"" _zv;

class TableManager
{
public:
	TableManager(std::string table_url)
		:connection(table_url) {}

	pqxx::connection& GetDB()
	{
		return connection;
	}

private:

	pqxx::connection connection;
};

int main(int argc, const char* argv[]) {

	try
	{
		if (argc == 1)
		{
			std::cout << "Usage: db_example <conn-string>\n"sv;
			return EXIT_SUCCESS;
		}
		else if (argc != 2)
		{
			std::cerr << "Invalid command line\n"sv;
			return EXIT_FAILURE;
		}

		// Подключаемся к БД, указывая её параметры в качестве аргумента
		TableManager manager(argv[1]);
		// Создаём транзакцию. Это понятие будет разобрано в следующих уроках.
		pqxx::work w(manager.GetDB());
		// Транзакция нужна, чтобы выполнять запросы.

		// Используя транзакцию создадим таблицу в выбранной базе данных:
		w.exec("CREATE TABLE IF NOT EXISTS books (id SERIAL PRIMARY KEY, title varchar(100), author varchar(100), year integer, ISBN char(13));");

		// Применяем все изменения
		w.commit();

		while (true)
		{
			pqxx::work query_work(manager.GetDB());
			pqxx::read_transaction read_t(manager.GetDB());

			bool status = true;
			std::string query;
			std::getline(std::cin, query);

			json::object request = json::parse(query).as_object();

			boost::json::string req_str = request.at("action").as_string();
			json::object payload = request.at("payload").as_object();

			if (req_str == "exit"s)
				break;
			if (req_str == "add_book")
			{
				std::string check_query = "SELECT id, title FROM books WHERE ISBN=";
				check_query += payload.at("ISBN").as_string();
				check_query += " LIMIT 1;";
				std::optional query_result = read_t.query01<int, std::string>(check_query);

				json::object result;

				if (query_result)
				{
					status = false;
				}
				else
				{
					query_work.exec("INSERT INTO movies (title, year) VALUES ('"
					+ static_cast<std::string>(payload.at("title").as_string()) + "', "
					+ static_cast<std::string>(payload.at("author").as_string()) + "', "
					+ static_cast<std::string>(payload.at("year").as_string()) + "', "
					+ static_cast<std::string>(payload.at("ISBN").as_string()) + ")");

					query_work.commit();
				}

				result.emplace("result", status);
				std::cout << json::serialize(result) << std::endl;
			}

			if (req_str == "all_books")
			{
				json::array books;

				auto query_text = "SELECT * FROM books"_zv;

				// Выполняем запрос и итерируемся по строкам ответа
				for (auto [id, title, author, year, ISBN] : read_t.query<int, std::string, std::string, int, std::string>(query_text))
				{
					json::object book;
					book.emplace("id", id);
					book.emplace("title", title);
					book.emplace("author", author);
					book.emplace("year", year);
					book.emplace("ISBN", ISBN);

					books.push_back(book);
				}

				std::cout << json::serialize(books) << std::endl;
			}
		}
	}

	catch (const std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
