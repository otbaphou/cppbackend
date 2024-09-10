// main.cpp

#include <iostream>
#include <pqxx/pqxx>
#include <boost/json.hpp>
#include <optional>

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
		w.exec("CREATE TABLE IF NOT EXISTS books (id SERIAL PRIMARY KEY NOT NULL, title varchar(100) NOT NULL, author varchar(100) NOT NULL, year integer NOT NULL, ISBN char(13) CONSTRAINT must_be_different UNIQUE);");

		// Применяем все изменения
		w.commit();

		while (true)
		{

			bool status = true;
			std::string query;
			std::getline(std::cin, query);

			json::value raw_request = json::parse(query);
			json::object request = raw_request.as_object();

			boost::json::string req_str = request.at("action").as_string();
			json::object payload = request.at("payload").as_object();

			if (req_str == "exit"s)
				break;
				
			if (req_str == "all_books")
			{
				pqxx::read_transaction read_t(manager.GetDB());
				
				json::array books;

				auto query_text = "SELECT * FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC"_zv;

				// Выполняем запрос и итерируемся по строкам ответа
				for (auto [id, title, author, year, ISBN] : read_t.query<int, std::string, std::string, int, std::optional<std::string>>(query_text))
				{
					json::object book;
					book.emplace("id", id);
					book.emplace("title", title);
					book.emplace("author", author);
					book.emplace("year", year);
					book.emplace("ISBN", ISBN.value_or("null"));

					books.push_back(book);
				}

				std::cout << json::serialize(books) << std::endl;
			}
			
			if (req_str == "add_book")
			{
				json::object result;

				try
				{
					pqxx::work query_work(manager.GetDB());
					json::value ISBN = payload.at("ISBN");
					
					std::string ISBN_str = "";
					
					if(ISBN.is_string())
					{
						ISBN_str += "\'";
						ISBN_str += ISBN.as_string();
						ISBN_str += "\'";
					}
					else
					{
						ISBN_str = "null";
					}
					
					query_work.exec("INSERT INTO books (title, author, year, ISBN) VALUES ('" 
					+ query_work.esc(static_cast<std::string>(payload.at("title").as_string())) + "', '"
					+ query_work.esc(static_cast<std::string>(payload.at("author").as_string())) + "', "
					+ std::to_string(payload.at("year").as_int64()) + ", "
					+ query_work.esc(ISBN_str) + ")");

					query_work.commit();
				}
				catch(...)
				{
					status = false;
				}

				result.emplace("result", status);
				std::cout << json::serialize(result) << std::endl;
			}
		}
	}

	catch (const std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
