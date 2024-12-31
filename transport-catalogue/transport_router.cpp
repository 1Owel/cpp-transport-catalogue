#include "transport_router.h"

    // Рассчет времени между остановками / вес ребра
    template <typename RouteIterator>
    double GraphBuilder::ComputeTime(double bus_velocity, const RouteIterator& to) {
        double dist = 0;
        const auto& container = catalogue_.GetDistanceContainer();
        auto it_dist = container.find({*(to - 1), *to});
        if (it_dist != container.end()) {
            dist += it_dist->second;
        } else {
            dist += container.at({*to, *(to - 1)});
        }
        // Время поездки в минутах
        return ((dist / 1000) / bus_velocity) * 60;
    }

    template <typename RouteIt>
    void GraphBuilder::ComputeAndAddEdge(double time, const RouteIt& from, const RouteIt& to, const Bus* bus, double wait_time) {
        using namespace std::literals;
        Stop *f = *from;
        Stop *t = *to;
        size_t from_index;
        size_t to_index;
        auto it_from = name_to_vortex_index_.find(f->name);
        if (it_from == name_to_vortex_index_.end()) {
            from_index = vortex_order.size();
            // Ребро ожидания
            name_to_vortex_index_.emplace(f->name, from_index);
            vortex_order.push_back(f);
            vortex_order.push_back(f);
            graph_.AddEdge({from_index, (from_index + 1), wait_time});
            edge_info_.push_back(WaitEdgeInfo{f});
        } else {
            from_index = it_from->second;
        }
        // Отправка автобуса всегда со второй половины остановки
        ++from_index;

        auto it_to = name_to_vortex_index_.find(t->name);
        if (it_to == name_to_vortex_index_.end()) {
            to_index = vortex_order.size();
            // Ребро ожидания
            name_to_vortex_index_.emplace(t->name, to_index);
            vortex_order.push_back(t);
            vortex_order.push_back(t);
            graph_.AddEdge({to_index, (to_index + 1), wait_time});
            edge_info_.push_back(WaitEdgeInfo{t});
        } else {
            to_index = it_to->second;
        }

        graph_.AddEdge({from_index, to_index, time});
        edge_info_.push_back(BusEdgeInfo{bus, static_cast<size_t>(std::distance(from, to)), time});
    }

    std::vector<Stop*> GraphBuilder::RouteMaker(const Bus& bus) {
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

    GraphBuilder::GraphBuilder(const TransportCatalogue& catalogue, 
    const Routing_settings& settings) : catalogue_(catalogue),  graph_(2 * catalogue.GetNameToStop().size()) {
        const auto& name_to_stop = catalogue_.GetNameToStop();
        vortex_order.reserve(name_to_stop.size() * 2);
        edge_info_.reserve(name_to_stop.size() * 2);
        name_to_vortex_index_.reserve(name_to_stop.size());
        // -buses
        // --stops
        // ---stops
        for (const Bus& bus : catalogue_.GetAllBuses()) {
            // Разворачивает маршрут если он не кольцевой
            const std::vector<Stop*> route = RouteMaker(bus);
            for (auto from = route.begin(); from < route.end(); from++)
            {
                // Возможное ускорение - переменная для времени здесь, ComputeTime находит только новую остановку (Последнюю)
                double time = 0;
                for (auto to = (from + 1); to < route.end(); to++)
                {
                    time += ComputeTime(settings.bus_velocity, to);
                    ComputeAndAddEdge(time, from, to, &bus, settings.bus_wait_time);
                }
            }   
        }
    }

    std::optional<graph::Router<double>::RouteInfo> RouterBuilder::BuildRoute(std::string_view from, std::string_view to) const {
        auto it_from = name_to_vortex_index_.find(from);
        auto it_to = name_to_vortex_index_.find(to);
        size_t from_index;
        size_t to_index;

        if (it_from != name_to_vortex_index_.end() && it_to != name_to_vortex_index_.end()) {
            from_index = it_from->second;
            to_index = it_to->second;
            return router_.BuildRoute(from_index, to_index);
        } else {
            return std::optional<graph::Router<double>::RouteInfo>(std::nullopt);
        }
    }