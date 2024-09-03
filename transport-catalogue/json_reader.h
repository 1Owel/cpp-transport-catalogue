#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"

#include <iostream>
#include <variant>
#include <unordered_map>

/*
using namespace json;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;
using NodeValue = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;
*/

void ApplyCommandsFromVariant([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& node, std::ostream& output);

void GetJSONAnswer([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& node, std::ostream& out);

inline void JSONToTransport(std::istream& in, TransportCatalogue& catalogue, std::ostream& out) {
    auto json_query = json::Load(in);
    ApplyCommandsFromVariant(catalogue, json_query.GetRoot(), out);
}

    /*
     * Примерная структура программы:
     *
     * Считать JSON из stdin
     * Построить на его основе JSON базу данных транспортного справочника
     * Выполнить запросы к справочнику, находящиеся в массива "stat_requests", построив JSON-массив
     * с ответами Вывести в stdout ответы в виде JSON
     */

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */