#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>

using Distance = std::unordered_map<std::string_view, unsigned int>;

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

Distance ParseDistance(std::string_view str) {
    // Stop Tolstopaltsevo: 55.611087, 37.20829, 3900m to Marushkino
    // Отсечение команды до дистанции
    Distance result;
    size_t str_b = 0;
    size_t str_e = 0;
    str = str.substr((str.find(',') + 1));
    str = str.substr((str.find(',') + 1));
    if (str.back() == ' ') {
        str = str.substr(str.find_last_not_of(' '));
    }

    // 3900m to Marushkino ,  3900m to Marushkino, 3900m to Marushkino
    // Разобрать дистанции на контейнер с остановкой и дистанцией к ней
    while (str_e != str.npos)
    {
        str_b = str.find_first_not_of(", ");
        str_e = str.find('m');
        if (str_e == str.npos) {
            return result;
        }
        int distance = std::stoi(std::string(str.substr(str_b, str_e)));
        str = str.substr(str_e);
        str_b = str.find_first_not_of("m ");
        str_b += 2;
        str = str.substr(str_b);
        str = str.substr(str.find_first_not_of(' '));
        str_e = str.find(',');
        if (str_e == str.npos) {
            std::string_view stop2 = str;
            result.emplace(stop2, distance);
        } else {
            std::string_view stop2 = str.substr(0, str_e);
            result.emplace(stop2, distance);
            str = str.substr(str_e);
        }
    }
    return result;
}


/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);
    auto comma2 = str.find(',');

    if (comma2 == str.npos) {
        return {nan, nan};
    }

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2, comma2 - not_space2)));

    return {lat, lng};
}



/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {
    for (auto& i : commands_) {
        if (i.command == "Stop") {
            catalogue.AddStop(i.id, ParseCoordinates(i.description));
        }
    }
    for (auto& i : commands_) {
        if (i.command == "Stop") {
            auto dist = ParseDistance(i.description);
            if (dist.empty()) {
                continue;
            }
            catalogue.AddDistance(i.id, dist);
        }
    }
    for (auto& i : commands_) {
        if (i.command == "Bus") {
            catalogue.AddBus(i.id, ParseRoute(i.description)); 
        }
    }
}