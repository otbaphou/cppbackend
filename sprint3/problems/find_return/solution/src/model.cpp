#include "model.h"

#include <stdexcept>

int GetRandomNumber(int range)
{
	std::srand(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));

	return rand() % range;
}

std::string GenerateToken()
{

	const char hexChar[] =
	{
		'0', '1', '2', '3', '4',
		'5', '6', '7', '8', '9',
		'a','b','c','d','e','f'
	};

	std::string token;

	for (int i = 0; i < 32; i++)
	{
		token += hexChar[GetRandomNumber(16)];
	}

	return token;
}

namespace model
{
	using namespace std::literals;


	//===Game===
	void Game::AddMap(Map map, double dog_speed, int bag_capacity) 
	{
		if (dog_speed == -1)
		{
			map.SetDogSpeed(global_dog_speed_);
		}
		else
		{
			map.SetDogSpeed(dog_speed);
		}

		if (bag_capacity == -1)
		{
			map.SetBagCapacity(global_bag_capacity);
		}
		else
		{
			map.SetBagCapacity(bag_capacity);
		}

		const size_t index = maps_.size();
		if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
			throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
		}
		else 
		{
			try 
			{
				maps_.emplace_back(std::move(map));
			}
			catch (...) 
			{
				map_id_to_index_.erase(it);
				throw;
			}
		}
	}

	//===Map===	
	void Map::AddOffice(Office office) 
	{

	if (warehouse_id_to_index_.contains(office.GetId()))
	{
		throw std::invalid_argument("Duplicate warehouse");
	}

	const size_t index = offices_.size();
	Office& o = offices_.emplace_back(std::move(office));
	try {
		warehouse_id_to_index_.emplace(o.GetId(), index);
	}
	catch (...) {
		// Удаляем офис из вектора, если не удалось вставить в unordered_map
		offices_.pop_back();
		throw;
	}
}
	void Map::CalcRoads()
	{
		for (Road& road : roads_)
		{
			Point start = road.GetStart();
			Point end = road.GetEnd();

			int small, big;

			if (road.IsHorizontal())
			{
				int x1 = start.x, x2 = end.x;

				if (x1 > x2)
				{
					small = x2;
					big = x1;
				}
				else
				{
					big = x2;
					small = x1;
				}

				for (int i = small; i <= big; ++i)
				{
					point_to_roads_[{i, start.y}].push_back(std::make_shared<Road>(road));
				}

			}
			else
			{
				int y1 = start.y, y2 = end.y;

				if (y1 > y2)
				{
					small = y2;
					big = y1;
				}
				else
				{
					big = y2;
					small = y1;
				}

				for (int i = small; i <= big; ++i)
				{
					point_to_roads_[{start.x, i}].push_back(std::make_shared<Road>(road));
				}
			}
		}
	}

	//===Dog===
	Dog::Dog(Coordinates coords, const Map* map)
		:position_(coords)
		, speed_(map->GetDogSpeed())
		, current_map_(map)
	{
		const std::deque<std::shared_ptr<Road>>& roads = GetRoadsByPos(coords);
		current_road_ = roads.front().get();

		Point start = current_road_->GetStart();
		Point end = roads[0]->GetEnd();
	}

	void Dog::MoveVertical(double distance)
	{
		if (distance == 0)
			return;

		Point start = current_road_->GetStart();
		Point end = current_road_->GetEnd();

		double desired_point = position_.y + distance;

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
		double y2 = big_y + 0.4;

		if (desired_point < y1 || desired_point > y2)
		{
			for (std::shared_ptr<Road> road : GetRoadsByPos(position_))
			{
				if (road.get() != current_road_ && road.get()->IsVertical())
				{
					current_road_ = road.get();

					MoveVertical(distance);
					return;
				}
			}

			velocity_ = { 0,0 };
			if (desired_point < y1)
			{
				position_.y = y1;
			}
			else
			{
				position_.y = y2;
			}
			return;
		}

		position_.y = desired_point;
		return;
	}
	void Dog::MoveHorizontal(double distance)
	{
		if (distance == 0)
			return;

		Point start = current_road_->GetStart();
		Point end = current_road_->GetEnd();

		double desired_point = position_.x + distance;

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
		double x2 = big_x + 0.4;

		if (desired_point < x1 || desired_point > x2)
		{

			for (std::shared_ptr<Road> road : GetRoadsByPos(position_))
			{
				if (road.get() != current_road_ && road.get()->IsHorizontal())
				{
					current_road_ = road.get();

					MoveHorizontal(distance);
					return;
				}
			}

			velocity_ = { 0,0 };
			if (desired_point < x1)
			{
				position_.x = x1;
			}
			else
			{
				position_.x = x2;
			}
			return;
		}

		position_.x = desired_point;
		return;
	}

	void Dog::Move(double ms)
	{
		double distance = ms / 1000;

		bool is_vertical = velocity_.x == 0 && velocity_.y != 0;
		bool is_horizontal = velocity_.x != 0 && velocity_.y == 0;

		if (is_vertical)
		{
			MoveVertical(distance * velocity_.y);
		}

		if (is_horizontal)
		{
			MoveHorizontal(distance * velocity_.x);
		}
	}

	//Players

	std::string Players::MakePlayer(std::string username, const Map* map)
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

	Player* Players::FindPlayerByIdx(size_t idx)
	{
		if (idx < players_.size())
		{
			return &players_[idx];
		}

		return nullptr;
	}
	Player* Players::FindPlayerByToken(std::string token) const
	{
		std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c)
			{
				return std::tolower(c);
			});

		if (token_to_player_.contains(token))
		{
			return token_to_player_.at(token);
		}

		return nullptr;
	}

	Dog* Players::BirthDog(const Map* map)
	{
		//Finding the suitable road for the puppy to be born
		const std::vector<Road>& roads = map->GetRoads();

		//Now we find the exact spot on that road
		Coordinates spot = map->GetRandomSpot();

		//Temporary setting dog spot to the start of the first road
		if (!randomize_)
		{
			Point p{ roads.at(0).GetStart() };
			spot.x = p.x;
			spot.y = p.y;
		}
		Dog pup{ spot, map };
		dogs_.push_back(std::move(pup));
		return &dogs_.back();
	}

	Dog* Players::FindDogByIdx(size_t idx)
	{
		if (idx >= 0 || idx < dogs_.size())
		{
			return &dogs_[idx];
		}

		return nullptr;
	}

}  // namespace model
