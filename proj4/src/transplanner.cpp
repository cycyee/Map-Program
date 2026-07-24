#include "TransportationPlannerConfig.h"
#include "DijkstraTransportationPlanner.h"
#include "TransportationPlannerCommandLine.h"
#include "OpenStreetMap.h"
#include "CSVBusSystem.h"
#include "XMLReader.h"
#include "DSVReader.h"
#include "FileDataFactory.h"
#include "StandardDataSource.h"
#include "StandardDataSink.h"
#include "StandardErrorDataSink.h"
#include "StringUtils.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Interactive command-line transportation planner.
//
// Usage:  transplanner [--data=<dir>] [--results=<dir>]
//   --data     directory holding city.osm, stops.csv, routes.csv (default ./data)
//   --results  directory where "save" writes calculated paths     (default ./results)
//
// Once running, type "help" for the list of commands.
int main(int argc, char *argv[]) {
    std::string dataDir = "./data";
    std::string resultsDir = "./results";

    const std::string OSMFilename = "city.osm";
    const std::string StopFilename = "stops.csv";
    const std::string RouteFilename = "routes.csv";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.rfind("--data=", 0) == 0) {
            dataDir = arg.substr(std::string("--data=").size());
        }
        else if (arg.rfind("--results=", 0) == 0) {
            resultsDir = arg.substr(std::string("--results=").size());
        }
        else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: transplanner [--data=<dir>] [--results=<dir>]\n"
                      << "  Loads city.osm, stops.csv, routes.csv from the data directory,\n"
                      << "  then starts an interactive planner. Type \"help\" for commands.\n";
            return EXIT_SUCCESS;
        }
        else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return EXIT_FAILURE;
        }
    }

    auto dataFactory = std::make_shared<CFileDataFactory>(dataDir);
    auto resultsFactory = std::make_shared<CFileDataFactory>(resultsDir);

    auto osmSource = dataFactory->CreateSource(OSMFilename);
    auto stopSource = dataFactory->CreateSource(StopFilename);
    auto routeSource = dataFactory->CreateSource(RouteFilename);
    if (!osmSource || !stopSource || !routeSource) {
        std::cerr << "Error: could not open data files in '" << dataDir << "'.\n"
                  << "Expected " << OSMFilename << ", " << StopFilename
                  << ", and " << RouteFilename << ".\n";
        return EXIT_FAILURE;
    }

    auto xmlReader = std::make_shared<CXMLReader>(osmSource);
    auto streetMap = std::make_shared<COpenStreetMap>(xmlReader);

    auto stopReader = std::make_shared<CDSVReader>(stopSource, ',');
    auto routeReader = std::make_shared<CDSVReader>(routeSource, ',');
    auto busSystem = std::make_shared<CCSVBusSystem>(stopReader, routeReader);

    auto config = std::make_shared<STransportationPlannerConfig>(streetMap, busSystem);
    auto planner = std::make_shared<CDijkstraTransportationPlanner>(config);

    auto stdIn = std::make_shared<CStandardDataSource>();
    auto stdOut = std::make_shared<CStandardDataSink>();
    auto stdErr = std::make_shared<CStandardErrorDataSink>();

    CTransportationPlannerCommandLine commandLine(stdIn, stdOut, stdErr, resultsFactory, planner);

    return commandLine.ProcessCommands() ? EXIT_SUCCESS : EXIT_FAILURE;
}
