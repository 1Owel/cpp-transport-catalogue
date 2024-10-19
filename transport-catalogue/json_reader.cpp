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
        catalogue.AddBus(mreq.at("name").AsString(), stops, mreq.at("is_roundtrip").AsBool());
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

// Формирует и возвращает ответ на запрос типа Map (Карта)
inline json::Dict GetMapAnswr([[maybe_unused]] TransportCatalogue& catalogue, const RenderSettings& settings, const json::Dict& query) {
    json::Dict answr;
    std::ostringstream map_render;
    RenderAllRoutes(catalogue.GetAllBuses(), settings, map_render);
    answr["map"] = map_render.str();
    answr["request_id"] = query.at("id");
    return answr;
}

// Выводит ответ на запрос в формате JSON
void GetJSONAnswer([[maybe_unused]] TransportCatalogue& catalogue, const RenderSettings& settings, const json::Node& node, std::ostream& out) {
    using Array = std::vector<json::Node>;
    using namespace std::literals;
    if (node.AsMap().at("stat_requests").AsArray().empty()) {return;}

    Array result;
    for (const auto& request : (node.AsMap().at("stat_requests")).AsArray()) {
        const auto& query = request.AsMap();
        if (query.at("type").AsString() == "Stop") {
            result.push_back(GetStopAnswr(catalogue, query));
        } else if (query.at("type").AsString() == "Bus") {
            result.push_back(GetBusAnswr(catalogue, query));
        } else if (query.at("type").AsString() == "Map") {
            result.push_back(GetMapAnswr(catalogue, settings, query));
        }
    }
    json::Print(json::Document(json::Node(result)), out);
    out << std::endl;
}

void ApplyCommandsFromVariant([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& node) {
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

}

// Чтение цвета из ноды
svg::Color ReadColor(const json::Node& node) {
    if (node.IsArray()) {
        const auto& arr = node.AsArray();
        if (arr.size() == 3) {
            return svg::Rgb(static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), static_cast<uint8_t>(arr[2].AsInt()));
        } else {
            return svg::Rgba(static_cast<uint8_t>(arr[0].AsInt()), static_cast<uint8_t>(arr[1].AsInt()), static_cast<uint8_t>(arr[2].AsInt()), (arr[3].AsDouble()));
        }
    } else if (node.IsString()) {
        return node.AsString();
    }
    return svg::NoneColor;
}

// Заполнение палитры
std::vector<svg::Color> ReadColorPalette(const json::Node& node) {
    auto& json_palate = node.AsArray();
    std::vector<svg::Color> color_palette;
    for (auto& i : json_palate) {
        color_palette.push_back(ReadColor(i));
    }
    return color_palette;
}

// Чтение параметров рендера
void GetJSONRenderSettings(const json::Node& node, RenderSettings& settings) {
    const auto& settings_map = node.AsMap().at("render_settings").AsMap();
    settings.width = settings_map.at("width").AsDouble();
    settings.height = settings_map.at("height").AsDouble();
    settings.padding = settings_map.at("padding").AsDouble();
    settings.stop_radius = settings_map.at("stop_radius").AsDouble();
    settings.line_width = settings_map.at("line_width").AsDouble();
    settings.bus_label_font_size = settings_map.at("bus_label_font_size").AsInt();
    settings.bus_label_offset = {settings_map.at("bus_label_offset").AsArray()[0].AsDouble()
    , settings_map.at("bus_label_offset").AsArray()[1].AsDouble()};
    settings.stop_label_font_size = settings_map.at("stop_label_font_size").AsInt();
    settings.stop_label_offset = {settings_map.at("stop_label_offset").AsArray()[0].AsDouble()
    , settings_map.at("stop_label_offset").AsArray()[1].AsDouble()};
    settings.underlayer_color = ReadColor(settings_map.at("underlayer_color"));
    settings.underlayer_width = settings_map.at("underlayer_width").AsDouble();
    settings.color_palette = ReadColorPalette(settings_map.at("color_palette"));
}