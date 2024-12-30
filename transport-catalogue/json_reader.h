#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "transport_router.h"

#include <iostream>
#include <variant>
#include <unordered_map>


void ApplyCommandsFromVariant([[maybe_unused]] TransportCatalogue& catalogue, const json::Node& node);

void GetJSONAnswer([[maybe_unused]] TransportCatalogue& catalogue, const RenderSettings& render_settings, const Routing_settings& routing_setting, const json::Node& node, std::ostream& out);

void GetJSONRenderSettings(const json::Node& node, RenderSettings& settings);

void GetJSONRouting_settings(const json::Node& node, Routing_settings& settings);

inline json::Document JSONToTransport(std::istream& in, TransportCatalogue& catalogue, RenderSettings& settings, Routing_settings& routing_setting) {
    auto json_query = json::Load(in);
    GetJSONRenderSettings(json_query.GetRoot(), settings);
    GetJSONRouting_settings(json_query.GetRoot(), routing_setting);
    ApplyCommandsFromVariant(catalogue, json_query.GetRoot());
    return json_query;
}