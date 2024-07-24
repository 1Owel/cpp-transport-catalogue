#pragma once

#include "geo.h"

#include <deque>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>


struct Stop
{
	std::string name;
	Coordinates coords = {0, 0};

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

struct RouteInfo
{
	std::string_view name;
	size_t all_stops;
	size_t unique_stops;
	double r_distance;
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

	size_t GetRouteSize(const std::string_view bus_name) {
		if (name_to_bus_.find(bus_name) == name_to_bus_.end()) {return 0;}
		return name_to_bus_.at(bus_name)->route.size();
		}

	size_t UniqueStops(const std::string_view name);

	std::deque<Stop> stops_; // Все имеющиеся остановки
	std::deque<Bus> buses_; // Автобусы и их маршруты
	std::unordered_map<std::string_view, Stop*> name_to_stop_; // Контейнер для быстрого доступа к остановки по имени
	std::unordered_map<std::string_view, Bus*> name_to_bus_;
	std::unordered_map<std::string_view, std::set<std::string_view>> buses_on_stop_;
	
	public:

	double GetRouteDistance(const std::string_view bus_name);

	bool HasBus(const std::string_view bus_name) { return name_to_bus_.find(bus_name) == name_to_bus_.end();}

	Stop* HasStop(const std::string_view stop_name) {
		if (name_to_stop_.find(stop_name) == name_to_stop_.end()) {return nullptr;}
		return name_to_stop_[stop_name];
		}

	bool IsStopEmpty(const std::string_view name) {
		if (buses_on_stop_.find(name) == buses_on_stop_.end()) {return true;}
		return buses_on_stop_.at(name).empty();
		}

	const std::set<std::string_view>& GetBusesOnStop(const std::string_view name) {return buses_on_stop_.at(name);}

	void AddStop(std::string_view name, const Coordinates& coord);

	void AddBus(std::string_view name, const std::vector<std::string_view>& stops);

	RouteInfo GetRouteInfo(const std::string_view bus_name);

	};