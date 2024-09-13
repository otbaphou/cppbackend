#pragma once
#include <string>

#include "../util/tagged_uuid.h"
#include "author.h"

namespace domain 
{

    namespace detail 
    {
        struct BookTag {};
    }  // namespace detail

    using BookId = util::TaggedUUID<detail::BookTag>;

    class Book 
    {
    public:
        Book(BookId id, AuthorId author_id, std::string name, int year)
            : id_(std::move(id)),
              author_id_(author_id),
              title_(std::move(name)),
              release_year_(year) 
        {}

        const BookId& GetId() const noexcept 
        {
            return id_;
        }

        const AuthorId& GetAuthorId() const noexcept
        {
            return author_id_;
        }

        const std::string& GetName() const noexcept 
        {
            return title_;
        }

        const int GetReleaseYear() const noexcept
        {
            return release_year_;
        }

    private:
        BookId id_;
        AuthorId author_id_;
        std::string title_;
        int release_year_;
    };

    class BookRepository 
    {
    public:
        virtual void Save(const Book& book) = 0;
        virtual const std::vector<domain::Book> Load() const = 0;
        virtual const std::vector<domain::Book> LoadByAuthor(const std::string& author_id) const = 0;

    protected:
        ~BookRepository() = default;
    };

}  // namespace domain
