#include <fstream>
#include <iostream>
#include <string>

#include "request_handler.h"
#include "json_reader.h"
#include "map_renderer.h"

using namespace std;

int main() {
    TransportCatalogue catalogue;
    ifstream file("in.txt");
    ofstream output("output.txt");

    RenderSettings settings;

    const auto querry = JSONToTransport(file, catalogue, settings);

    RenderAllRoutes(catalogue.GetAllBuses(), settings, output);

}