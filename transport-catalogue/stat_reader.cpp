#include "stat_reader.h"

namespace detail {
    std::pair<std::string_view, std::string_view> SplitWithoutSpaces(const std::string_view& str) {
        auto spos = str.find_first_not_of(' ');
        auto pos = str.find(' ', spos);
        std::string_view lhs = str.substr(spos, pos);
        spos = str.find_first_not_of(' ', pos);
        pos = str.find_last_not_of(' ');
        std::string_view rhs = str.substr(spos, pos);
        return {lhs, rhs};
    }

    size_t UniqueStops(std::vector<Stop*> stops) {
        std::unordered_set<Stop*> unique_s;
        for (const auto& i : stops) {
            unique_s.emplace(i);
        }
        return unique_s.size();
    }
}

void ParseAndPrintStat(TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output) {
    using namespace std::literals;
    auto splited = detail::SplitWithoutSpaces(request);
    
    if (splited.first == "Bus"s) {
        if (transport_catalogue.name_to_bus_.find(splited.second) == transport_catalogue.name_to_bus_.end()) {
            output << "Bus "s << splited.second << ": not found\n"s;
        } else {
        output << splited.first << " "s << splited.second << ": "s 
        << transport_catalogue.name_to_bus_.at(splited.second)->route.size() 
        << " stops on route, "s 
        << detail::UniqueStops(transport_catalogue.name_to_bus_.at(splited.second)->route) 
        << " unique stops, "s << transport_catalogue.GetRouteDistance(splited.second) << " route length\n"s;
        }
    } else if (splited.first == "Stop"s) {
        if (transport_catalogue.name_to_stop_.find(splited.second) == transport_catalogue.name_to_stop_.end()) {
            output << "Stop "s << splited.second << ": not found\n"s;
        } else if (transport_catalogue.name_to_stop_.at(splited.second)->buses_on_stop.empty()) {
            output << "Stop "s << splited.second << ": no buses\n"s;
        } else {
            output << "Stop "s << splited.second << ": buses "s;
            for (auto& i : transport_catalogue.name_to_stop_.at(splited.second)->buses_on_stop) {
                output << i << ' ';
            }
            output << "\n";
        }
    }
}