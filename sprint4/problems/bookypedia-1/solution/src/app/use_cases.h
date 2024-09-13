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
		virtual void AddBook(int pub_year, const std::string& title) = 0;
		virtual const std::vector<domain::Author> GetAuthors() const = 0;
		virtual const std::vector<domain::Book> GetBooks() const = 0;;
	protected:
		~UseCases() = default;

	};

}  // namespace app
