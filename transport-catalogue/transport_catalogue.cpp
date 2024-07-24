#include "transport_catalogue.h"

double TransportCatalogue::GetRouteDistance(const std::string_view bus_name) {
    if (name_to_bus_.find(bus_name) == name_to_bus_.end()) {return 0.0;}
    const auto& route = name_to_bus_.at(bus_name)->route;
    double result = 0.0;
    for( size_t i = 1; i <  route.size(); ++i) {
        result += ComputeDistance(route[i-1]->coords, route[i]->coords);
    }
    return result;
}

size_t TransportCatalogue::UniqueStops(const std::string_view name) {
    std::unordered_set<Stop*> unique_s;
    for (const auto& i : name_to_bus_.at(name)->route) {
        unique_s.emplace(i);
    }
    return unique_s.size();
}

void TransportCatalogue::AddStop(std::string_view name, const Coordinates& coord) {
    stops_.push_back({move(static_cast<std::string>(name)), coord}); // Создание остановки
    name_to_stop_.emplace(stops_.back().name, &stops_.back()); // Дублирование указателями в unordered_map для быстрого доступа по имени
}

	void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string_view>& stops) {
        {
        std::vector<Stop*> def;
        buses_.push_back({move(static_cast<std::string>(name)), move(def)});
        }
        for (auto stop_name : stops) {
            buses_.back().route.push_back(name_to_stop_[stop_name]);
            buses_on_stop_[name_to_stop_.at(stop_name)->name].emplace(buses_.back().name); // Добавление str_view имени автобуса который заезжает на остановку
        }
        name_to_bus_.emplace(buses_.back().name, &buses_.back());
    }

	RouteInfo TransportCatalogue::GetRouteInfo(const std::string_view bus_name) {
        return {bus_name, GetRouteSize(bus_name), UniqueStops(bus_name), GetRouteDistance(bus_name)};
    }