#include "json_reader.h"

//Передавать только node хранящую base_requests
void AddStopWithoutDist([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& request) {
    const auto& mreq = request.AsMap();
    if (mreq.at("type").AsString() == "Stop") {
        catalogue.AddStop(mreq.at("name").AsString(),
        {mreq.at("latitude").AsDouble(), mreq.at("longitude").AsDouble()});
    }
}

void AddDistToStop([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& request) {
    const auto& mreq = request.AsMap();
    if (mreq.at("type").AsString() == "Stop") {
        if (mreq.at("road_distances").AsMap().empty()) {
            return;
        }
        std::unordered_map<std::string_view, unsigned int> distances;
        for (const auto& i : mreq.at("road_distances").AsMap()) {
            distances.emplace(i.first, static_cast<unsigned int>(i.second.AsInt()));
        }
        catalogue.AddDistance(mreq.at("name").AsString(), std::move(distances));
    }
}


/*
 * Для кольцевого маршрута (A>B>C>A) = [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) = [A,B,C,D,C,B,A]
 */
void AddBusFromNode([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& request) {
    const auto& mreq = request.AsMap();
    if (mreq.at("type").AsString() == "Bus") {
        std::vector<std::string_view> stops;
        const auto& arr = mreq.at("stops").AsArray();
        for (const auto& i : arr) {
            stops.push_back(i.AsString());
        }
        if (!mreq.at("is_roundtrip").AsBool()) { // Если не кольцевой
            auto rit = arr.rbegin();
            ++rit; // Пропуск конечной остановки
            while (rit != arr.rend())
            {
                stops.push_back(rit->AsString());
                ++rit;
            }
        }
        catalogue.AddBus(mreq.at("name").AsString(), stops);
    }
}

void GetJSONAnswer([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& node, std::ostream& out){
    using Dict = std::map<std::string, json::Node>;
    using Array = std::vector<json::Node>;
    using namespace std::literals;
    Array result;
    for (const auto& request : (node.AsMap().at("stat_requests")).AsArray()) {
        const auto& querry = request.AsMap();
        if (querry.at("type").AsString() == "Stop") {
            Dict answr;
            Array buses;

            if (!catalogue.HasStop(querry.at("name").AsString())) {
                answr["error_message"] = "not found"s;
            } else {
                auto buses_set = catalogue.GetBusesOnStop(querry.at("name").AsString());
                for (std::string_view i : buses_set) {
                    buses.push_back(static_cast<std::string>(i.data()));
                }
                answr["buses"] = buses;
            }
            answr["request_id"] = json::Node(querry.at("id"));
            result.push_back(answr);
        } else if (querry.at("type").AsString() == "Bus") {
            auto routeinf = catalogue.GetRouteInfo(querry.at("name").AsString());
            Dict answr;
            if (routeinf.name.empty()) {
                answr["error_message"] = "not found"s;
            } else {
                answr["curvature"] = routeinf.curvature;
                answr["route_length"] = static_cast<int>(routeinf.r_distance);
                answr["stop_count"] = static_cast<int>(routeinf.all_stops);
                answr["unique_stop_count"] = static_cast<int>(routeinf.unique_stops);
            }
            answr["request_id"] = querry.at("id");
            result.push_back(answr);
        }
    }
    json::Print(json::Document(json::Node(result)), out);
}

void ApplyCommandsFromVariant([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& node, std::ostream& output) {
    // Добавление остановок без дистанции до других остановок
    for (const auto& request : (node.AsMap().at("base_requests")).AsArray()) {
        AddStopWithoutDist(catalogue, request);
    }
    // повторный цикл для добавления дистанций до других остановок
    for (const auto& request : (node.AsMap().at("base_requests")).AsArray()) {
        AddDistToStop(catalogue, request);
    }
    // Цикл для добавления автобусов
    for (const auto& request : (node.AsMap().at("base_requests")).AsArray()) {
        AddBusFromNode(catalogue, request);
    }
    // Читает запрос к каталогу и выводит ответ в output
    GetJSONAnswer(catalogue, node, output);
}