#include "transport_catalogue.h"

double TransportCatalogue::GetDistance(const std::pair<Stop*, Stop*>& stop_pair) {
    if (distance_.find(stop_pair) == distance_.end()) {
        distance_[stop_pair] = ComputeDistance(stop_pair.first->coords, stop_pair.second->coords);
    }
    return distance_.at(stop_pair);
}

double TransportCatalogue::GetRouteDistance(const std::string_view bus_name) {
    double result = 0;
    auto& lhs = name_to_bus_.at(bus_name)->route.front();
    bool first = true;
    for (const auto& rhs : name_to_bus_.at(bus_name)->route) {
        if (first) {
            first = false;
            continue; 
            }
        result += GetDistance({lhs, rhs});
        lhs = rhs;
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

void TransportCatalogue::AddStop(std::string_view name, Coordinates coord) {
    stops_.push_back({move(static_cast<std::string>(name)), coord}); // Создание остановки
    name_to_stop_.emplace(stops_.back().name, &stops_.back()); // Дублирование указателями в unordered_map для быстрого доступа по имени
}

	void TransportCatalogue::AddBus(std::string_view name, std::vector<std::string_view> stops) {
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