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

    void PrintBus(TransportCatalogue& transport_catalogue, const std::string_view bus, std::ostream& output) {
        using namespace std::literals;
        if (transport_catalogue.HasBus(bus)) {
            output << "Bus "s << bus << ": not found\n"s;
        } else {
        output << "Bus "s << bus << ": "s 
        << transport_catalogue.GetRouteSize(bus)
        << " stops on route, "s 
        << transport_catalogue.UniqueStops(bus) 
        << " unique stops, "s << transport_catalogue.GetRouteDistance(bus) << " route length\n"s;
        }
    }

    void PrintStop(TransportCatalogue& transport_catalogue, const std::string_view stop, std::ostream& output) {
        using namespace std::literals;
        if (transport_catalogue.HasStop(stop)) {
            output << "Stop "s << stop << ": not found\n"s;
        } else if (transport_catalogue.IsStopEmpty(stop)) {
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