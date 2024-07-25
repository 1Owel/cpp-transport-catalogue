#include "transport_catalogue.h"

double TransportCatalogue::GetRouteDistance(const Bus* bus_p) {
    if (bus_p == nullptr) {return 0.0;}
    double result = 0.0;
    for( size_t i = 1; i <  bus_p->route.size(); ++i) {
        result += ComputeDistance(bus_p->route[i-1]->coords, bus_p->route[i]->coords);
    }
    return result;
}

size_t TransportCatalogue::UniqueStops(const Bus* bus_pointer) {
    std::unordered_set<Stop*> unique_s; 
    for (const auto& i : bus_pointer->route) {
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
        for (const auto& stop_name : stops) {
            buses_.back().route.push_back(name_to_stop_[stop_name]);
            buses_on_stop_[name_to_stop_.at(stop_name)->name].emplace(buses_.back().name); // Добавление str_view имени автобуса который заезжает на остановку
        }
        name_to_bus_.emplace(buses_.back().name, &buses_.back());
    }

	RouteInfo TransportCatalogue::GetRouteInfo(const std::string_view bus_name) {
        const Bus* bus_pointer = HasBus(bus_name);
        return {bus_name, GetRouteSize(bus_pointer), UniqueStops(bus_pointer), GetRouteDistance(bus_pointer)};
    }

	RouteInfo TransportCatalogue::GetRouteInfo(const Bus* busp) {
        return {busp->name, GetRouteSize(busp), UniqueStops(busp), GetRouteDistance(busp)};
    }