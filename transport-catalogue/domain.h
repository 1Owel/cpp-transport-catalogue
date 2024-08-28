#pragma once

#include "geo.h"
#include <string>
#include <vector>

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

/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области (domain)
 * вашего приложения и не зависят от транспортного справочника. Например Автобусные маршруты и Остановки. 
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */