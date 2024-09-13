#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }
    
    void Save(const domain::Author& author) override;
    const std::vector<domain::Author> Load() const override;
private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository 
{
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{ connection } 
    {}

    void Save(const domain::Book& book) override;
    const std::vector<domain::Book> Load() const override;	
    const std::vector<domain::Book> LoadByAuthor(const std::string& author_id) const override;

private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks()& 
    {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl books_{ connection_ };
};

}  // namespace postgres