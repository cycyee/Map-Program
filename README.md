# Campus Mapping Tool

Multi-modal navigation tool for a college campus, built in C++. Parses OpenStreetMap XML and bus system CSV data, then computes optimal routes using Dijkstra's algorithm across walking, biking, and bus networks. Route accuracy validated within 15% of Google Maps.

## How it works

**Data ingestion** — custom XML parser reads `.osm` files to extract street nodes, ways, speed limits, one-way restrictions, and bike accessibility. A separate CSV parser loads bus routes and stop locations.

**Graph construction** — builds three weighted graphs from the same node set:
- **Shortest distance** — edge weights are Haversine distances between nodes
- **Fastest by bike** — edge weights are distance / bike speed, only on bikeable ways (always bidirectional)
- **Fastest by walk + bus** — walking edges everywhere, bus edges between stops weighted by distance / speed limit + stop dwell time

**Pathfinding** — Dijkstra's algorithm with a min-heap (via `std::make_heap` / `std::pop_heap`) finds the shortest or fastest path. For fastest-path queries, the planner runs both the bike and walk+bus routers and returns whichever is quicker.

**Output** — results rendered to the terminal via a CLI, or exported as KML files for visualization in Google Earth.

## Features

- OpenStreetMap XML parsing (custom parser built on Expat)
- CSV bus system ingestion (routes, stops, paths)
- Haversine distance calculation for geographic coordinates
- Multi-modal routing: walk, bike, bus with automatic mode selection
- Bearing calculation and compass direction output
- DMS coordinate formatting
- KML export for route visualization
- Google Test suite with unit tests for each component
- Docker-based development environment

## Build & run

Requires a C++17 compiler, Make, and Expat XML library.

```bash
cd proj4
make
./bin/transplanner    # interactive CLI
```

Or with Docker:
```bash
python3 start_ecs34_development.py
cd proj4 && make
```

## Architecture

```
proj4/
  src/
    OpenStreetMap.cpp           # OSM XML parser
    DijkstraPathRouter.cpp      # Generic Dijkstra with min-heap
    DijkstraTransportationPlanner.cpp  # Multi-modal graph builder + router
    GeographicUtils.cpp         # Haversine, bearing, DMS conversion
    CSVBusSystem.cpp            # Bus route/stop parser
    TransportationPlannerCommandLine.cpp  # CLI interface
    KMLWriter.cpp               # KML route export
  include/                      # Header files / interfaces
  testsrc/                      # Google Test suites
  data/
    city.osm                    # OpenStreetMap data
    routes.csv, stops.csv       # Bus system data
```

## Stack

C++17, Dijkstra's Algorithm, OpenStreetMap, Expat XML, Google Test, Docker, Make
