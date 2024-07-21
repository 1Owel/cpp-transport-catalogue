#pragma once

#include <iosfwd>
#include <iostream>
#include <string_view>
#include <algorithm>
#include <unordered_set>

#include "transport_catalogue.h"


void ParseAndPrintStat(TransportCatalogue& tansport_catalogue, std::string_view request,
                       std::ostream& output);