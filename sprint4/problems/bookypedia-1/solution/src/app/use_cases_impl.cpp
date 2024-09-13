#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

#include <iostream>

namespace app
{
	using namespace domain;

	void UseCasesImpl::AddAuthor(const std::string& name)
	{
		authors_.Save({ AuthorId::New(), name });
	}


	void UseCasesImpl::AddBook(int pub_year, const std::string& title, const std::string& author_id)
	{
		books_.Save({BookId::New(), AuthorId::FromString(author_id), title, pub_year });
	}

	const std::vector<domain::Author> UseCasesImpl::GetAuthors() const
	{
		return authors_.Load();
	}

	const std::vector<domain::Book> UseCasesImpl::GetBooks() const
	{
		return books_.Load();
	}
	
	const std::vector<domain::Book> UseCasesImpl::GetAuthorBooks(const std::string& author_id) const
	{
		return books_.LoadByAuthor(author_id);
	}

}  // namespace app
