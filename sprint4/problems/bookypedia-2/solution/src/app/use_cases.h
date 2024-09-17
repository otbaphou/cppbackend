#pragma once

#include <string>
#include <vector>
#include "../domain/author_fwd.h"

namespace app 
{

	class UseCases
	{

	public:
		virtual void AddAuthor(const std::string& name) = 0;
		virtual void AddBook(int pub_year, const std::string& title, const std::string& author_id) = 0;
		virtual void DeleteAuthor(const std::string& author_id) = 0;
		virtual void DeleteBook(const std::string& book_id) = 0;

		virtual void EditAuthor(const std::string& author_id, const std::string new_name) = 0;
		virtual void EditBook(const std::string& book_id, const domain::BookRepresentation& new_data, const std::vector<std::string>& tags) = 0;

		virtual void SaveTags(const std::string& book_id, const std::vector<std::string>& tags) = 0;

		virtual const std::vector<domain::Author> GetAuthors() const = 0;
		virtual const std::vector<domain::BookRepresentation> GetBooks() const = 0;
		virtual const std::vector<std::string> GetTags(const std::string& book_id) const = 0;
		virtual const std::vector<domain::BookRepresentation> GetAuthorBooks(const std::string& author_id) const = 0;
		virtual const std::vector<domain::BookRepresentation> GetBooksWithName(const std::string& author_id) const = 0;
		virtual const domain::BookRepresentation GetBookById(const std::string& book_id) const = 0;
		virtual const std::string GetAuthorId(const std::string& name) const = 0;

	protected:
		~UseCases() = default;

	};

}  // namespace app
