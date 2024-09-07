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

void ApplyCommandsFromVariant([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& node);

void GetJSONAnswer([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& node, std::ostream& out);

void GetJSONRenderSettings(const json::Node& node, RenderSettings& settings);

inline json::Document JSONToTransport(std::istream& in, TransportCatalogue& catalogue, RenderSettings& settings) {
    auto json_query = json::Load(in);
    GetJSONRenderSettings(json_query.GetRoot(), settings);
    ApplyCommandsFromVariant(catalogue, json_query.GetRoot());
    return json_query;
}