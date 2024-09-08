#include "transport_catalogue.h"

unsigned int TransportCatalogue::GetRouteDistanceM(const Bus* bus_p) {
    if (bus_p == nullptr) {return 0;}
    unsigned int result = 0;
    for( size_t i = 1; i <  bus_p->route.size(); ++i) {
        auto dist = distance_.find({bus_p->route[i-1], bus_p->route[i]});
        if (dist != distance_.end()) {
            result += dist->second;
        } else {
            dist = distance_.find({bus_p->route[i], bus_p->route[i-1]});
            result += dist->second;
        }
    }
    return result;
}

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

	void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string_view>& stops, bool roundtrip) {
        {
        std::vector<Stop*> def;
        buses_.push_back({move(static_cast<std::string>(name)), move(def), roundtrip});
        }
        for (const auto& stop_name : stops) {
            buses_.back().route.push_back(name_to_stop_[stop_name]);
            buses_on_stop_[name_to_stop_.at(stop_name)->name].emplace(buses_.back().name); // Добавление str_view имени автобуса который заезжает на остановку
        }
        name_to_bus_.emplace(buses_.back().name, &buses_.back());
    }

    void TransportCatalogue::AddDistance(std::string_view stop1, std::unordered_map<std::string_view, unsigned int> distances) {
        Stop* stop_ptr = name_to_stop_.at(stop1);
        for (auto i : distances) {
            std::pair<Stop*, Stop*> key_stops = {stop_ptr, name_to_stop_.at(i.first)};
            distance_.emplace(move(key_stops), i.second);
        }
    }

	RouteInfo TransportCatalogue::GetRouteInfo(const std::string_view bus_name) {
        const Bus* bus_pointer = HasBus(bus_name);
        if (bus_pointer == nullptr) {return {};}
        const auto real_dist = GetRouteDistanceM(bus_pointer);
        return {bus_name, GetRouteSize(bus_pointer), UniqueStops(bus_pointer), real_dist, real_dist / GetRouteDistance(bus_pointer)};
    }

	RouteInfo TransportCatalogue::GetRouteInfo(const Bus* busp) {
        const auto real_dist = GetRouteDistanceM(busp);
        return {busp->name, GetRouteSize(busp), UniqueStops(busp), real_dist, real_dist / GetRouteDistance(busp)};
    }