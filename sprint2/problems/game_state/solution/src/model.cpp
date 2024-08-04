#include "model.h"

#include <stdexcept>

int GetRandomNumber(int range)
{
	std::srand(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));

	return rand() % range;
}

std::string GenerateToken()
{

	char hexChar[] =
	{
		'0', '1', '2', '3', '4',
		'5', '6', '7', '8', '9',
		'A','B','C','D','E','F'
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

	void Map::AddOffice(Office office) {
		if (warehouse_id_to_index_.contains(office.GetId())) {
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

	void Game::AddMap(Map map) {
		const size_t index = maps_.size();
		if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
			throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
		}
		else {
			try {
				maps_.emplace_back(std::move(map));
			}
			catch (...) {
				map_id_to_index_.erase(it);
				throw;
			}
		}
	}

}  // namespace model
