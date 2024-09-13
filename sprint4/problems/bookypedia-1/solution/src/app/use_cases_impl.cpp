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


	void UseCasesImpl::AddBook(int pub_year, const std::string& title)
	{
		//TODO: Complete this operation
		//Show the list of authors on request
		//Pass the selected id on successful attempt

		int author_id;
		std::cout << "Select author:" << std::endl;
		std::cin >> author_id;
		books_.Save({BookId::New(), AuthorId::New(), title, pub_year });
	}

	const std::vector<domain::Author> UseCasesImpl::GetAuthors() const
	{
		return authors_.Load();
	}

	const std::vector<domain::Book> UseCasesImpl::GetBooks() const
	{
		return books_.Load();
	}

}  // namespace app
