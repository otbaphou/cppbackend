#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <chrono>

#include "tagged.h"

const std::string X_COORDINATE = "x";
const std::string Y_COORDINATE = "y";


const std::string X_OFFSET = "offsetX";
const std::string Y_OFFSET = "offsetY";

const std::string X0_COORDINATE = "x0";
const std::string Y0_COORDINATE = "y0";
const std::string X1_COORDINATE = "x1";
const std::string Y1_COORDINATE = "y1";

const std::string WIDTH = "w";
const std::string HEIGHT = "h";

std::string GenerateToken();

namespace model
{

	using Dimension = int;
	using Coord = Dimension;

	struct Point {
		Coord x, y;
	};

	struct Size {
		Dimension width, height;
	};

	struct Rectangle {
		Point position;
		Size size;
	};

	struct Offset {
		Dimension dx, dy;
	};

	class Road {
		struct HorizontalTag {
			explicit HorizontalTag() = default;
		};

		struct VerticalTag {
			explicit VerticalTag() = default;
		};

	public:
		constexpr static HorizontalTag HORIZONTAL{};
		constexpr static VerticalTag VERTICAL{};

		Road(HorizontalTag, Point start, Coord end_x) noexcept
			: start_{ start }
			, end_{ end_x, start.y } {
		}

		Road(VerticalTag, Point start, Coord end_y) noexcept
			: start_{ start }
			, end_{ start.x, end_y } {
		}

		bool IsHorizontal() const noexcept {
			return start_.y == end_.y;
		}

		bool IsVertical() const noexcept {
			return start_.x == end_.x;
		}

		Point GetStart() const noexcept {
			return start_;
		}

		Point GetEnd() const noexcept {
			return end_;
		}

	private:
		Point start_;
		Point end_;
	};

	class Building {
	public:
		explicit Building(Rectangle bounds) noexcept
			: bounds_{ bounds } {
		}

		const Rectangle& GetBounds() const noexcept {
			return bounds_;
		}

	private:
		Rectangle bounds_;
	};

	class Office {
	public:
		using Id = util::Tagged<std::string, Office>;

		Office(Id id, Point position, Offset offset) noexcept
			: id_{ std::move(id) }
			, position_{ position }
			, offset_{ offset } {
		}

		const Id& GetId() const noexcept {
			return id_;
		}

		Point GetPosition() const noexcept {
			return position_;
		}

		Offset GetOffset() const noexcept {
			return offset_;
		}

	private:
		Id id_;
		Point position_;
		Offset offset_;
	};

	class Map 
	{
	public:
		using Id = util::Tagged<std::string, Map>;
		using Roads = std::vector<Road>;
		using Buildings = std::vector<Building>;
		using Offices = std::vector<Office>;

		Map(Id id, std::string name) noexcept
			: id_(std::move(id))
			, name_(std::move(name)) {
		}

		const Id& GetId() const noexcept {
			return id_;
		}

		const std::string& GetName() const noexcept {
			return name_;
		}

		const Buildings& GetBuildings() const noexcept {
			return buildings_;
		}

		const Roads& GetRoads() const noexcept {
			return roads_;
		}

		const Offices& GetOffices() const noexcept {
			return offices_;
		}

		void AddRoad(const Road& road) {
			roads_.emplace_back(road);
		}

		void AddBuilding(const Building& building) {
			buildings_.emplace_back(building);
		}

		void AddOffice(Office office);

	private:
		using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

		Id id_;
		std::string name_;
		Roads roads_;
		Buildings buildings_;

		OfficeIdToIndex warehouse_id_to_index_;
		Offices offices_;
	};

	class Dog
	{

	};

	class GameSession
	{
	public:

		GameSession(Map* maptr)
			:current_map_(maptr)
		{}

	private:

		Map* current_map_;

	};

	class Player
	{
	public:
		Player(Dog* dog, size_t id, std::string username, const Map* maptr)
			:pet_(dog),
			id_(id),
			current_map_(maptr),
			username_(username)
		{}

		void SetSession(GameSession* sesh)
		{
			session_ = sesh;
		}		

		std::string GetName() const
		{
			return username_;
		}		

		const Map* GetCurrentMap()
		{
			return current_map_;
		}

		size_t GetId() const
		{
			return id_;
		}

	private:

		std::string username_;
		const Map* current_map_;

		const size_t id_;

		Dog* pet_;
		GameSession* session_;
	};

	class Players
	{
	public:

		//This function creates a player and returns a token
		std::string MakePlayer(std::string username, const Map* map)
		{
			Player player{ BirthDog(), players_.size(), username, map };
			players_.push_back(std::move(player));

			Player* ptr = &players_.back();

			std::string token{ std::move(GenerateToken()) };

			//Protection against impossible odds of two identical tokens generating. What a waste :p
			while (FindPlayerByToken(token) != nullptr)
			{
				//You should buy a lottery ticket!
				token = std::move(GenerateToken());
			}

			token_to_player_.emplace(token, ptr);
			map_id_to_players_[*map->GetId()].push_back(ptr);

			return token;
		}

		Player* FindPlayerByIdx(size_t idx)
		{
			if (idx >= 0 || idx < players_.size())
			{
				return &players_[idx];
			}

			return nullptr;
		}

		Player* FindPlayerByToken(std::string token) const
		{
			if (token_to_player_.contains(token))
			{
				return token_to_player_.at(token);
			}

			return nullptr;
		}

		Dog* BirthDog()
		{
			Dog pup;
			dogs_.push_back(std::move(pup));
			return &dogs_.back();
		}

		Dog* FindDogByIdx(size_t idx)
		{
			if (idx >= 0 || idx < dogs_.size())
			{
				return &dogs_[idx];
			}

			return nullptr;
		}

		const std::deque<Player*> GetPlayerList(std::string map_id) const
		{
			return map_id_to_players_.at(map_id);
		}

	private:

		std::unordered_map<std::string, Player*> token_to_player_;
		std::unordered_map<std::string, std::deque<Player*>> map_id_to_players_;
		std::deque<Player> players_;
		std::deque<Dog> dogs_;
	};

	class Game 
	{
	public:
		using Maps = std::vector<Map>;

		void AddMap(Map map);

		const Maps& GetMaps() const noexcept {
			return maps_;
		}

		const Map* FindMap(const Map::Id& id) const noexcept 
		{
			if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) 
			{
				return &maps_.at(it->second);
			}
			return nullptr;
		}

		std::string SpawnPlayer(std::string username, const Map* map)
		{
			return player_manager_.MakePlayer(username, map);
		}

		Player* FindPlayerByToken(std::string token) const
		{
			return player_manager_.FindPlayerByToken(token);
		}

		const std::deque<Player*> GetPlayerList(std::string map_id) const
		{
			return player_manager_.GetPlayerList(map_id);
		}

	private:
		using MapIdHasher = util::TaggedHasher<Map::Id>;
		using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

		std::vector<Map> maps_;
		std::deque<GameSession> sessions_;
		MapIdToIndex map_id_to_index_;

		Players player_manager_;
	};

}  // namespace model
