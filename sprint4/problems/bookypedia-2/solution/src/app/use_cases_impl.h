#pragma once
#include "../domain/author_fwd.h"
#include "use_cases.h"

namespace app 
{

	class UseCasesImpl : public UseCases
	{
	public:
		explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
			: authors_{ authors },
			  books_{books}
		{}

		void AddAuthor(const std::string& name) override;
		void AddBook(int pub_year, const std::string& title, const std::string& author_id) override;
		void DeleteAuthor(const std::string& author_id) override;
		void DeleteBook(const std::string& book_id) override;

		void EditAuthor(const std::string& author_id, const std::string new_name) override;
		void EditBook(const std::string& book_id, const domain::BookRepresentation& new_data, const std::vector<std::string>& tags) override;

		void SaveTags(const std::string& book_id, const std::vector<std::string>& tags) override;

		const std::vector<domain::Author> GetAuthors() const override;
		const std::vector<domain::BookRepresentation> GetBooks() const override;
		const std::vector<std::string> GetTags(const std::string& book_id) const override;
		const std::vector<domain::BookRepresentation> GetAuthorBooks(const std::string& author_id) const override;
		const std::vector<domain::BookRepresentation> GetBooksWithName(const std::string& author_id) const override; 
		const domain::BookRepresentation GetBookById(const std::string& book_id) const override;
		const std::string GetAuthorId(const std::string& name) const override;

	private:
		domain::AuthorRepository& authors_;
		domain::BookRepository& books_;
	};

}  // namespace app
