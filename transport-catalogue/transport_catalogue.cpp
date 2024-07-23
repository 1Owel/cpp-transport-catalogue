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