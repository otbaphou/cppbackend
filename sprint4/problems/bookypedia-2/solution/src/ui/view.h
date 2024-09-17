#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AddBookParams {
    std::string title;
    std::string author_id;
    int publication_year = 0;
};

struct AuthorInfo {
    std::string id;
    std::string name;
};

struct BookInfo 
{
    std::string title;
    std::string author_name;
    int publication_year;
    std::string id;
};

}  // namespace detail

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool AddBook(std::istream& cmd_input) const;
    bool DeleteBook(std::istream& cmd_input) const;
    bool DeleteAuthor(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowBooks() const;
    bool ShowBook(std::istream& cmd_input) const;
    bool ShowAuthorBooks() const;
    bool EditAuthor(std::istream& cmd_input) const;
    bool EditBook(std::istream& cmd_input) const;

    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    std::optional<std::string> SelectAuthor(bool) const;
    std::optional<std::string> SelectBook() const;
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::vector<detail::BookInfo> GetBooks() const;    
    std::vector<std::string> GetTags(const std::string& book_id) const;
    //detail::BookInfo GetBook(const std::string id) const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;

    std::pair<detail::BookInfo, std::vector<std::string>> MakeBookData(const detail::BookInfo& old_book, std::istream& cmd_input) const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui