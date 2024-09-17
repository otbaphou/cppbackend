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
    const domain::Author LoadSingle(const std::string& id) const override;
    const std::string GetAuthorId(const std::string& name) const override;

    void EditAuthor(const std::string& author_id, const std::string new_name) override;

    void DeleteAuthor(const std::string& id) override;
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
    const std::vector<domain::BookRepresentation> Load() const override;
    const std::vector<domain::BookRepresentation> LoadByAuthor(const std::string& author_id) const override;
    const std::vector<domain::BookRepresentation> LoadByName(const std::string& author_id) const override; 
    const domain::BookRepresentation LoadById(const std::string& book_id) const override;
    const std::vector<std::string> LoadTags(const std::string& book_id) const override; 
    void SaveTags(const std::string& book_id, const std::vector<std::string>& tags) override;

    void EditBook(const std::string& book_id, const domain::BookRepresentation& new_data, const std::vector<std::string>& tags) override;

    void DeleteBook(const std::string& id) override;

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