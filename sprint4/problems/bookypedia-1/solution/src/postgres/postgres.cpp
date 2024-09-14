#include "postgres.h"

#include <pqxx/zview.hxx>
#include <pqxx/pqxx>

namespace postgres {

	using namespace std::literals;
	using pqxx::operator"" _zv;

	void AuthorRepositoryImpl::Save(const domain::Author& author)
	{
		// Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
		// В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
		// запросов выполнить в рамках одной транзакции.
		// Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
		pqxx::work work{ connection_ };
		work.exec_params(R"(INSERT INTO authors (id, name) VALUES ($1, $2) ON CONFLICT (id) DO UPDATE SET name=$2;)"_zv, author.GetId().ToString(), author.GetName());
		work.commit();
	}

	const std::vector<domain::Author> AuthorRepositoryImpl::Load() const
	{
		std::vector<domain::Author> result;

		pqxx::read_transaction read_t(connection_);

		auto query_text = "SELECT id, name FROM authors ORDER BY name ASC, id ASC"_zv;

		//Выполняем запрос и итерируемся по строкам ответа
		for (auto [id, name] : read_t.query<std::string, std::string>(query_text))
		{
			domain::AuthorId author_id = domain::AuthorId::FromString(id);
			domain::Author author{ author_id, name};

			result.push_back(author);
		}

		return result;
	}


	void BookRepositoryImpl::Save(const domain::Book& book)
	{
		pqxx::work work{ connection_ };
		work.exec_params(R"(INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4) ON CONFLICT (id) DO UPDATE SET name=$2;)"_zv, 
			book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetName(), book.GetReleaseYear());
		work.commit();
	}

	const std::vector<domain::Book> BookRepositoryImpl::Load() const
	{
		std::vector<domain::Book> result;

		pqxx::read_transaction read_t(connection_);
		auto query_text = "SELECT id, author_id, title, publication_year FROM books ORDER BY title ASC"_zv;

		for (auto [id, author_id, title, publication_year] : read_t.query<std::string, std::string, std::string, int>(query_text))
		{
			domain::BookId book_id = domain::BookId::FromString(id);
			domain::AuthorId author_id_tmp = domain::AuthorId::FromString(author_id);

			domain::Book book{ book_id, author_id_tmp, title, publication_year };

			result.push_back(book);
		}

		return result;
	}

	const std::vector<domain::Book> BookRepositoryImpl::LoadByAuthor(const std::string& author_id ) const
	{
		std::vector<domain::Book> result;

		pqxx::read_transaction read_t(connection_);
		std::string query_text = "SELECT id, author_id, title, publication_year FROM books WHERE author_id='";
		query_text = query_text + domain::AuthorId::FromString(author_id).ToString();
		query_text = query_text + "' ORDER BY publication_year ASC, title ASC";

		for (auto [id, author_id, title, publication_year] : read_t.query<std::string, std::string, std::string, int>(query_text))
		{
			domain::BookId book_id = domain::BookId::FromString(id);
			domain::AuthorId author_id_tmp = domain::AuthorId::FromString(author_id);

			domain::Book book{ book_id, author_id_tmp, title, publication_year };

			result.push_back(book);
		}

		return result;
	}

	Database::Database(pqxx::connection connection)
		: connection_{ std::move(connection) }
	{
		pqxx::work work{ connection_ };
		work.exec(R"(CREATE TABLE IF NOT EXISTS authors ( id UUID CONSTRAINT author_id_constraint PRIMARY KEY, name varchar(100) UNIQUE NOT NULL );)"_zv);
		work.exec(R"(CREATE TABLE IF NOT EXISTS books ( id UUID CONSTRAINT book_id_constraint PRIMARY KEY, author_id UUID NOT NULL, title varchar(100) NOT NULL, publication_year integer);)"_zv);

		// ... создать другие таблицы

		// коммитим изменения
		work.commit();
	}

}  // namespace postgres
