#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>
#include <map>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <variant>
bool IsValidToken(std::string token);
namespace http_handler
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace json = boost::json;
	namespace fs = std::filesystem;
	namespace sys = boost::system;
	using namespace std::literals;
	using FileRequest = http::request<http::file_body>;
	using FileResponse = http::response<http::file_body>;
	using StringRequest = http::request<http::string_body>;
	using StringResponse = http::response<http::string_body>;
	struct ContentType
	{
		ContentType() = delete;
		constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
		constexpr static std::string_view APPLICATION_XML = "application/xml"sv;
		constexpr static std::string_view APPLICATION_BLANK = "application/octet-stream"sv;
		constexpr static std::string_view TEXT_JS = "text/javascript"sv;
		constexpr static std::string_view TEXT_CSS = "text/css"sv;
		constexpr static std::string_view TEXT_TXT = "text/plain"sv;
		constexpr static std::string_view TEXT_HTML = "text/html"sv;
		constexpr static std::string_view IMAGE_PNG = "image/png"sv;
		constexpr static std::string_view IMAGE_JPG = "image/jpeg"sv;
		constexpr static std::string_view IMAGE_GIF = "image/gif"sv;
		constexpr static std::string_view IMAGE_BMP = "image/bmp"sv;
		constexpr static std::string_view IMAGE_ICO = "image/vnd.microsoft.icon"sv;
		constexpr static std::string_view IMAGE_TIF = "image/tiff"sv;
		constexpr static std::string_view IMAGE_SVG = "image/svg+xml"sv;
		constexpr static std::string_view AUDIO_MP3 = "audio/mpeg"sv;
	};
	//Creates a response using given parameters
	StringResponse MakeStringResponse(http::status status, std::string_view body,
		unsigned http_version, bool keep_alive, std::string_view content_type);
	FileResponse MakeResponse(http::status status, http::file_body::value_type& body,
		unsigned http_version, bool keep_alive, std::string_view content_type);
	//Data Packets
	void PackMaps(json::array& target_container, model::Game& game);
	void PackRoads(json::array& target_container, const model::Map* map_ptr);
	void PackBuildings(json::array& target_container, const model::Map* map_ptr);
	void PackOffices(json::array& target_container, const model::Map* map_ptr);
	// Returns true, if catalogue p is inside base_path.
	bool IsSubPath(fs::path path, fs::path base);
	// Returns content type by raw extension string_view
	std::string_view GetContentType(std::string_view extension);
	void LogResponse(auto time, int code, std::string_view content_type)
	{
		//Logging request
		json::object logger_data{ {"response_time", time}, {"code", code} };
		if (content_type.empty())
		{
			logger_data.emplace("content_type", "null");
		}
		else
		{
			std::string type{ content_type }; // :(
			logger_data.emplace("content_type", type);
		}
		BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "response sent"sv;
	}
	template <typename Send>
	void HandleRequestFile(Send&& send, model::Game& game, fs::path& requested_path,
		const auto& text_response, const auto& file_response)
	{
		if (!fs::exists(requested_path))
		{
			send(text_response(http::status::not_found, { "You got the wrong door, buddy." }, ContentType::TEXT_TXT));
			return;
		}
		//###Extension checks###
		std::error_code ec;
		//File is a directory?
		if (fs::is_directory(requested_path, ec))
		{
			requested_path = requested_path / fs::path{ "index.html" };
		}
		if (ec) // Optional handling of possible errors.
		{
			//Adding custom error logging in here too, hoping it won't mess up the tests and break everything
			json::object logger_data{ {"code", "dirCheckError"}, {"exception", ec.message()} };
			BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "error"sv;
		}
		//File is a.. file?!
		if (fs::is_regular_file(requested_path, ec))
		{
			std::string extension = requested_path.extension().string();
			//Stupid check
			for (char& c : extension)
			{
				c = tolower(c);
			}
			http::file_body::value_type file;
			if (sys::error_code ec; file.open(requested_path.string().c_str(), beast::file_mode::read, ec), ec)
			{
				json::object logger_data{ {"code", "fileLoadingError"}, {"exception", ec.message()} };
				BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "error"sv;
				return;
			}
			send(file_response(http::status::ok, { file }, GetContentType(extension)));
			return;
		}
		//Something else???
		if (ec)
		{
			json::object logger_data{ {"code", "fileCheckError"}, {"exception", ec.message()} };
			BOOST_LOG_TRIVIAL(info) << logging::add_value(timestamp, pt::microsec_clock::local_time()) << logging::add_value(additional_data, logger_data) << "error"sv;
		}
		send(text_response(http::status::ok, { "Why Are We Here?" }, ContentType::TEXT_HTML));
		return;
	}
	template <typename Send>
	void HandleRequestAPI(Send&& send, model::Game& game, std::string_view target, const auto& text_response, const auto& request)
	{
		std::string_view req_type = request.method_string();

		if (target == "/api/v1/maps"sv || target == "/api/v1/maps/"sv)
		{
			if (req_type != "GET"sv && req_type != "HEAD"sv)
			{
				send(text_response(http::status::method_not_allowed, { "Invalid method" }, ContentType::APPLICATION_JSON));
				return;
			}
			json::array response;
			//Inserting Map Data Into JSON Array
			PackMaps(response, game);
			StringResponse str_response{ text_response(http::status::ok, { json::serialize(response) }, ContentType::APPLICATION_JSON) };
			str_response.set(http::field::cache_control, "no-cache");
			send(str_response);
			return;
		}

		if (target == "/api/v1/game/tick"sv || target == "/api/v1/game/tick/"sv)
		{
			json::object response;
			http::status response_status;
			bool allow_post = false;

			if (req_type != "POST"sv)
			{
				response.emplace("code", "invalidMethod");
				response.emplace("message", "Only POST method is expected");
				response_status = http::status::method_not_allowed;
				allow_post = true;
			}
			else
			{
				try
				{
					auto value = json::parse(request.body());
					int ticks = 0;
					ticks = value.as_object().at("timeDelta").as_int64();

					//Method not allowed if method isn't POST
						//Bad request error if player name is invalid
					if (ticks == 0)
					{
						response.emplace("code", "invalidArgument");
						response.emplace("message", "Failed to parse tick request JSON");
						response_status = http::status::bad_request;
					}
					else
					{
						game.ServerTick(ticks); //This shit (no longer) breaks
						response_status = http::status::ok;
					}
				}
				catch (...)
				{
					response.emplace("code", "invalidArgument");
					response.emplace("message", "Failed to parse tick request JSON");
					response_status = http::status::bad_request;
				}
			}

			StringResponse str_response{ text_response(response_status, { json::serialize(response) }, ContentType::APPLICATION_JSON) };
			str_response.set(http::field::cache_control, "no-cache");
			if (allow_post)
			{
				str_response.set(http::field::allow, "POST"sv);
			}
			send(str_response);
			return;
		}

		if (target == "/api/v1/game/join"sv || target == "/api/v1/game/join/"sv)
		{
			json::object response;
			http::status response_status;
			bool allow_post = false;

			if (req_type != "POST"sv)
			{
				response.emplace("code", "invalidMethod");
				response.emplace("message", "Only POST method is expected");
				response_status = http::status::method_not_allowed;
				allow_post = true;
			}
			else
			{
				try
				{
					auto value = json::parse(request.body());
					std::string username{ value.as_object().at("userName").as_string() };
					std::string map_id{ value.as_object().at("mapId").as_string() };
					using Id = util::Tagged<std::string, model::Map>;
					Id id{ map_id };
					const model::Map* maptr = game.FindMap(id);
					//Method not allowed if method isn't POST
						//Bad request error if player name is invalid
					if (username.empty())
					{
						response.emplace("code", "invalidArgument");
						response.emplace("message", "Invalid name");
						response_status = http::status::bad_request;
					}
					else
					{
						//Not found error if requested map doesn't exist
						if (maptr == nullptr)
						{
							response.emplace("code", "mapNotFound");
							response.emplace("message", "Map not found");
							response_status = http::status::not_found;
						}
						else
						{
							//Everything is okay here. Really.
							std::string token = game.SpawnPlayer(username, maptr);
							response.emplace("authToken", token);
							response.emplace("playerId", game.FindPlayerByToken(token)->GetId());
							response_status = http::status::ok;
						}
					}
				}
				catch (...)
				{
					response.emplace("code", "invalidArgument");
					response.emplace("message", "Join game request parse error");
					response_status = http::status::bad_request;
				}
			}
			StringResponse str_response{ text_response(response_status, { json::serialize(response) }, ContentType::APPLICATION_JSON) };
			str_response.set(http::field::cache_control, "no-cache");
			if (allow_post)
			{
				str_response.set(http::field::allow, "POST"sv);
			}
			send(str_response);
			return;
		}

		if (target == "/api/v1/game/players"sv || target == "/api/v1/game/players/"sv)
		{
			bool allow_get_head = false;
			json::object response;
			http::status response_status;
			std::string token;
			if (req_type != "GET"sv && req_type != "HEAD"sv)
			{
				response.emplace("code", "invalidMethod");
				response.emplace("message", "Invalid method");
				response_status = http::status::method_not_allowed;
				allow_get_head = true;
			}
			else
			{
				try
				{
					auto auth = request.find(http::field::authorization);
					std::string auth_str = std::string(auth->value());
					assert(auth_str.size() == 39);
					token = auth_str.substr(7, 32);
					assert(token.size() == 32);
				}
				catch (...)
				{
					response.emplace("code", "invalidToken");
					response.emplace("message", "Authorization header is missing");
					response_status = http::status::unauthorized;
				}
				if (token.size() == 32)
				{
					model::Player* player_ptr = game.FindPlayerByToken(token);
					if (player_ptr == nullptr)
					{
						response.emplace("code", "unknownToken");
						response.emplace("message", "Player token has not been found");
						response_status = http::status::unauthorized;
					}
					else
					{
						for (model::Player* player : game.GetPlayerList(*(player_ptr->GetCurrentMap()->GetId())))
						{
							json::object entry;
							entry.emplace("name", player->GetName());
							response.emplace(std::to_string(static_cast<int>(player->GetId())), entry);
						}
						response_status = http::status::ok;
					}
				}
				else
				{
					//For some reason it doesn't work without it..
					response.emplace("code", "invalidToken");
					response.emplace("message", "Authorization header is required");
					response_status = http::status::unauthorized;
				}
			}
			StringResponse str_response{ text_response(response_status, { json::serialize(response) }, ContentType::APPLICATION_JSON) };
			str_response.set(http::field::cache_control, "no-cache");
			if (allow_get_head)
			{
				str_response.set(http::field::allow, "GET, HEAD"sv);
			}
			send(str_response);
			return;
		}
		if (target == "/api/v1/game/state"sv || target == "/api/v1/game/state/"sv)
		{
			bool allow_get_head = false;
			json::object response;
			http::status response_status;
			std::string token;
			if (req_type != "GET"sv && req_type != "HEAD"sv)
			{
				response.emplace("code", "invalidMethod");
				response.emplace("message", "Invalid method");
				response_status = http::status::method_not_allowed;
				allow_get_head = true;
			}
			else
			{
				try
				{
					auto auth = request.find(http::field::authorization);
					std::string auth_str = std::string(auth->value());
					assert(auth_str.size() == 39);
					token = auth_str.substr(7, 32);
					assert(token.size() == 32);
				}
				catch (...)
				{
					response.emplace("code", "invalidToken");
					response.emplace("message", "Authorization header is required");
					response_status = http::status::unauthorized;
				}
				if (token.size() == 32)
				{
					model::Player* player_ptr = game.FindPlayerByToken(token);
					if (player_ptr == nullptr)
					{
						response.emplace("code", "unknownToken");
						response.emplace("message", "Player token has not been found");
						response_status = http::status::unauthorized;
					}
					else
					{
						json::object final_obj;
						for (model::Player* player : game.GetPlayerList(*(player_ptr->GetCurrentMap()->GetId())))
						{
							json::object entry;
							model::Coordinates pos = player->GetPos();
							json::array pos_arr;
							pos_arr.push_back(pos.x);
							pos_arr.push_back(pos.y);
							entry.emplace("pos", pos_arr);
							model::Velocity vel = player->GetVel();
							json::array vel_arr;
							vel_arr.push_back(vel.x);
							vel_arr.push_back(vel.y);
							entry.emplace("speed", vel_arr);
							entry.emplace("dir", std::string{ player->GetDir() });
							final_obj.emplace(std::to_string(static_cast<int>(player->GetId())), entry);
						}
						response.emplace("players", final_obj);
						response_status = http::status::ok;
					}
				}
				else
				{
					//For some reason it doesn't work without it..
					response.emplace("code", "invalidToken");
					response.emplace("message", "Authorization header is required");
					response_status = http::status::unauthorized;
				}
			}
			StringResponse str_response{ text_response(response_status, { json::serialize(response) }, ContentType::APPLICATION_JSON) };
			str_response.set(http::field::cache_control, "no-cache");
			if (allow_get_head)
			{
				str_response.set(http::field::allow, "GET, HEAD"sv);
			}
			send(str_response);
			return;
		}
		if (target == "/api/v1/game/player/action"sv || target == "/api/v1/game/player/action/"sv)
		{
			json::object response;
			http::status response_status;
			bool allow_post = false;
			std::string token;
			bool valid_content_type = true;
			try
			{
				auto content_type = request.find(http::field::content_type);
				std::string content_type_str = std::string(content_type->value());
				if (content_type_str != "application/json")
				{
					valid_content_type = false;
					response.emplace("code", "invalidArgument");
					response.emplace("message", "Invalid content type");
					response_status = http::status::bad_request;
				}
			}
			catch (...)
			{
				valid_content_type = false;
				response.emplace("code", "invalidArgument");
				response.emplace("message", "Invalid content type");
				response_status = http::status::bad_request;
			}
			if (req_type != "POST"sv)
			{
				response.emplace("code", "invalidMethod");
				response.emplace("message", "Invalid method");
				response_status = http::status::method_not_allowed;
				allow_post = true;
			}
			else
			{
				if (valid_content_type)
				{
					try
					{
						auto auth = request.find(http::field::authorization);
						std::string auth_str = std::string(auth->value());
						assert(auth_str.size() == 39);
						token = auth_str.substr(7, 32);
						assert(token.size() == 32);
					}
					catch (...)
					{
						response.emplace("code", "invalidToken");
						response.emplace("message", "Authorization header is required");
						response_status = http::status::unauthorized;
					}
					if (token.size() == 32)
					{
						try
						{
							bool failed = false;
							auto value = json::parse(request.body());
							if (!value.as_object().contains("move"))
							{
								failed = true;
							}
							std::string user_input;
							if (!failed)
							{
								user_input = value.as_object().at("move").as_string();
							}
							model::Player* player = game.FindPlayerByToken(token);
							if (player != nullptr)
							{
								if (failed)
								{
									response.emplace("code", "invalidArgument");
									response.emplace("message", "Failed to parse action");
									response_status = http::status::bad_request;
								}
								else
								{
									if (user_input.empty())
									{
										player->SetVel(0, 0);
									}
									else
									{
										char input_char = user_input[0];
										switch (input_char)
										{
										case 'U':
											player->SetVel(0, -1);
											player->SetDir(model::Direction::NORTH);
											break;
										case 'D':
											player->SetVel(0, 1);
											player->SetDir(model::Direction::SOUTH);
											break;
										case 'L':
											player->SetVel(-1, 0);
											player->SetDir(model::Direction::WEST);
											break;
										case 'R':
											player->SetVel(1, 0);
											player->SetDir(model::Direction::EAST);
											break;
										default:
											failed = true;
											break;
										}
									}
									response_status = http::status::ok;
								}
							}
							else
							{
								response.emplace("code", "unknownToken");
								response.emplace("message", "Player token has not been found");
								response_status = http::status::unauthorized;
							}
						}
						catch (...)
						{
							response.emplace("code", "invalidArgument");
							response.emplace("message", "Failed to parse action");
							response_status = http::status::bad_request;
						}
					}
					else
					{
						//For some reason it doesn't work without it..
						response.emplace("code", "invalidToken");
						response.emplace("message", "Authorization header is required");
						response_status = http::status::unauthorized;
					}
				}
			}
			StringResponse str_response{ text_response(response_status, { json::serialize(response) }, ContentType::APPLICATION_JSON) };
			str_response.set(http::field::cache_control, "no-cache");
			if (allow_post)
			{
				str_response.set(http::field::allow, "POST"sv);
			}
			send(str_response);
			return;
		}
		if (req_type != "GET"sv && req_type != "HEAD"sv)
		{
			send(text_response(http::status::method_not_allowed, { "Invalid method" }, ContentType::APPLICATION_JSON));
			return;
		}
		if (target.size() >= 13)
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
					//Printing response
					send(text_response(http::status::ok, { json::serialize(response) }, ContentType::APPLICATION_JSON));
					return;
				}
				else
				{
					//Throwing error if requested map isn't found
					json::object response;
					response.emplace("code", "mapNotFound");
					response.emplace("message", "Map not found");
					send(text_response(http::status::not_found, { json::serialize(response) }, ContentType::APPLICATION_JSON));
					return;
				}
			}
		}
		//Throwing error when URL starts with /api/ but doesn't correlate to any of the commands
		json::object response;
		response.emplace("code", "badRequest");
		response.emplace("message", "Bad request");
		send(text_response(http::status::bad_request, { json::serialize(response) }, ContentType::APPLICATION_JSON));
		return;
	}
	template <typename Send>
	void HandleRequest(auto&& req, model::Game& game, const fs::path& static_path, Send&& send)
	{
		using std::chrono::duration_cast;
		using std::chrono::microseconds;
		std::chrono::system_clock::time_point request_start = std::chrono::system_clock::now();
		const auto text_response = [&req, request_start](http::status status, std::string_view body, std::string_view content_type = ContentType::APPLICATION_JSON)
			{
				StringResponse response{ MakeStringResponse(status, body, req.version(), req.keep_alive(), content_type) };
				std::chrono::system_clock::time_point request_end = std::chrono::system_clock::now();
				LogResponse(duration_cast<std::chrono::milliseconds>(request_end - request_start).count(), static_cast<int>(status), content_type);
				return response;
			};
		const auto file_response = [&req, request_start](http::status status, http::file_body::value_type& body, std::string_view content_type = ContentType::APPLICATION_JSON)
			{
				FileResponse response{ MakeResponse(status, body, req.version(), req.keep_alive(), content_type) };
				std::chrono::system_clock::time_point request_end = std::chrono::system_clock::now();
				LogResponse(duration_cast<std::chrono::milliseconds>(request_end - request_start).count(), static_cast<int>(status), content_type);
				return response;
			};
		std::string_view req_type = req.method_string();
		std::string_view target = req.target();
		size_t size = target.size();
		//Checking if current request is an API request, then handling it
		if (size >= 4)
		{
			if (std::string_view(target.begin(), target.begin() + 5) == "/api/"sv || std::string_view(target.begin(), target.begin() + 4) == "/api"sv)
			{
				HandleRequestAPI(send, game, target, text_response, req);
				return;
			}
		}
		if (req_type != "GET"sv && req_type != "HEAD"sv)
		{
			send(text_response(http::status::method_not_allowed, { "Invalid method" }, ContentType::APPLICATION_JSON));
			return;
		}
		std::string_view target_but_without_the_slash{ target.begin() + 1, target.end() };
		fs::path requested_path = static_path / target_but_without_the_slash;
		//Security checks
		if (!IsSubPath(requested_path, static_path))
		{
			send(text_response(http::status::bad_request, { "Masterhack Denied. Root remains secure and lives to be breached another day.." }, ContentType::TEXT_TXT));
			return;
		}
		//Handle file if it's within bounds and is safe (totally)
		HandleRequestFile(send, game, requested_path, text_response, file_response);
		return; //In case I ever decide to add something to the code and forget to add return;
	}
	class RequestHandler
	{
	public:
		explicit RequestHandler(const fs::path& static_path, model::Game& game)
			: static_path_{ static_path },
			game_{ game }
		{}
		RequestHandler(const RequestHandler&) = delete;
		RequestHandler& operator=(const RequestHandler&) = delete;
		template <typename Body, typename Allocator, typename Send>
		void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send)
		{
			// Обработать запрос request и отправить ответ, используя send
			HandleRequest(req, game_, static_path_, send);
		}
	private:
		const fs::path static_path_;
		model::Game& game_;
	};
}  // namespace http_handler