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

		auto query_text = "SELECT id, name FROM authors ORDER BY name"_zv;

		//Выполняем запрос и итерируемся по строкам ответа
		for (auto [id, name] : read_t.query<std::string, std::string>(query_text))
		{
			domain::AuthorId author_id = domain::AuthorId::FromString(id);
			domain::Author author{ author_id, name };

			result.push_back(author);
		}
		read_t.commit();

		return result;
	}

	const domain::Author AuthorRepositoryImpl::LoadSingle(const std::string& id) const
	{
		pqxx::read_transaction read_t(connection_);

		std::string query_text = "SELECT id, name FROM authors WHERE id=";
		query_text = query_text + id;
		query_text = query_text + "';";

		//Выполняем запрос и итерируемся по строкам ответа
		for (auto [id, name] : read_t.query<std::string, std::string>(query_text))
		{
			domain::AuthorId author_id = domain::AuthorId::FromString(id);
			domain::Author author{ author_id, name };

			return author;
			break;
		}

		read_t.commit();

		throw std::invalid_argument("Author not found!");
	}

	void BookRepositoryImpl::Save(const domain::Book& book)
	{
		pqxx::work work{ connection_ };
		work.exec_params(R"(INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4) ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;)"_zv,
			book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetName(), book.GetReleaseYear());
		work.commit();
	}

	//const std::vector<domain::Book> BookRepositoryImpl::Load() const
	//{
	//	std::vector<domain::Book> result;

	//	pqxx::read_transaction read_t(connection_);
	//	auto query_text = "SELECT id, author_id, title, publication_year FROM books ORDER BY title"_zv;

	//	for (auto [id, author_id, title, publication_year] : read_t.query<std::string, std::string, std::string, int>(query_text))
	//	{
	//		domain::BookId book_id = domain::BookId::FromString(id);
	//		domain::AuthorId author_id_tmp = domain::AuthorId::FromString(author_id);

	//		domain::Book book{ book_id, author_id_tmp, title, publication_year };

	//		result.push_back(book);
	//	}
	//	read_t.commit();

	//	return result;
	//}

	const std::vector<domain::BookRepresentation> BookRepositoryImpl::Load() const
	{
		std::vector<domain::BookRepresentation> result;

		pqxx::read_transaction read_t(connection_);
		auto query_text = "SELECT books.id, books.title, authors.name AS author_name, books.publication_year FROM books JOIN authors ON books.author_id = authors.id ORDER BY books.title ASC,authors.name ASC, books.publication_year ASC;"_zv;

		for (auto [id, title, author_name, publication_year] : read_t.query<std::string, std::string, std::string, int>(query_text))
		{
			domain::BookId book_id = domain::BookId::FromString(id);

			domain::BookRepresentation book{ title, book_id, author_name, publication_year };

			result.push_back(book);
		}

		read_t.commit();

		return result;
	}

	const std::vector<domain::BookRepresentation> BookRepositoryImpl::LoadByAuthor(const std::string& author_id) const
	{
		std::vector<domain::BookRepresentation> result;

		pqxx::read_transaction read_t(connection_);

		std::string query_text = "SELECT id, author_id, title, publication_year FROM books WHERE author_id='";
		query_text = query_text + author_id;
		query_text = query_text + "' ORDER BY publication_year, title";

		for (auto [id, author_id, title, publication_year] : read_t.query<std::string, std::string, std::string, int>(query_text))
		{
			domain::BookId book_id = domain::BookId::FromString(id);
			domain::AuthorId author_id_tmp = domain::AuthorId::FromString(author_id);

			domain::BookRepresentation book{ title, book_id, author_id_tmp, publication_year };

			result.push_back(book);
		}
		read_t.commit();

		return result;
	}

	const std::string AuthorRepositoryImpl::GetAuthorId(const std::string& name) const
	{
		pqxx::read_transaction read_t(connection_);

		std::string query_text = "SELECT id, name FROM authors WHERE name='";
		query_text = query_text + name;
		query_text = query_text + "' ORDER BY name";

		for (auto [id, name] : read_t.query<std::string, std::string>(query_text))
		{
			return name;
		}

		read_t.commit();

		return "";
	}

	const std::vector<domain::BookRepresentation> BookRepositoryImpl::LoadByName(const std::string& book_name) const
	{
		std::vector<domain::BookRepresentation> result;

		pqxx::read_transaction read_t(connection_);

		std::string query_text = "SELECT id, author_id, title, publication_year FROM books WHERE title='";
		query_text = query_text + book_name;
		query_text = query_text + "' ORDER BY publication_year, title";

		for (auto [id, author_id, title, publication_year] : read_t.query<std::string, std::string, std::string, int>(query_text))
		{
			domain::BookId book_id = domain::BookId::FromString(id);
			domain::AuthorId author_id_tmp = domain::AuthorId::FromString(author_id);

			domain::BookRepresentation book{ title, book_id, author_id_tmp, publication_year };

			result.push_back(book);
		}
		read_t.commit();

		return result;
	}

	const domain::BookRepresentation BookRepositoryImpl::LoadById(const std::string& book_id) const
	{
		pqxx::read_transaction read_t(connection_);

		std::string query_text = "SELECT id, author_id, title, publication_year FROM books WHERE id='";
		query_text = query_text + book_id;
		query_text = query_text + "';";

		for (auto [id, author_id, title, publication_year] : read_t.query<std::string, std::string, std::string, int>(query_text))
		{
			domain::BookId book_id = domain::BookId::FromString(id);
			domain::AuthorId author_id_tmp = domain::AuthorId::FromString(author_id);

			domain::BookRepresentation book{ title, book_id, author_id_tmp, publication_year };

			return book;
		}
		read_t.commit();

		return {"error", domain::BookId::New(), "I'm tired, boss", 1234 };
	}

	const std::vector<std::string> BookRepositoryImpl::LoadTags(const std::string& book_id) const
	{
		std::vector<std::string> result;

		pqxx::read_transaction read_t(connection_);

		std::string query_text = "SELECT book_id, tag FROM book_tags WHERE book_id='";
		query_text = query_text + book_id;
		query_text = query_text + "' ORDER BY tag";

		for (auto [book_id, tag] : read_t.query<std::string, std::string>(query_text))
		{
			//domain::BookId book_id = domain::BookId::FromString(book_id);

			result.push_back(tag);
		}
		read_t.commit();

		return result;
	}

	void BookRepositoryImpl::SaveTags(const std::string& book_id, const std::vector<std::string>& tags)
	{
		pqxx::work work{ connection_ };

		for (const std::string& tag : tags)
		{
			work.exec_params(R"(INSERT IGNORE INTO book_tags (id, tag) VALUES ($1, $2);)"_zv, book_id, tag);
		}
	}

	//EDITING

	void AuthorRepositoryImpl::EditAuthor(const std::string& author_id, const std::string new_name)
	{
		pqxx::work work{ connection_ };

		std::string query_text = "UPDATE authors SET name = '";
		query_text = query_text + new_name + "' WHERE id = '" + author_id + "';";

		work.exec(query_text);

		work.commit();
	}

	void BookRepositoryImpl::EditBook(const std::string& book_id, const domain::BookRepresentation& new_data, const std::vector<std::string>& tags)
	{
		pqxx::work work{ connection_ };

		std::string query_text = "UPDATE books SET title = '";
		query_text = query_text + new_data.title + "', publication_year = " + std::to_string(new_data.year) + " WHERE id = '" + book_id + "';";

		work.exec(query_text);

		query_text = "DELETE FROM book_tags WHERE book_id = '";
		query_text = query_text + book_id;
		query_text = query_text + "';";

		work.exec(query_text);

		for (const std::string& tag : tags)
		{
			work.exec_params(R"(INSERT IGNORE INTO book_tags (id, tag) VALUES ($1, $2);)"_zv, book_id, tag);
		}

		work.commit();
	}

	//DELETION

	void AuthorRepositoryImpl::DeleteAuthor(const std::string& author_id)
	{
		pqxx::work work{ connection_ };

		std::string query_text = "DELETE FROM authors WHERE id = '";
		query_text = query_text + author_id;
		query_text = query_text + "';";

		work.exec(query_text);

		work.commit();
	}

	void BookRepositoryImpl::DeleteBook(const std::string& book_id)
	{
		pqxx::work work{ connection_ };

		std::string query_text = "DELETE FROM book_tags WHERE book_id = '";
		query_text = query_text + book_id;
		query_text = query_text + "';";

		work.exec(query_text);

		query_text = "DELETE FROM books WHERE id = '";
		query_text = query_text + book_id;
		query_text = query_text + "';";

		work.exec(query_text);

		work.commit();
	}

	//INITIALIZATION

	Database::Database(pqxx::connection connection)
		: connection_{ std::move(connection) }
	{
		pqxx::work work{ connection_ };
		work.exec(R"(CREATE TABLE IF NOT EXISTS authors ( id UUID CONSTRAINT author_id_constraint PRIMARY KEY, name varchar(100) UNIQUE NOT NULL );)"_zv);
		work.exec(R"(CREATE TABLE IF NOT EXISTS books ( id UUID CONSTRAINT book_id_constraint PRIMARY KEY, author_id UUID NOT NULL, title varchar(100) NOT NULL, publication_year integer);)"_zv);
		work.exec(R"(CREATE TABLE IF NOT EXISTS book_tags ( book_id UUID CONSTRAINT book_t_id_constraint PRIMARY KEY, tag varchar(30) UNIQUE);)"_zv);

		// ... создать другие таблицы

		// коммитим изменения
		work.commit();
	}

	/*
	 
		1. Show books +?
	
		2. Show book -/

		3. Add book +
	
		4. Edit book +

		5. Edit author +

		6. Delete book +

		7. Delete author +
	
	*/



}  // namespace postgres