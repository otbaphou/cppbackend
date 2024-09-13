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
		void AddBook(int pub_year, const std::string& title) override;
		const std::vector<domain::Author> GetAuthors() const override;
		const std::vector<domain::Book> GetBooks() const override;

	private:
		domain::AuthorRepository& authors_;
		domain::BookRepository& books_;
	};

}  // namespace app
