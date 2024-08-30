#include "json_reader.h"

//Передавать только node хранящую base_requests
void AddStopWithoutDist([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& request) {
    const auto& mreq = request.AsMap();
    if (mreq.at("type").AsString() == "Stop") {
        catalogue.AddStop(mreq.at("name").AsString(),
        {mreq.at("latitude").AsDouble(), mreq.at("longitude").AsDouble()});
    }
}

//Передавать только node хранящую base_requests
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

// Формирует и возвращает ответ на запрос типа Stop (Автобусы на остановке)
inline json::Dict GetStopAnswr([[maybe_unused]] TransportCatalogue& catalogue, const json::Dict& query) {
    using namespace std::literals;
    using namespace json;
    Dict answr;
    Array buses;

    if (!catalogue.HasStop(query.at("name").AsString())) {
        answr["error_message"] = "not found"s;
    } else {
        auto buses_set = catalogue.GetBusesOnStop(query.at("name").AsString());
        for (std::string_view i : buses_set) {
            buses.push_back(static_cast<std::string>(i.data()));
        }
        answr["buses"] = buses;
    }
    answr["request_id"] = json::Node(query.at("id"));
    return answr;
}

// Формирует и возвращает ответ на запрос типа Bus (Маршрут)
inline json::Dict GetBusAnswr([[maybe_unused]] TransportCatalogue& catalogue, const json::Dict& query) {
    using namespace std::literals;
    auto routeinf = catalogue.GetRouteInfo(query.at("name").AsString());
    json::Dict answr;
    if (routeinf.name.empty()) {
        answr["error_message"] = "not found"s;
    } else {
        answr["curvature"] = routeinf.curvature;
        answr["route_length"] = static_cast<int>(routeinf.r_distance);
        answr["stop_count"] = static_cast<int>(routeinf.all_stops);
        answr["unique_stop_count"] = static_cast<int>(routeinf.unique_stops);
    }
    answr["request_id"] = query.at("id");
    return answr;
}
// Выводит ответ на запрос в формате JSON
void GetJSONAnswer([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& node, std::ostream& out) {
    using Array = std::vector<json::Node>;
    using namespace std::literals;

    Array result;
    for (const auto& request : (node.AsMap().at("stat_requests")).AsArray()) {
        const auto& query = request.AsMap();
        if (query.at("type").AsString() == "Stop") {
            result.push_back(GetStopAnswr(catalogue, query));
        } else if (query.at("type").AsString() == "Bus") {
            result.push_back(GetBusAnswr(catalogue, query));
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