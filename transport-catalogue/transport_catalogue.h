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
	unsigned int r_distance;
	double curvature;
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

	size_t GetRouteSize(const Bus* bus_pointer) {
		return bus_pointer->route.size();
		}

	size_t UniqueStops(const Bus* bus_pointer);

	double GetRouteDistance(const Bus* bus_p);

	unsigned int GetRouteDistanceM(const Bus* bus_p);

	std::deque<Stop> stops_; // Все имеющиеся остановки
	std::deque<Bus> buses_; // Автобусы и их маршруты
	std::unordered_map<std::string_view, Stop*> name_to_stop_; // Контейнер для быстрого доступа к остановки по имени
	std::unordered_map<std::string_view, Bus*> name_to_bus_;
	std::unordered_map<std::string_view, std::set<std::string_view>> buses_on_stop_;
	std::unordered_map<std::pair<Stop*, Stop*>, unsigned int, DistanceHasher, ComparePairStops> distance_;
	
	public:

	Bus* HasBus(const std::string_view bus_name) {
		const auto bus = name_to_bus_.find(bus_name);
		if (bus == name_to_bus_.end()) {return nullptr;}
		return bus->second;
		}

	Stop* HasStop(const std::string_view stop_name) {
		const auto stop = name_to_stop_.find(stop_name);
		if (stop == name_to_stop_.end()) {return nullptr;}
		return stop->second;
		}

	const std::set<std::string_view>& GetBusesOnStop(const std::string_view name) {
		const auto check = buses_on_stop_.find(name);
		if (check == buses_on_stop_.end()) {
			static std::set<std::string_view> def = {};
			return def;
		}
		return check->second;
		}

	void AddStop(std::string_view name, const Coordinates& coord);

	void AddBus(std::string_view name, const std::vector<std::string_view>& stops);

	void AddDistance(std::string_view stop1, std::unordered_map<std::string_view, unsigned int> distances);

	RouteInfo GetRouteInfo(const std::string_view bus_name);
	// Попробовал сделать методы для GetRouteInfo через указатели, чтобы каждый метод используемый
	// внутри GetRouteInfo не искал объект по имени, для этого две функции, с указателем сразу
	// и еще одна где используеться HasBus. Получаеться что поиск по имени должен быть один за одно
	// использование функции.
	RouteInfo GetRouteInfo(const Bus* busp);


	};