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

void AddBusFromNode([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& request) {
    const auto& mreq = request.AsMap();
    if (mreq.at("type").AsString() == "Bus") {
        std::vector<std::string_view> stops;
        for (const auto& i : mreq.at("stops").AsArray()) {
            stops.push_back(i.AsString());
        }
        catalogue.AddBus(mreq.at("name").AsString(), stops);
    }
}

void GetJSONAnswer([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& node, std::ostream& out){
    using Dict = std::map<std::string, json::Node>;
    using Array = std::vector<json::Node>;
    Array result;
    for (const auto& request : (node.AsMap().at("stat_requests")).AsArray()) {
        const auto& querry = request.AsMap();
        if (querry.at("type").AsString() == "Stop") {
            Dict answr;
            Array buses;
            auto buses_set = catalogue.GetBusesOnStop(querry.at("name").AsString());
            for (std::string_view i : buses_set) {
                buses.push_back(json::Node(static_cast<std::string>(i.data())));
            }
            answr["buses"] = buses;
            answr["request_id"] = json::Node(querry.at("id"));
            result.push_back(answr);
        } else if (querry.at("type").AsString() == "Bus") {
            auto routeinf = catalogue.GetRouteInfo(querry.at("name").AsString());
            Dict answr;
            answr["curvature"] = json::Node(routeinf.curvature);
            answr["request_id"] = json::Node(querry.at("id"));
            answr["route_length"] = json::Node(static_cast<int>(routeinf.r_distance));
            answr["stop_count"] = json::Node(static_cast<int>(routeinf.all_stops));
            answr["unique_stop_count"] = json::Node(static_cast<int>(routeinf.unique_stops));
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