#pragma once

#include <filesystem>
#include <string>
#include <boost/json.hpp>

#include "model.h"

namespace json_loader 
{
	namespace json = boost::json;

	void ParseRoads(model::Map& map, json::object& parent_object);
	void ParseBuildings(model::Map& map, json::object& parent_object);
	void ParseOffices(model::Map& map, json::object& parent_object);

	model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
