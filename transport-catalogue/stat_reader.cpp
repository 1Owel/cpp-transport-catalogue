#include "stat_reader.h"

namespace detail {
    std::pair<std::string_view, std::string_view> SplitWithoutSpaces(const std::string_view& str) {
        auto spos = str.find_first_not_of(' ');
        auto pos = str.find(' ', spos);
        std::string_view lhs = str.substr(spos, pos);
        spos = str.find_first_not_of(' ', pos);
        pos = str.find_last_not_of(' ');
        std::string_view rhs = str.substr(spos, pos - spos + 1);
        return {lhs, rhs};
    }

    void PrintBus(TransportCatalogue& transport_catalogue, const std::string_view bus, std::ostream& output) {
        using namespace std::literals;
        const auto bus_pointer = transport_catalogue.HasBus(bus);
        if (bus_pointer == nullptr) {
            output << "Bus "s << bus << ": not found\n"s;
        } else {
            const auto info = transport_catalogue.GetRouteInfo(bus);
            output << "Bus "s << info.name << ": "s 
            << info.all_stops
            << " stops on route, "s 
            << info.unique_stops 
            << " unique stops, "s << info.r_distance << " route length, " << info.curvature << " curvature\n"s;
        }
    }

    void PrintStop(TransportCatalogue& transport_catalogue, const std::string_view stop, std::ostream& output) {
        using namespace std::literals;
        if (transport_catalogue.HasStop(stop) == nullptr) {
            output << "Stop "s << stop << ": not found\n"s;
        } else if (transport_catalogue.GetBusesOnStop(stop).empty()) {
            output << "Stop "s << stop << ": no buses\n"s;
        } else {
            output << "Stop "s << stop << ": buses "s;
            for (auto& i : transport_catalogue.GetBusesOnStop(stop)) {
                output << i << ' ';
            }
            output << "\n";
        }
    }
}

void ParseAndPrintStat(TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output) {
    auto splited = detail::SplitWithoutSpaces(request);

    if (splited.first == "Bus") {
        detail::PrintBus(transport_catalogue, splited.second, output);
    } else if (splited.first == "Stop") {
        detail::PrintStop(transport_catalogue, splited.second, output);
    }
}