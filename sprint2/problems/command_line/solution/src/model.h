#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <iostream>
#include <chrono>
#include <memory>
#include <algorithm>

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

int GetRandomNumber(int range);

std::string GenerateToken();

namespace model
{

	using Dimension = int;
	using Coord = Dimension;

	struct Point
	{
		Coord x, y;

		bool operator==(const Point& other) const
		{
			return (x == other.x && y == other.y);
		}
	};

	struct PointHasher
	{
		std::size_t operator()(const Point& p) const
		{
			using std::size_t;

			return p.x + p.y * p.y;
		}
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
			, name_(std::move(name))
		{}

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

		void SetDogSpeed(double speed)
		{
			dog_speed_ = speed;
		}

		double GetDogSpeed() const
		{
			return dog_speed_;
		}

		void CalcRoads();

		const std::deque<std::shared_ptr<Road>>& GetRoadsAtPoint(Point p) const
		{
			return point_to_roads_.at(p);
		}

		void AddOffice(Office office);

	private:
		using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

		Id id_;
		std::string name_;
		Roads roads_;
		Buildings buildings_;

		std::unordered_map<Point, std::deque<std::shared_ptr<Road>>, PointHasher> point_to_roads_;

		double dog_speed_ = 1;

		OfficeIdToIndex warehouse_id_to_index_;
		Offices offices_;
	};

	enum Direction
	{
		NORTH = 'U',
		SOUTH = 'D',
		WEST = 'L',
		EAST = 'R'
	};

	struct Coordinates
	{
		double x;
		double y;
	};

	struct Velocity
	{
		double x;
		double y;
	};

	class Dog
	{
	public:

		Dog(Coordinates coords, const Map* map);

		void SetVel(double vel_x, double vel_y)
		{
			velocity_.x = speed_ * vel_x;
			velocity_.y = speed_ * vel_y;
		}

		void SetDir(Direction dir)
		{
			direction_ = dir;
		}

		Coordinates GetPos() const
		{
			return position_;
		}

		Velocity GetVel() const
		{
			return velocity_;
		}

		char GetDir() const
		{
			return direction_;
		}

		const std::deque<std::shared_ptr<Road>>& GetRoadsByPos(Coordinates pos) const
		{
			int pos_x = static_cast<int>(pos.x + 0.5);
			int pos_y = static_cast<int>(pos.y + 0.5);

			return current_map_->GetRoadsAtPoint({ pos_x, pos_y });
		}

		void MoveVertical(double distance);

		void MoveHorizontal(double distance);

		void Move(double ms);

	private:

		Coordinates position_;

		Road* current_road_;

		Velocity velocity_{ 0, 0 };
		double speed_;
		Direction direction_ = Direction::NORTH;

		bool is_moving = false;

		const Map* current_map_;
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

		Coordinates GetPos()
		{
			if (pet_ == nullptr)
				return { -1, -1 };

			return pet_->GetPos();
		}

		Velocity GetVel()
		{
			if (pet_ == nullptr)
				return { -1, -1 };

			return pet_->GetVel();
		}

		char GetDir()
		{
			if (pet_ == nullptr)
				return '!';

			return pet_->GetDir();
		}

		void SetVel(double vel_x, double vel_y)
		{
			pet_->SetVel(vel_x, vel_y);
		}

		void SetDir(Direction dir)
		{
			pet_->SetDir(dir);
		}

		void Move(int ms)
		{
			pet_->Move(ms);
		}

	private:

		std::string username_;
		const Map* current_map_;

		const size_t id_;

		Dog* pet_;
	};

	class Players
	{
	public:
		explicit Players(bool randomize)
			:randomize_(randomize) {}

		//This function creates a player and returns a token
		std::string MakePlayer(std::string username, const Map* map);

		Player* FindPlayerByIdx(size_t idx);
		Player* FindPlayerByToken(std::string token) const;

		Dog* BirthDog(const Map* map);

		Dog* FindDogByIdx(size_t idx);

		const std::deque<Player*> GetPlayerList(std::string map_id) const
		{
			return map_id_to_players_.at(map_id);
		}

		void MoveAll(double ms)
		{
			for (Dog& dog : dogs_)
			{
				dog.Move(ms);
			}
		}

	private:

		bool randomize_;

		std::unordered_map<std::string, Player*> token_to_player_;
		std::unordered_map<std::string, std::deque<Player*>> map_id_to_players_;
		std::deque<Player> players_;
		std::deque<Dog> dogs_;
	};

	class Game
	{
	public:

		Game(Players& pm)
			:player_manager_(pm) {}

		using Maps = std::vector<Map>;

		void AddMap(Map map, double dog_speed = -1);

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

		void SetGlobalDogSpeed(double speed)
		{
			global_dog_speed_ = speed;
		}

		void ServerTick(int milliseconds)
		{
			player_manager_.MoveAll(milliseconds);
		}

	private:

		double global_dog_speed_ = 1;

		using MapIdHasher = util::TaggedHasher<Map::Id>;
		using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

		std::vector<Map> maps_;
		std::deque<GameSession> sessions_;
		MapIdToIndex map_id_to_index_;

		Players& player_manager_;
	};

}  // namespace model
