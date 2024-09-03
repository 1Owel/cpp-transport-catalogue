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

std::map<std::string_view, svg::Color> GetColoredBuses(const std::deque<Bus>& buses,
const std::vector<svg::Color>& colors) { // Вернет Map с цветами из colors
    std::map<std::string_view, svg::Color> colored_buses;
    auto color_it = colors.begin();
    for (const auto& bus : buses) {
        colored_buses[bus.name] = *color_it;
        if (!bus.route.empty()) {
            ++color_it;
            if (color_it == colors.end()) { // Проверка не нужна если маршрут пустой, так как цвет остаеться тем же
                color_it = colors.begin();
            }
        }
    }
    return colored_buses;
}

void RenderAllRoutes(const std::deque<Bus>& buses, const RenderSettings& settings, std::ostream& out) {
    const std::map<std::string_view, svg::Color> colored_buses = GetColoredBuses(buses, settings.color_palette); // Название маршрута \ цвет
    svg::Document picture; // Картина / карта
    const std::vector<Coordinates> all_stops_cords = GetAllStopsCoords(buses); // Координаты всех остановок, нужны для конвертации координат на полотно картинки
    SphereProjector projector(all_stops_cords.begin(), all_stops_cords.end(), settings.width, settings.height, settings.padding); // Форматирует координаты geo в svg::Point оператором ()
    for (const auto& bus : buses) {
        picture.Add(BusToPolyline(bus, settings, projector, colored_buses.at(bus.name)));
    }
    picture.Render(out);
}