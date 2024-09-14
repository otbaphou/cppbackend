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

        std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
            out << book.title << ", " << book.publication_year;
            return out;
        }

    }  // namespace detail

    template <typename T>
    void PrintVector(std::ostream& out, const std::vector<T>& vector)
    {
        int i = 1;
        for (auto& value : vector)
        {
            out << i++ << " " << value << std::endl;
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
        menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
            std::bind(&View::AddBook, this, ph::_1));
        menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
        menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
        menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
            std::bind(&View::ShowAuthorBooks, this));
    }

    bool View::AddAuthor(std::istream& cmd_input) const {
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

    bool View::AddBook(std::istream& cmd_input) const {
        try
        {
            if (auto params = GetBookParams(cmd_input))
            {
                detail::AddBookParams p = params.value();

                if (p.title.empty())
                {
                    assert(0 == 1);
                    //use_cases_.AddBook(p.publication_year, p.title, p.author_id);
                }
            }/*
            else
            {
                return false;
            }*/

        }
        catch (const std::exception& ex)
        {
            //output_ << "Failed to add book"sv << std::endl;
            output_ << "Failed to add book: "sv << ex.what() << std::endl;
        }
        return true;
    }

    bool View::ShowAuthors() const {
        PrintVector(output_, GetAuthors());
        return true;
    }

    bool View::ShowBooks() const {
        PrintVector(output_, GetBooks());
        return true;
    }

    bool View::ShowAuthorBooks() const 
    {
        try
        {
            if (auto author_id = SelectAuthor())
            {
                PrintVector(output_, GetAuthorBooks(*author_id));
            }/*
            else
            {
                return true;
            }*/
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error("Failed to Show Books");
        }
        return true;
    }

    std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
        detail::AddBookParams params;

        cmd_input >> params.publication_year;

        cmd_input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(cmd_input, params.title);

        boost::algorithm::trim(params.title);

        auto author_id = SelectAuthor();

        if (!author_id.has_value())
        {
            return std::nullopt;
        }

        params.author_id = author_id.value();
        return params;
    }

    std::optional<std::string> View::SelectAuthor() const {
        output_ << "Select author:" << std::endl;
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

        for (const domain::Book& book : use_cases_.GetBooks())
        {
            books.emplace_back(book.GetName(), book.GetReleaseYear());
        }

        return books;
    }

    std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const
    {
        std::vector<detail::BookInfo> books;

        for (const domain::Book& book : use_cases_.GetAuthorBooks(author_id))
        {
            books.emplace_back(book.GetName(), book.GetReleaseYear());
        }

        return books;
    }

}  // namespace ui
