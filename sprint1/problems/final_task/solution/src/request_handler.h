#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>

namespace http_handler
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace json = boost::json;
	using namespace std::literals;

	// Запрос, тело которого представлено в виде строки
	using StringRequest = http::request<http::string_body>;
	// Ответ, тело которого представлено в виде строки
	using StringResponse = http::response<http::string_body>;

	struct ContentType
	{
		ContentType() = delete;
		constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
		// При необходимости внутрь ContentType можно добавить и другие типы контента
	};

	// Создаёт StringResponse с заданными параметрами
	StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, 
	bool keep_alive, std::string_view content_type = ContentType::APPLICATION_JSON);

	//Data Packets
	void PackMaps(json::array& target_container, model::Game& game);

	void PackRoads(json::array& target_container, const model::Map* map_ptr);
	void PackBuildings(json::array& target_container, const model::Map* map_ptr);
	void PackOffices(json::array& target_container, const model::Map* map_ptr);

	StringResponse HandleRequest(auto&& req, model::Game& game)
	{

		const auto text_response = [&req](http::status status, std::string_view text)
			{
				return MakeStringResponse(status, text, req.version(), req.keep_alive());
			};

		std::string_view req_type = req.method_string();

		if (req_type != "GET"sv)
		{
			return text_response(http::status::method_not_allowed, { "Invalid method" });
		}

		std::string_view target = req.target();

		size_t size = target.size();

		if (size >= 4)
		{

			if (std::string_view(target.begin(), target.begin() + 5) == "/api/"sv)
			{
				if (target == "/api/v1/maps"sv || target == "/api/v1/maps/"sv)
				{
					json::array response;

					//Inserting Map Data Into JSON Array
					PackMaps(response, game);

					return text_response(http::status::ok, { json::serialize(response) });
				}

				if (size >= 13)
				{
					if (std::string_view(target.begin(), target.begin() + 13) == "/api/v1/maps/"sv)
					{
						using Id = util::Tagged<std::string, model::Map>;
						Id id{ std::string(target.begin() + 13, target.end()) };

						const model::Map* maptr = game.FindMap(id);

						if (maptr != nullptr)
						{
							//JSON object with map data
							json::object response;

							//Initializing object using map id and name
							response.emplace("id", *maptr->GetId());
							response.emplace("name", maptr->GetName());

							//Inserting Roads
							json::array roads;
							PackRoads(roads, maptr);

							response.emplace("roads", std::move(roads));

							//Inserting Buildings
							json::array buildings;
							PackBuildings(buildings, maptr);

							response.emplace("buildings", std::move(buildings));

							//Inserting Offices
							json::array offices;
							PackOffices(offices, maptr);

							response.emplace("offices", std::move(offices));

							return text_response(http::status::ok, { json::serialize(response) });
						}
						else
						{
							//Throwing error if requested map isn't found

							json::object response;

							response.emplace("code", "mapNotFound");
							response.emplace("message", "Map not found");

							return text_response(http::status::not_found, { json::serialize(response) });
						}
					}
				}
				//Throwing error when URL starts with /api/ but doesn't correlate to any of the commands

				json::object elem;

				elem.emplace("code", "badRequest");
				elem.emplace("message", "Bad request");
			}				
			//Throwing error when URL starts with /api/ but doesn't correlate to any of the commands

			json::object response;

			response.emplace("code", "badRequest");
			response.emplace("message", "Bad request");

			return text_response(http::status::bad_request, { json::serialize(response) });
		}

		std::string response_str = "Why Are We Here?";

		return text_response(http::status::ok, { response_str });

	}

	//RequestHandler
	class RequestHandler
	{
	public:
		explicit RequestHandler(model::Game& game)
			: game_{ game }
		{}

		RequestHandler(const RequestHandler&) = delete;
		RequestHandler& operator=(const RequestHandler&) = delete;

		template <typename Body, typename Allocator, typename Send>
		void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send)
		{
			// Обработать запрос request и отправить ответ, используя send
			send(HandleRequest(req, game_));
		}

	private:
		model::Game& game_;
	};

}  // namespace http_handler
