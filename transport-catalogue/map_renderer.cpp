#include "map_renderer.h"

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршртутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

void GetRouteCoords(const Bus& bus, std::vector<Coordinates>& result_output) {
    // Перебор маршрута (Результат - контейнер координат маршрута)
    for (const auto& stop : bus.route) {
        result_output.push_back(stop->coords);
    }
}

std::vector<Coordinates> GetAllStopsCoords(const std::deque<Bus>& buses) {
    std::vector<Coordinates> all_stops_coords;
    for (const auto& bus : buses) {
        GetRouteCoords(bus, all_stops_coords);
    }
    return all_stops_coords;
}

svg::Polyline BusToPolyline(const Bus& bus, const RenderSettings& settings, SphereProjector& projector, svg::Color stroke_color) {
    svg::Polyline route;
    // Перебор остановок переданного маршрута
    for (const auto& stop : bus.route) {
        // Конвертация координат остановки geo в координаты заданого изображения svg
        // и добавление точки в Polyline (Ломаная линия)
        route.AddPoint(projector(stop->coords));
    }
    route.SetStrokeColor(stroke_color);
    // Дефолтная настройка для всех маршрутов
    route.SetFillColor(std::monostate());
    route.SetStrokeWidth(settings.line_width);
    route.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    route.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    return route;
}

struct BusLexCompare {
    bool operator()(const Bus* s1, const Bus* s2) const {
        return std::lexicographical_compare(s1->name.begin(), s1->name.end(), s2->name.begin(), s2->name.end());
    }
};

std::map<const Bus*, svg::Color, BusLexCompare> GetColoredBuses(const std::deque<Bus>& buses,
const std::vector<svg::Color>& colors) { // Вернет Map с цветами из colors
    std::map<const Bus*, svg::Color, BusLexCompare> colored_buses;
    for (const auto& bus : buses) {
        const Bus* b = &bus;
        colored_buses[b] = {};
    }
    auto color_it = colors.begin();
    for (auto& bus : colored_buses) {
        bus.second = *color_it;
        if (!bus.first->route.empty()) {
            ++color_it;
            if (color_it == colors.end()) { // Проверка не нужна если маршрут пустой, так как цвет остаеться тем же
                color_it = colors.begin();
            }
        }
    }
    return colored_buses;
}

/*
x и y — координаты соответствующей остановки;
смещение dx и dy равно настройке bus_label_offset;
размер шрифта font-size равен настройке bus_label_font_size;
название шрифта font-family — "Verdana";
толщина шрифта font-weight — "bold".
содержимое — название автобуса.
*/

// Создает пару, текст и подложку
std::pair<svg::Text, svg::Text> BusToText(const Bus& bus, const RenderSettings& settings, SphereProjector& projector, svg::Color color) {
    using namespace std::literals;

    svg::Text route_name;
    // Наполнение общими параметрами
    route_name.SetPosition(projector(bus.route.at(0)->coords));
    route_name.SetOffset({settings.bus_label_offset.first, settings.bus_label_offset.second});
    route_name.SetFontSize(static_cast<uint32_t>(settings.bus_label_font_size));
    route_name.SetFontFamily("Verdana"s);
    route_name.SetFontWeight("bold"s);
    route_name.SetData(bus.name);

    // Обводка
    svg::Text route_name_outline(route_name);
    route_name_outline.SetFillColor(settings.underlayer_color);
    route_name_outline.SetStrokeColor(settings.underlayer_color);
    route_name_outline.SetStrokeWidth(settings.underlayer_width);
    route_name_outline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    route_name_outline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    
    //Допсвойтсво текста
    route_name.SetFillColor(color);

    return {route_name, route_name_outline};
}

void RenderAllRoutes(const std::deque<Bus>& buses, const RenderSettings& settings, std::ostream& out) {
    const std::map<const Bus*, svg::Color, BusLexCompare> colored_buses = GetColoredBuses(buses, settings.color_palette); // Название маршрута \ цвет
    svg::Document picture; // Картина / карта
    const std::vector<Coordinates> all_stops_cords = GetAllStopsCoords(buses); // Координаты всех остановок, нужны для конвертации координат на полотно картинки
    SphereProjector projector(all_stops_cords.begin(), all_stops_cords.end(), settings.width, settings.height, settings.padding); // Форматирует координаты geo в svg::Point оператором ()
    // Рисует линии маршрутов
    for (const auto& bus : colored_buses) {
        if (!bus.first->route.empty()) {
            picture.Add(BusToPolyline(*bus.first, settings, projector, bus.second));
        }
    }
    // Пишет названия маршрута у конечных остановок
    for (const auto& bus : colored_buses) {
        if (!bus.first->route.empty()) {
            auto route_text = BusToText(*bus.first, settings, projector, bus.second);
            picture.Add(route_text.second); // Подложка
            picture.Add(route_text.first); // Текст
            if (!bus.first->roundtrip) { // Добавляет текст у конечной если маршрут не кольцевой
                // Смена координат на последнюю остановку
                Stop* center = bus.first->route.at(bus.first->route.size() / 2);
                
                route_text.first.SetPosition(projector(center->coords));
                route_text.second.SetPosition(projector(center->coords));

                picture.Add(route_text.second); // Подложка
                picture.Add(route_text.first); // Текст
            }
        }
    }
    picture.Render(out);
}