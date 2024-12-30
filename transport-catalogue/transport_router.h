#pragma once

#include "router.h"
#include "domain.h"


#include <string>
#include <iterator>

struct Routing_settings
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

class GraphBuilder
{
private:
    std::vector<Stop*> vortex_order;
    std::vector<EdgeInfo> edge_info_;
    const TransportCatalogue& catalogue_;
    graph::DirectedWeightedGraph<double> graph_;

    // Рассчет времени между остановками / вес ребра
    template <typename RouteIterator>
    double ComputeTime(double bus_velocity, const RouteIterator& from, const RouteIterator& to) {
        double dist = 0.0;
        const auto& distance_container = catalogue_.GetDistanceContainer();
        for(RouteIterator i = from + 1; i < (to + 1); ++i) {
            auto it_dist = distance_container.find({*(i - 1), *i});
            if (it_dist != distance_container.end()) {
                dist += it_dist->second;
            } else {
                it_dist = distance_container.find({*i, *(i - 1)});
                dist += it_dist->second;
            }
        }
        // Время поездки в минутах
        return ((dist / 1000) / bus_velocity) * 60;
    }

    template <typename RouteIt>
    void ComputeAndAddEdge(double time, const RouteIt& from, const RouteIt& to, const Bus* bus) {
        Stop *f = *from;
        Stop *t = *to;
        const auto& name_to_stop = catalogue_.GetNameToStop();
        size_t from_index = static_cast<size_t>(std::distance(name_to_stop.begin(), name_to_stop.find(f->name)));
        size_t to_index = static_cast<size_t>(std::distance(name_to_stop.begin(), name_to_stop.find(t->name)));
        // Конвертация индекса name_to_stop в индекс vertex_order
        // Отправление (from) идет со второй половины остановки, поэтому нужен + 1
        if (from_index == 0)
        {
            ++from_index;
        }
        else
        {
            from_index = (from_index * 2) + 1;
        }
        to_index = to_index * 2;
        graph_.AddEdge({from_index, to_index, time});
        edge_info_.push_back(BusEdgeInfo{bus, static_cast<size_t>(std::distance(from, to)), time});
    }

    std::vector<Stop*> RouteMaker(const Bus& bus) {
        std::vector<Stop *> route;
        if (!bus.roundtrip)
        {
            route.reserve(bus.route.size() * 2);
            for (auto i = bus.route.begin(); i < bus.route.end(); i++)
            {
                route.push_back(*i);
            }
            for (auto i = bus.route.rbegin() + 1; i < bus.route.rend(); i++)
            {
                route.push_back(*i);
            }
        }
        else
        {
            route.reserve(bus.route.size());
            for (Stop *s : bus.route)
            {
                route.push_back(s);
            }
        }
        return route;
    }


public:
    GraphBuilder(const TransportCatalogue& catalogue, 
    const Routing_settings& settings) : catalogue_(catalogue),  graph_(2 * catalogue.GetNameToStop().size()) {
        const auto& name_to_stop = catalogue_.GetNameToStop();
        vortex_order.reserve(name_to_stop.size() * 2);
        edge_info_.reserve(name_to_stop.size() * 2);
        // Добавление вершин \ остановок
        for (const auto& stop : name_to_stop) {
            vortex_order.push_back(stop.second);
            vortex_order.push_back(stop.second);
        }
        // Заполнение ожидания (Время пересадки)
        for (size_t i = 0; i < vortex_order.size(); i += 2)
        {
            // Каждая остановка - пара, вес между ними - пересадка
            graph_.AddEdge({i, (i + 1), static_cast<double>(settings.bus_wait_time)});
            edge_info_.push_back(WaitEdgeInfo{vortex_order.at(i)});
        }
        // Заполнение маршрутов
        for (const Bus& bus : catalogue_.GetAllBuses()) {
            // Разворачивает маршрут если он не кольцевой
            const std::vector<Stop*> route = RouteMaker(bus);
            for (auto from = route.begin(); from < route.end(); from++)
            {
                for (auto to = (from + 1); to < route.end(); to++)
                {
                    // Расчет времени поездки
                    double time = ComputeTime(settings.bus_velocity, from, to);
                    ComputeAndAddEdge(time, from, to, &bus);
                }
            }
        }
    }

    graph::DirectedWeightedGraph<double>&& GetBuildedGraph() {
        return std::move(graph_);
    }

    std::vector<EdgeInfo>&& GetInfoAllEdges() {
        return std::move(edge_info_);
    }

    ~GraphBuilder() = default;
};


class RouterBuilder
{
public:
    RouterBuilder(const TransportCatalogue& catalogue, graph::DirectedWeightedGraph<double>&& grph, std::vector<EdgeInfo>&& edge_info) 
    : name_to_stop_(catalogue.GetNameToStop()), 
    edge_info_(edge_info),
    router_(grph) 
    {}

    ~RouterBuilder() = default;

    auto BuildRoute(std::string_view from, std::string_view to) const {
        size_t from_index = static_cast<size_t>(std::distance(name_to_stop_.begin(), name_to_stop_.find(from)));
        size_t to_index = static_cast<size_t>(std::distance(name_to_stop_.begin(), name_to_stop_.find(to)));
        // Конвертация в индекс роутера
        from_index = from_index * 2;
        to_index = to_index * 2;
        return router_.BuildRoute(from_index, to_index);
    }

    const EdgeInfo& GetEdgeInfo(size_t edgeid) const {
        return edge_info_.at(edgeid);
    }

private:
    const std::unordered_map<std::string_view, Stop *>& name_to_stop_;
    std::vector<EdgeInfo>& edge_info_;
    graph::Router<double> router_;
};