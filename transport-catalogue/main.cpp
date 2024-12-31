#include <fstream>
#include <iostream>
#include <string>

#include "request_handler.h"
#include "json_reader.h"
#include "map_renderer.h"

using namespace std;

int main() {
    TransportCatalogue catalogue;
    ifstream file("tests.txt");
    ofstream out("out.svg");

    RenderSettings render_settings;
    Routing_settings routing_settings;

    const auto querry = JSONToTransport(file, catalogue, render_settings, routing_settings);
    {
    GetJSONAnswer(catalogue, render_settings, routing_settings, querry.GetRoot(), out);
    }
}