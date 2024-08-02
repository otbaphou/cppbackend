#include "request_handler.h"

namespace http_handler 
{
	StringResponse MakeStringResponse(http::status status, std::string_view body, 
	unsigned http_version, bool keep_alive, std::string_view content_type)
	{
		StringResponse response(status, http_version);
		response.set(http::field::content_type, content_type);
		response.body() = body;
		response.content_length(body.size());
		response.keep_alive(keep_alive);
		return response;
	}

	
	void PackMaps(json::array& target_container, model::Game& game)
	{
		for (auto& map : game.GetMaps())
		{
			json::object elem;

			elem.emplace("id", *map.GetId());
			elem.emplace("name", map.GetName());

			target_container.push_back(std::move(elem));
		}
	}


	void PackRoads(json::array& target_container, const model::Map* map_ptr)
	{
		for (const model::Road& road_data : map_ptr->GetRoads())
		{
			//Temporary road object
			json::object road;

			model::Point start = road_data.GetStart();
			model::Point end = road_data.GetEnd();

			road.emplace(X0_COORDINATE, start.x);
			road.emplace(Y0_COORDINATE, start.y);

			if (road_data.IsHorizontal())
			{
				road.emplace(X1_COORDINATE, end.x);
			}
			else
			{
				road.emplace(Y1_COORDINATE, end.y);
			}

			//Pushing road to the road container
			target_container.push_back(std::move(road));
		}
	}

	void PackBuildings(json::array& target_container, const model::Map* map_ptr)
	{
		for (const model::Building& building_data : map_ptr->GetBuildings())
		{
			//Temporary building object
			json::object building;

			model::Rectangle dimensions = building_data.GetBounds();

			building.emplace(X_COORDINATE, dimensions.position.x);
			building.emplace(Y_COORDINATE, dimensions.position.y);
			building.emplace(WIDTH, dimensions.size.width);
			building.emplace(HEIGHT, dimensions.size.height);

			//Pushing building to the building container
			target_container.push_back(std::move(building));
		}
	}

	void PackOffices(json::array& target_container, const model::Map* map_ptr)
	{
		for (const model::Office& office_data : map_ptr->GetOffices())
		{
			//Temporary office object
			json::object office;

			office.emplace("id", *office_data.GetId());

			model::Point pos = office_data.GetPosition();
			office.emplace(X_COORDINATE, pos.x);
			office.emplace(Y_COORDINATE, pos.y);

			model::Offset offset = office_data.GetOffset();
			office.emplace(X_OFFSET, offset.dx);
			office.emplace(Y_OFFSET, offset.dy);

			//Pushing office to the building container
			target_container.push_back(std::move(office));
		}
	}

}  // namespace http_handler
