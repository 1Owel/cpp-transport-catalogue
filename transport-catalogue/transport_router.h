#pragma once

#include "router.h"
#include "domain.h"
#include "transport_catalogue.h"


#include <string>
#include <iterator>
#include <variant>

struct RoutingSettings
{
    // Минуты
    int bus_wait_time = 1;
    // Километры в час
    double bus_velocity = 1;
};

struct BusEdgeInfo {
    const Bus* bus_ptr;
    size_t span_count;
    double time;
};
struct WaitEdgeInfo {
    Stop* wait_place;
};
using EdgeInfo = std::variant<BusEdgeInfo, WaitEdgeInfo>;

class RouteSearch;

namespace TransportRouterInternals {
        
    class GraphBuilder
    {
    friend RouteSearch;
    private:
        std::vector<Stop*> vortex_order;
        std::vector<EdgeInfo> edge_info_;
        const TransportCatalogue& catalogue_;
        graph::DirectedWeightedGraph<double> graph_;
        std::unordered_map<std::string_view, size_t> name_to_vortex_index_;

        // Рассчет времени между остановками / вес ребра
        template <typename RouteIterator>
        double ComputeTime(double bus_velocity, const RouteIterator& to);

        template <typename RouteIt>
        void ComputeAndAddEdge(double time, const RouteIt& from, const RouteIt& to, const Bus* bus, double wait_time);

        std::vector<Stop*> RouteMaker(const Bus& bus);

        graph::DirectedWeightedGraph<double>& GetBuildedGraph() {
            return graph_;
        }

        std::vector<EdgeInfo>& GetInfoAllEdges() {
            return edge_info_;
        }

        std::unordered_map<std::string_view, size_t>& GetNameToVertex() {
            return name_to_vortex_index_;
        }

    public:
        GraphBuilder(const TransportCatalogue& catalogue, 
        const RoutingSettings& settings);

        ~GraphBuilder() = default;
    };


    class RouterBuilder
    {
    friend RouteSearch;
    public:
        RouterBuilder(const TransportCatalogue& catalogue,
        graph::DirectedWeightedGraph<double>& grph,
        std::vector<EdgeInfo>& edge_info,
        std::unordered_map<std::string_view, size_t>& ntvi) 
        : name_to_stop_(catalogue.GetNameToStop()), 
        edge_info_(edge_info),
        router_(grph), 
        name_to_vortex_index_(ntvi)
        {}

        ~RouterBuilder() = default;

    private:
        std::optional<graph::Router<double>::RouteInfo> BuildRoute(std::string_view from, std::string_view to) const;

        const EdgeInfo& GetEdgeInfo(size_t edgeid) const {
            return edge_info_.at(edgeid);
        }

        const std::unordered_map<std::string_view, Stop *>& name_to_stop_;
        const std::vector<EdgeInfo>& edge_info_;
        graph::Router<double> router_;
        const std::unordered_map<std::string_view, size_t>& name_to_vortex_index_;
    };

} // End of namespace TransportRouterInternals

struct RouteResult
{
    std::vector<EdgeInfo> route;
    double weight;
};


class RouteSearch {
private:
    TransportRouterInternals::GraphBuilder gb;
    TransportRouterInternals::RouterBuilder rb;
public:
    RouteSearch(const TransportCatalogue& catalogue, 
    const RoutingSettings& settings) 
    : gb(catalogue, settings), 
    rb(catalogue, gb.GetBuildedGraph(), gb.GetInfoAllEdges(), gb.GetNameToVertex()) 
    {}
    ~RouteSearch() = default;

    std::optional<RouteResult>  BuildRoute(std::string_view from, std::string_view to) const;

};
