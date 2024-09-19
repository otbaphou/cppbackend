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

	const std::vector<domain::BookRepresentation> UseCasesImpl::GetBooks() const
	{
		return books_.Load();
	}
	
	const std::vector<domain::BookRepresentation> UseCasesImpl::GetAuthorBooks(const std::string& author_id) const
	{
		return books_.LoadByAuthor(author_id);
	}

	const std::vector<domain::BookRepresentation> UseCasesImpl::GetBooksWithName(const std::string& book_name) const
	{
		return books_.LoadByName(book_name);
	}

	const domain::BookRepresentation UseCasesImpl::GetBookById(const std::string& book_id) const
	{
		return books_.LoadById(book_id);
	}

	const std::vector<std::string> UseCasesImpl::GetTags(const std::string& book_id) const
	{
		return books_.LoadTags(book_id);
	}

	void UseCasesImpl::DeleteAuthor(const std::string& author_id)
	{
		authors_.DeleteAuthor(author_id);
	}

	void UseCasesImpl::DeleteBook(const std::string& book_id)
	{
		books_.DeleteBook(book_id);
	}
	
	const std::string UseCasesImpl::GetAuthorName(const std::string& id) const
	{
		return authors_.GetAuthorName(id);
	}
	
	const std::string UseCasesImpl::GetAuthorId(const std::string& name) const
	{
		return authors_.GetAuthorId(name);
	}

	void UseCasesImpl::EditAuthor(const std::string& author_id, const std::string new_name)
	{
		authors_.EditAuthor(author_id, new_name);
	}

	void UseCasesImpl::EditBook(const std::string& book_id, const domain::BookRepresentation& new_data, const std::vector<std::string>& tags)
	{
		books_.EditBook(book_id, new_data, tags);
	}

	void UseCasesImpl::SaveTags(const std::string& book_id, const std::vector<std::string>& tags)
	{
		books_.SaveTags(book_id, tags);
	}

}  // namespace app
