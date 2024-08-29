#include <fstream>
#include <iostream>
#include <string>

#include "request_handler.h"
#include "json_reader.h"

using namespace std;

int main() {
    TransportCatalogue catalogue;
    ifstream file("tests.txt");

    JSONToTransport(file, catalogue, cout);

}