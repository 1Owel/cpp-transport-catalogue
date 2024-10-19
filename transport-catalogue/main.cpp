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

    RenderSettings settings;

    const auto querry = JSONToTransport(file, catalogue, settings);

    GetJSONAnswer(catalogue, settings, querry.GetRoot(), out);

}