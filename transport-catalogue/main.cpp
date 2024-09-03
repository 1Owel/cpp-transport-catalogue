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

    JSONToTransport(file, catalogue, cout);
    
    RenderSettings settings;

    RenderAllRoutes(catalogue.GetAllBuses(), settings, cout);
}