#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <cassert>
#include <iostream>


#include "../domain/author.h"
#include "../domain/book.h"
#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
    namespace detail {

        std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
            out << author.name;
            return out;
        }

        std::ostream& operator<<(std::ostream& out, const BookInfo& book) 
        {
            out << book.title << ", " << book.publication_year;
            return out;
        }

    }  // namespace detail

    std::vector<std::string> ParseTags(const std::string& raw_line)
    {
        std::vector<std::string> result;

        std::string tag = "";

        for (int i = 0; i < raw_line.size(); ++i)
        {
            char c = raw_line[i];

            if (tag == "" && c == ' ')
            {
                continue;
            }

            if (c == ' ' && tag[tag.size() - 1] == ' ')
            {
                continue;
            }

            if (c == ',')
            {
                if(!tag.empty())
                {
                    result.push_back(tag);
                    tag.clear();
                }
            }

            tag.push_back(c);
        }

        if(!tag.empty())
        {
            if (tag[tag.size() - 1] == ' ')
            {
                tag.pop_back();
            }

            if (std::find(result.begin(), result.end(), tag) == result.end())
            {
                result.push_back(tag);
            }
        }

        return result;
    }

    template <typename T>
    void PrintVector(std::ostream& out, const std::vector<T>& vector)
    {
        int i = 1;
        for (auto& value : vector)
        {
            out << i++ << " " << value << std::endl;
        }
    }

    void PrintBooks(std::ostream& out, const std::vector<detail::BookInfo>& vector)
    {
        int i = 1;
        for (const detail::BookInfo& book : vector)
        {
            out << i++ << " " << book.title << " by " << book.author_name << ", " << book.publication_year << std::endl;
        }
    }

    void PrintBookDetailed(std::ostream& out, const domain::BookRepresentation& book, const std::vector<std::string>& tags)
    {
        out << "Title: " << book.title << "\nAuthor: " << book.author_name << "\nPublication year: " << book.year << "\nTags: " << std::endl;

        for (int i = 0; i < tags.size(); ++i)
        {
            out << tags[i];

            if (i != tags.size() - 1)
            {
                out << ", ";
            }
            else
            {
                out << "\n";
            }
        }
    }

    View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
        : menu_{ menu }
        , use_cases_{ use_cases }
        , input_{ input }
        , output_{ output } {
        menu_.AddAction(  //
            "AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1)
            // ëèáî
            // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
        );
        menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s, std::bind(&View::AddBook, this, ph::_1));
        menu_.AddAction("DeleteBook"s, {}, "Deletes book"s, std::bind(&View::DeleteBook, this, ph::_1));
        menu_.AddAction("DeleteAuthor"s, {}, "Deletes author"s, std::bind(&View::DeleteAuthor, this, ph::_1));
        menu_.AddAction("EditBook"s, {}, "Deletes book"s, std::bind(&View::EditBook, this, ph::_1));
        menu_.AddAction("EditAuthor"s, {}, "Deletes author"s, std::bind(&View::EditAuthor, this, ph::_1));
        menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
        menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
        menu_.AddAction("ShowBook"s, {}, "Shows book"s, std::bind(&View::ShowBook, this, ph::_1));
        menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s, std::bind(&View::ShowAuthorBooks, this));
    }

    bool View::AddAuthor(std::istream& cmd_input) const 
    {
        try
        {
            std::string name;
            std::getline(cmd_input, name);
            boost::algorithm::trim(name);
            if (!name.empty())
            {
                use_cases_.AddAuthor(std::move(name));
            }
            else
            {
                output_ << "Failed to add author"sv << std::endl;
            }
        }
        catch (const std::exception&)
        {
            output_ << "Failed to add author"sv << std::endl;
        }
        return true;
    }

    bool View::AddBook(std::istream& cmd_input) const
    {
        try
        {
            if (auto params = GetBookParams(cmd_input))
            {
                if (!params.has_value())
                {
                    //throw std::invalid_argument("Failed to add the book..");
                    return true;
                }

                detail::AddBookParams p = params.value();
                
                if (!p.title.empty())
                {
                    use_cases_.AddBook(p.publication_year, p.title, p.author_id);

                    output_ << "Enter tags (comma separated):\n";

                    std::string tags;
                    std::getline(cmd_input, tags);
                    boost::algorithm::trim(tags);

                    auto& tmp_books = use_cases_.GetBooksWithName(p.title);

                    std::string current_id;

                    if (tmp_books.size() > 1)
                    {
                        for (auto& book : tmp_books)
                        {
                            if (book.author_id.ToString() == p.author_id)
                            {
                                current_id = book.book_id.ToString();
                            }
                        }
                    }

                    if(!current_id.empty())
                    {
                        use_cases_.SaveTags(current_id, ParseTags(tags));
                    }
                }

                return true;
            }
        }
        catch (const std::exception& ex)
        {
            output_ << "Failed to add book"sv << std::endl;
        }
        return true;
    }

    bool View::DeleteBook(std::istream& cmd_input) const
    {
        try
        {
            std::string title;
            std::getline(cmd_input, title);

            boost::algorithm::trim(title);

            if (title.empty())
            {
                ShowBooks();
            }

            auto books = use_cases_.GetBooksWithName(title);

            if (books.size() == 0)
            {
                throw std::invalid_argument("No book found to delete!");
            }
            else
            {
                if(books.size() > 1)
                { 
                    auto book_id = SelectBook();

                    if (book_id.has_value())
                    {
                        use_cases_.DeleteBook(book_id.value());
                    }
                    else
                    {
                        throw std::invalid_argument("Chosen Invalid Book To Delete!");
                    }
                }
                else
                {
                    use_cases_.DeleteBook(books[0].book_id.ToString());
                }
            }

        }
        catch (const std::exception& ex)
        {
            output_ << "Failed to delete book"sv << std::endl;
        }
        return true;
    }

    bool View::DeleteAuthor(std::istream& cmd_input) const
    {
        try
        {
            std::string name;
            std::getline(cmd_input, name);

            boost::algorithm::trim(name);

            std::string author_id;

            if (name.empty())
            {
                //ShowAuthors();           

                auto author = SelectAuthor(false);

                if (!author.has_value())
                {

                    return true;
                }
                else
                {
                    author_id = author.value();
                }
            }
            else
            {
                author_id = use_cases_.GetAuthorId(name);
            }

            if (author_id == "")
            {
                throw std::invalid_argument("Author does not exist!");
            }

            auto books = use_cases_.GetAuthorBooks(author_id);

            for (const auto& book : books)
            {
                use_cases_.DeleteBook(book.book_id.ToString());
            }

            use_cases_.DeleteAuthor(author_id);
            return true;
        }
        catch (const std::exception& ex)
        {
            output_ << "Failed to delete author"sv << std::endl;
        }
        return true;
    }

    bool View::EditAuthor(std::istream& cmd_input) const
    {
        try
        {
            std::string name;
            std::getline(cmd_input, name);

            boost::algorithm::trim(name);

            std::string author_id;

            if (name.empty())
            {
                //ShowAuthors();

                auto author = SelectAuthor(false);

                if (!author.has_value())
                {
                    return true;
                }
                else
                {
                    author_id = author.value();

                    if (author_id == "")
                    {
                        return true;
                    }
                }
            }
            else
            {
                author_id = use_cases_.GetAuthorId(name);

                if (author_id == "")
                {
                    throw std::invalid_argument("Author not found!!\n");
                    //return true;
                }
            }

            output_ << "Enter new name:\n";

            std::string new_name;
            std::getline(cmd_input, new_name);

            boost::algorithm::trim(new_name);

            use_cases_.EditAuthor(author_id, new_name);
        }
        catch (...)
        {
            output_ << "Failed to edit author\n";
        }

        return true;
    }

    std::pair<detail::BookInfo, std::vector<std::string>> View::MakeBookData(const detail::BookInfo& old_book, std::istream& cmd_input) const
    {
        detail::BookInfo book = old_book;

        output_ << "Enter new title or empty line to use the current one (" << old_book.title << "):";

        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        if (!title.empty())
        {
            book.title = title;
        }

        output_ << "Enter publication year or empty line to use the current one (" << old_book.publication_year << "):";

        std::string year;
        std::getline(cmd_input, year);
        boost::algorithm::trim(year);

        if (!year.empty())
        {
            book.publication_year = std::stoi(year);
        }

        auto tags = use_cases_.GetTags(old_book.id);

        output_ << "Enter tags (current tags: ";

        for (int i = 0; i < tags.size(); ++i)
        {
            if (i != tags.size() - 1)
            {
                output_ << tags[i] << ", ";
            }
            else
            {
                output_ << tags[i] << "):";
            }
        }        
        
        std::string tag_str;
        std::getline(cmd_input, tag_str);
        boost::algorithm::trim(tag_str);

        auto new_tags = ParseTags(tag_str);

        return { book, new_tags };
    }

    bool View::EditBook(std::istream& cmd_input) const
    {
        std::string title;
        std::getline(cmd_input, title);

        boost::algorithm::trim(title);

        std::string book_id;

        if (title.empty())
        {
            //ShowBooks();

            auto book = SelectBook();

            if (!book.has_value())
            {
                return true;
            }
            else
            {
                book_id = book.value();

                auto& book = use_cases_.GetBookById(book_id);
                detail::BookInfo tmp_book{ book.title, book.author_name, book.year, book.book_id.ToString() };
                std::pair<detail::BookInfo, std::vector<std::string>> book_shell = MakeBookData(tmp_book, cmd_input);

                domain::BookRepresentation result{book_shell.first.title, domain::BookId::FromString(book_id), book_shell.first.author_name, book_shell.first.publication_year};

                use_cases_.EditBook(book_id, result, book_shell.second);
            }
        }
        else
        {
            auto books = use_cases_.GetBooksWithName(title);

            if (books.size() == 0)
            {
                return true;
            }
            else
            {
                if (books.size() > 1)
                {
                    if (const auto& val = SelectBook())
                    {
                        if (val.has_value())
                        {
                            book_id = val.value();
                        }
                        else
                        {
                            throw std::invalid_argument("Invalid book edit selection!");
                        }
                    }
                }
                else
                {
                    book_id = books[0].book_id.ToString();
                }

                auto& book = use_cases_.GetBookById(book_id);
                detail::BookInfo tmp_book{ book.title, book.author_name, book.year, book.book_id.ToString() };
                std::pair<detail::BookInfo, std::vector<std::string>> book_shell = MakeBookData(tmp_book, cmd_input);

                domain::BookRepresentation result{ book_shell.first.title, domain::BookId::FromString(book_id), book_shell.first.author_name, book_shell.first.publication_year };

                use_cases_.EditBook(book_id, result, book_shell.second);
            }
            
        }

        output_ << "Enter new name:\n";

        std::string new_name;
        std::getline(cmd_input, new_name);

        boost::algorithm::trim(new_name);

        //use_cases_.EditAuthor(author_id, new_name);

        return true;
    }

    bool View::ShowAuthors() const 
    {
        PrintVector(output_, GetAuthors());
        return true;
    }

    bool View::ShowBooks() const 
    {
        PrintBooks(output_, GetBooks());
        return true;
    }

    bool View::ShowBook(std::istream& cmd_input) const
    {
        try
        {
            std::string title;

            std::getline(cmd_input, title);
            std::vector<domain::BookRepresentation> books = use_cases_.GetBooksWithName(title);

            if (!books.empty())
            {
                if (books.size() == 1)
                {
                    PrintBookDetailed(output_, books[0], GetTags(books[0].book_id.ToString()));//BROKEN
                }
                else
                {
                    auto book_id = SelectBook();
                }
            }
        }
        catch(...)
        {
            //throw std::invalid_argument("Failed to show book!");
        }

        return true;
    }

    bool View::ShowAuthorBooks() const
    {
        try
        {
            if (auto author_id = SelectAuthor(true))
            {
                PrintVector(output_, GetAuthorBooks(*author_id));
            }
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error("Failed to Show Books");
        }
        return true;
    }



    std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const 
    {
        detail::AddBookParams params;

        cmd_input >> params.publication_year;

        std::getline(cmd_input, params.title);

        boost::algorithm::trim(params.title);

        output_ << "Enter author name or empty line to select from list:\n";

        std::string author;
        std::getline(cmd_input, author);
        boost::algorithm::trim(author);

        if (author.empty())
        {
            auto author_id = SelectAuthor(true);

            if (!author_id.has_value())
            {
                return std::nullopt;
            }

            params.author_id = author_id.value();
            return params;
        }
        else
        {
            try
            {
                std::string author_id = use_cases_.GetAuthorId(author);
                params.author_id = author_id;

                if (author_id != "")
                {
                    return params;
                }
                else
                {
                    throw std::invalid_argument("Author not found.");
                }
            }
            catch (...)
            {
                output_ << "No author found. Do you want to add Jack London (y/n)?\n";

                std::string response;
                std::getline(cmd_input, response);
                boost::algorithm::trim(response);

                if (response.size() != 1 || response[0] != 'Y' || response[0] != 'y')
                {
                    return std::nullopt;
                }

                use_cases_.AddAuthor(author);

                std::string author_id = use_cases_.GetAuthorId(author);
                params.author_id = author_id;
                return params;
            }
        }

    }

    std::optional<std::string> View::SelectAuthor(bool first_line) const 
    {
        if(first_line)
        {
            output_ << "Select author:" << std::endl;
        }

        auto authors = GetAuthors();
        PrintVector(output_, authors);
        output_ << "Enter author # or empty line to cancel" << std::endl;

        std::string str;
        if (!std::getline(input_, str) || str.empty()) {
            return std::nullopt;
        }

        int author_idx;
        try {
            author_idx = std::stoi(str);
        }
        catch (std::exception const&) {
            throw std::runtime_error("Invalid author num");
        }

        --author_idx;
        if (author_idx < 0 || author_idx >= authors.size())
        {
            throw std::runtime_error("Invalid author num");
        }

        return authors[author_idx].id;
    }

    std::optional<std::string> View::SelectBook() const 
    {   
        auto books = GetBooks();
        PrintBooks(output_, books);
        output_ << "Enter the book # or empty line to cancel:" << std::endl;

        std::string str;
        if (!std::getline(input_, str) || str.empty()) {
            return std::nullopt;
        }

        int book_idx;
        try {
            book_idx = std::stoi(str);
        }
        catch (std::exception const&) {
            throw std::runtime_error("Invalid author num");
        }

        --book_idx;
        if (book_idx < 0 || book_idx >= books.size())
        {
            throw std::runtime_error("Invalid author num");
        }

        return books[book_idx].id;
    }

    std::vector<detail::AuthorInfo> View::GetAuthors() const
    {
        std::vector<detail::AuthorInfo> dst_authors;

        for (const domain::Author& author : use_cases_.GetAuthors())
        {
            dst_authors.emplace_back(author.GetId().ToString(), author.GetName());
        }

        return dst_authors;
    }

    std::vector<detail::BookInfo> View::GetBooks() const
    {
        std::vector<detail::BookInfo> books;

        for (const domain::BookRepresentation& book : use_cases_.GetBooks())
        {
            books.emplace_back(book.title, book.author_name, book.year, book.book_id.ToString());
        }

        return books;
    }

    std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const
    {
        std::vector<detail::BookInfo> books;

        for (const domain::BookRepresentation& book : use_cases_.GetAuthorBooks(author_id))
        {
            books.emplace_back(book.title, book.author_name, book.year);
        }

        return books;
    }    
    
    std::vector<std::string> View::GetTags(const std::string& book_id) const
    {
        std::vector<std::string> tags;

        for (const std::string& tag : use_cases_.GetTags(book_id))
        {
            tags.push_back(tag);
        }

        return tags;
    }

}  // namespace ui
