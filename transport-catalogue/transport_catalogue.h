#pragma once

#include "geo.h"

#include <deque>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <set>


struct Stop
{
	std::string name;
	Coordinates coords = {0, 0};
	std::set<std::string_view> buses_on_stop;

	bool operator==(const Stop& rhs) {
		return name == rhs.name;
	}
};

struct ComparePairStops {
 bool operator()(const std::pair<Stop*, Stop*>& lhs, const std::pair<Stop*, Stop*>& rhs) const {
	return (lhs.first == rhs.first) && (lhs.second == rhs.second);
 }
};

struct Bus
{
	std::string name; // Название автобуса
	std::vector<Stop*> route; // Указатели на остановки (Маршрут для автобуса)
};

class TransportCatalogue {
	private:
	struct DistanceHasher {
		size_t operator() (const std::pair<Stop*, Stop*> stop_pair) const {
			size_t zeroes = 1;
			for (size_t i = 0; i < stop_pair.first->name.size(); i++)
			{
				zeroes = zeroes * 100;
			}

			return s_hasher(stop_pair.first->name) + (s_hasher(stop_pair.second->name) + zeroes);
		}

		std::hash<std::string> s_hasher;
	};
	public:
	std::deque<Stop> Stops_; // Все имеющиеся остановки
	std::deque<Bus> Buses_; // Автобусы и их маршруты
	std::unordered_map<std::string_view, Stop*> name_to_stop_; // Контейнер для быстрого доступа к остановки по имени
	std::unordered_map<std::string_view, Bus*> name_to_bus_;
	std::unordered_map<std::pair<Stop*, Stop*>, double, DistanceHasher, ComparePairStops> distance_;

	double GetDistance(const std::pair<Stop*, Stop*>& stop_pair);

	double GetRouteDistance(const std::string_view bus_name);
	};