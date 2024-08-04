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

		void CalcRoads()
		{
			for (Road& road : roads_)
			{
				point_to_roads_[road.GetStart()].push_back(&road);
				point_to_roads_[road.GetEnd()].push_back(&road);
			}
		}

		std::deque<Road*> GetRoadsAtPoint(Point p) const
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

		std::unordered_map<Point, std::deque<Road*>, PointHasher> point_to_roads_;

		double dog_speed_;

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
		Dog(Coordinates coords, const Map* map)
		:position_(coords)
		, current_map_(map)
		, speed_(map->GetDogSpeed())
		{
			current_road_ = map->GetRoadsAtPoint(map->GetRoads().at(0).GetStart())[0];//Ew
		}

		void SetVel(double vel_x, double vel_y)
		{
			velocity_.x = speed_ * vel_x;
			velocity_.y = speed_ * vel_y;
		}

		void SetDir(Direction dir)
		{
			direction_ = dir;
		}

		Coordinates GetPos()
		{
			return position_;
		}

		Velocity GetVel()
		{
			return velocity_;
		}

		char GetDir()
		{
			return direction_;
		}

		std::deque<Road*> GetRoadsByPos(Coordinates pos)
		{
			int pos_x = static_cast<int>(pos.x + 0.5);
			int pos_y = static_cast<int>(pos.y + 0.5);

			return current_map_->GetRoadsAtPoint({ pos_x, pos_y });
		}

		//Make two methods:
		//MoveVertically(int dst);
		//MoveHorizontally(int dst);
		//
		//First we check the orientation of the current road
		// 
		//If it's parallel to our direction
		//Check if the distance between the player and edge of the road is bigger or equal to the desired path's distance
		//If former is less - set the player position to the edge and check for the adjacent roads with the same orientation
		//If there are none - set the velocity to zero
		//But if there are - change the current_road of the player and continue moving until you get to the edge or complete the distance
		//
		//If it's perpendicular
		//Set the player's position to the edge of the road
		//Check for adjacent roads with the orientation the same as ours
		//If there's none - congratulations, you got lucky
		//If there are - change the current_road of the player and continue moving until you get to the edge or complete the distance
		//
		//I guess I see the pattern now..
		//The downside is this script doesn't take account of overalapping roads
		//The solution would be to parse every single step of the road
		//
		//Interesting notice:
		//
		//You probably wouldn't step on the intersecting roads if you're going parallel to the road
		//

		void MoveVertical(double distance)
		{
			double dist_left;

			Point start = current_road_->GetStart();
			Point end = current_road_->GetEnd();

			double desired_point = position_.y + distance;

			if (current_road_->IsVertical())
			{
				double small_y, big_y;

				if (start.y < end.y)
				{
					small_y = start.y;
					big_y = end.y;
				}
				else
				{
					small_y = end.y;
					big_y = start.y;
				}

				double y1 = small_y - 0.4;
				if (desired_point < y1)
				{
					dist_left = desired_point - y1;
					position_.y = y1;

					bool found = false;

					for (Road* road : GetRoadsByPos({ position_.x, position_.y }))
					{
						if (road->IsVertical() && road != current_road_)
						{
							current_road_ = road;
							found = true;
						}
					}

					if (!found)
					{
						velocity_.y = 0;
					}
					else
					{
						MoveVertical(dist_left);
					}

					return;
				}

				double y2 = small_y + 0.4;
				if (desired_point > y2)
				{
					dist_left = desired_point - y2;
					position_.y = y2;

					bool found = false;

					for (Road* road : GetRoadsByPos({ position_.x, position_.y }))
					{
						if (road->IsVertical() && road != current_road_)
						{
							current_road_ = road;
							found = true;
						}
					}

					if (!found)
					{
						velocity_.y = 0;
					}
					else
					{
						MoveVertical(dist_left);
					}

					return;
				}
			}
			else
			{
				double y1 = start.y - 0.4;
				if (desired_point < y1)
				{
					dist_left = desired_point - y1;
					position_.y = y1;

					bool found = false;

					for (Road* road : GetRoadsByPos({ position_.x, position_.y }))
					{
						if (road->IsVertical())
						{
							current_road_ = road;
							found = true;
						}
					}

					if (!found)
					{
						velocity_.y = 0;
					}
					else
					{
						MoveVertical(dist_left);
					}

					return;
				}

				double y2 = start.y + 0.4;
				if (desired_point > y2)
				{
					dist_left = desired_point - y2;
					position_.y = y2;

					bool found = false;

					for (Road* road : GetRoadsByPos({ position_.x, position_.y }))
					{
						if (road->IsVertical())
						{
							current_road_ = road;
							found = true;
						}
					}

					if (!found)
					{
						velocity_.y = 0;
					}
					else
					{
						MoveVertical(dist_left);
					}

					return;
				}
			}

			dist_left = 0;
			position_.y = desired_point;
		}

		void MoveHorizontal(double distance)
		{
			double dist_left;

			Point start = current_road_->GetStart();
			Point end = current_road_->GetEnd();

			double desired_point = position_.x + distance;

			if (current_road_->IsHorizontal())
			{
				double small_x, big_x;

				if (start.x < end.x)
				{
					small_x = start.x;
					big_x = end.x;
				}
				else
				{
					small_x = end.x;
					big_x = start.x;
				}

				double x1 = small_x - 0.4;
				if (desired_point < x1)
				{
					dist_left = desired_point - x1;
					position_.x = x1;

					bool found = false;

					for (Road* road : GetRoadsByPos({ position_.x, position_.y }))
					{
						if (road->IsHorizontal() && road != current_road_)
						{
							current_road_ = road;
							found = true;
						}
					}

					if (!found)
					{
						velocity_.x = 0;
					}
					else
					{
						MoveHorizontal(dist_left);
					}

					return;
				}

				double x2 = small_x + 0.4;
				if (desired_point > x2)
				{
					dist_left = desired_point - x2;
					position_.x = x2;

					bool found = false;

					for (Road* road : GetRoadsByPos({ position_.x, position_.y }))
					{
						if (road->IsHorizontal() && road != current_road_)
						{
							current_road_ = road;
							found = true;
						}
					}

					if (!found)
					{
						velocity_.x = 0;
					}
					else
					{
						MoveHorizontal(dist_left);
					}

					return;
				}
			}
			else
			{
				double x1 = start.x - 0.4;
				if (desired_point < x1)
				{
					dist_left = desired_point - x1;
					position_.x = x1;

					bool found = false;

					for (Road* road : GetRoadsByPos({ position_.x, position_.y }))
					{
						if (road->IsHorizontal())
						{
							current_road_ = road;
							found = true;
						}
					}

					if (!found)
					{
						velocity_.x = 0;
					}
					else
					{
						MoveHorizontal(dist_left);
					}

					return;
				}

				double x2 = start.x + 0.4;
				if (desired_point > x2)
				{
					dist_left = desired_point - x2;
					position_.x = x2;

					bool found = false;

					for (Road* road : GetRoadsByPos({ position_.x, position_.y }))
					{
						if (road->IsHorizontal())
						{
							current_road_ = road;
							found = true;
						}
					}

					if (!found)
					{
						velocity_.x = 0;
					}
					else
					{
						MoveHorizontal(dist_left);
					}

					return;
				}
			}

			dist_left = 0;
			position_.x = desired_point;
		}

		void Move(double ms)
		{
			double distance = ms / 1000;

			Coordinates new_pos;

			new_pos.x = position_.x + (velocity_.x * distance);
			new_pos.y = position_.y + (velocity_.y * distance);

			bool is_vertical = new_pos.x == position_.x;

			if (is_vertical)
			{
				MoveVertical(velocity_.y * distance);
			}
			else
			{
				MoveHorizontal(velocity_.x * distance);
			}
		}

	private:

		Coordinates position_;

		Road* current_road_;

		Velocity velocity_;
		double speed_;
		Direction direction_ = Direction::NORTH;

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
		GameSession* session_;
	};

	class Players
	{
	public:

		//This function creates a player and returns a token
		std::string MakePlayer(std::string username, const Map* map)
		{
			Player player{ BirthDog(map), players_.size(), username, map };
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
			for (char& c : token)
			{
				c = std::tolower(c);
			}

			if (token_to_player_.contains(token))
			{
				return token_to_player_.at(token);
			}

			return nullptr;
		}

		Dog* BirthDog(const Map* map)
		{
			//Finding the suitable road for the puppy to be born
			const std::vector<Road>& roads = map->GetRoads();
			const Road& road = roads.at(GetRandomNumber(roads.size()));

			//Now we find the exact spot on that road
			Coordinates spot;

			Point start = road.GetStart();
			Point end = road.GetEnd();

			if (road.IsVertical())
			{
				spot.x = start.x;
				
				int offset = GetRandomNumber(std::abs(start.y - end.y));

				if (start.y < end.y)
				{
					spot.y = start.y + offset;
				}
				else
				{
					spot.y = end.y + offset;
				}
			}
			else
			{
				spot.y = start.y;

				int offset = GetRandomNumber(std::abs(start.x - end.x));

				if (start.x < end.x)
				{
					spot.x = start.x + offset;
				}
				else
				{
					spot.x = end.x + offset;
				}
			}
			//Temporary setting dog spot to the start of the first road
			Point p{ roads.at(0).GetStart() };
			spot.x = p.x;
			spot.y = p.y;
			Dog pup{ spot, map };
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
			for(Map& map : maps_)
			{
				for (Player* player : player_manager_.GetPlayerList(*map.GetId()))
				{
					player->Move(milliseconds);
				}
			}
		}

	private:

		double global_dog_speed_ = 1;

		using MapIdHasher = util::TaggedHasher<Map::Id>;
		using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

		std::vector<Map> maps_;
		std::deque<GameSession> sessions_;
		MapIdToIndex map_id_to_index_;

		Players player_manager_;
	};

}  // namespace model
