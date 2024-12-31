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
inline void GetStopAnswr([[maybe_unused]] TransportCatalogue& catalogue, const json::Dict& query, json::Builder& result) {
    using namespace std::literals;
    using namespace json;

    if (!catalogue.HasStop(query.at("name").AsString())) {
        result.StartDict().Key("error_message").Value("not found");
    } else {
        auto buses_set = catalogue.GetBusesOnStop(query.at("name").AsString());
        result.StartDict().Key("buses").StartArray();
        for (std::string_view i : buses_set) {
            result.Value(static_cast<std::string>(i.data()));
        }
        result.EndArray();
    }
    result.Key("request_id").Value(query.at("id").AsInt()).EndDict();
}

// Формирует и возвращает ответ на запрос типа Bus (Маршрут)
inline void GetBusAnswr([[maybe_unused]] TransportCatalogue& catalogue, const json::Dict& query, json::Builder& result) {
    using namespace std::literals;
    auto routeinf = catalogue.GetRouteInfo(query.at("name").AsString());
    if (routeinf.name.empty()) {
        result.StartDict().Key("error_message").Value("not found");
    } else {
        result.StartDict()
        .Key("curvature").Value(routeinf.curvature)
        .Key("route_length").Value(static_cast<int>(routeinf.r_distance))
        .Key("stop_count").Value(static_cast<int>(routeinf.all_stops))
        .Key("unique_stop_count").Value(static_cast<int>(routeinf.unique_stops));
    }
    result.Key("request_id").Value(query.at("id").AsInt()).EndDict();
}

// Формирует и возвращает ответ на запрос типа Map (Карта)
inline void GetMapAnswr([[maybe_unused]] TransportCatalogue& catalogue, const RenderSettings& settings, const json::Dict& query, json::Builder& result) {
    std::ostringstream map_render;
    RenderAllRoutes(catalogue.GetAllBuses(), settings, map_render);
    result.StartDict()
    .Key("map").Value(map_render.str())
    .Key("request_id").Value(query.at("id").AsInt())
    .EndDict();
}

inline void GetRouteAnswr([[maybe_unused]] TransportCatalogue &catalogue, const Routing_settings &settings, const json::Dict &query, json::Builder &result, const RouterBuilder& rb)
{
    std::string_view from = query.at("from").AsString();
    std::string_view to = query.at("to").AsString();
    auto route = rb.BuildRoute(from, to);

    if (!route.has_value())
    {
        result.StartDict()
            .Key("request_id")
            .Value(query.at("id").AsInt())
            .Key("error_message")
            .Value("not found")
            .EndDict();
    }
    else
    {
        result.StartDict()
            .Key("request_id")
            .Value(query.at("id").AsInt())
            .Key("total_time")
            .Value(route->weight)
            .Key("items")
            .StartArray();
        // Заполнить Array с результата работы кода выше, Dict с действиями
        for (size_t edgeid : route->edges)
        {
            const EdgeInfo& edge = rb.GetEdgeInfo(edgeid);
            switch (edge.index())
            {
            case 0:
            {
                const BusEdgeInfo& info = std::get<BusEdgeInfo>(edge);
                result.StartDict()
                    .Key("type")
                    .Value("Bus")
                    .Key("bus")
                    .Value(info.bus_ptr->name)
                    .Key("span_count")
                    .Value(static_cast<int>(info.span_count))
                    .Key("time")
                    .Value(info.time)
                    .EndDict();
                break;
            }
            case 1:
            {
                const WaitEdgeInfo& winfo = std::get<WaitEdgeInfo>(edge);
                result.StartDict()
                    .Key("type")
                    .Value("Wait")
                    .Key("stop_name")
                    .Value(winfo.wait_place->name)
                    .Key("time")
                    .Value(settings.bus_wait_time)
                    .EndDict();
                break;
            }
            }
        }
        result.EndArray().EndDict();
    }
}

// Выводит ответ на запрос в формате JSON
void GetJSONAnswer([[maybe_unused]] TransportCatalogue& catalogue, const RenderSettings& render_settings, const Routing_settings& routing_setting, const json::Node& node, std::ostream& out) {
    using namespace std::literals;
    if (node.AsMap().at("stat_requests").AsArray().empty()) {return;}
    GraphBuilder gb(catalogue, routing_setting);
    RouterBuilder rb(catalogue, gb.GetBuildedGraph(), gb.GetInfoAllEdges(), gb.GetNameToVertex());

    json::Builder result;
    result.StartArray();
    for (const auto& request : (node.AsMap().at("stat_requests")).AsArray()) {
        const auto& query = request.AsMap();
        if (query.at("type").AsString() == "Stop") {
            GetStopAnswr(catalogue, query, result);
        } else if (query.at("type").AsString() == "Bus") {
            GetBusAnswr(catalogue, query, result);
        } else if (query.at("type").AsString() == "Map") {
            GetMapAnswr(catalogue, render_settings, query, result);
        } else if (query.at("type").AsString() == "Route") {
            GetRouteAnswr(catalogue, routing_setting, query, result, rb);
        }
    }
    result.EndArray();
    json::Print(json::Document(result.Build()), out);
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

// Чтение параметров для поиска маршрута
void GetJSONRouting_settings(const json::Node& node, Routing_settings& settings) {
    const auto& routing_settings = node.AsMap().at("routing_settings").AsMap();
    settings.bus_velocity = routing_settings.at("bus_velocity").AsDouble();
    settings.bus_wait_time = routing_settings.at("bus_wait_time").AsInt();
}