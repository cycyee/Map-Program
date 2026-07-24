#include "DijkstraTransportationPlanner.h"
#include <unordered_map>
#include "DijkstraPathRouter.h"
#include "GeographicUtils.h"
#include "BusSystemIndexer.h"
#include "TransportationPlannerConfig.h"
#include "OpenStreetMap.h"
#include <iostream>
#include <string>
#include <algorithm>

struct CDijkstraTransportationPlanner::SImplementation {
    std::shared_ptr < CStreetMap > DStreetMap;
    std::shared_ptr < CBusSystem > DBusSystem;

    std::unordered_map< CStreetMap::TNodeID, CPathRouter::TVertexID> DNodeToVertexID;
    std::unordered_map< CPathRouter::TVertexID, CStreetMap::TNodeID> DVertexToNodeID;
    CDijkstraPathRouter DShortestPathRouter;
    CDijkstraPathRouter DFastestPathRouterBike;
    CDijkstraPathRouter DFastestPathRouterWalkBus;

    std::unordered_map <CStreetMap::TNodeID, CTransportationPlanner::ETransportationMode> TransportType;
    std::unordered_map <CStreetMap::TNodeID, double> BusSpeedLimits;
    std::unordered_map <CStreetMap::TNodeID, bool> BIDIR;
    std::vector<TNodeID> NodeVect;

    SImplementation(std::shared_ptr<SConfiguration> config)  {
        DStreetMap = config->StreetMap();//pull map and busSystem from config
        DBusSystem = config->BusSystem();
        CBusSystemIndexer BusSystemIndexer(DBusSystem);
        //check for null map or bus system (should never happen pretty much)
        if(DStreetMap == nullptr || DBusSystem == nullptr) {
            std::cout<<"Huge error"<<std::endl;
        }
        //std::cout<<"streetmap node count: "<<DStreetMap->NodeCount()<<std::endl;      testing line
        for(size_t Index = 0; Index < DStreetMap->NodeCount(); Index++) { //populate DNodeToVertexID + routers with vertices
            auto Node = DStreetMap->NodeByIndex(Index);
            //std::cout<<"Node ID: "<<Node->ID()<<std::endl;        testing line
            auto VertexID = DShortestPathRouter.AddVertex(Node->ID());//add in vertex after getting it from streetmap
            DFastestPathRouterBike.AddVertex(Node->ID());//add vertices for fastest pathrouters too
            DFastestPathRouterWalkBus.AddVertex(Node->ID());
            DNodeToVertexID[Node->ID()] = VertexID;//populate DNodeToVertexID
            DVertexToNodeID[VertexID] = Node->ID();
            NodeVect.push_back(Node->ID());
        }
        std::sort(NodeVect.begin(), NodeVect.end());//sort nodevect
        //std::cout<<"Way Count: "<<DStreetMap->WayCount()<<std::endl;          testing line
        
        for(size_t Index = 0; Index < DStreetMap->WayCount(); Index++) { //iterate through ways
            auto way = DStreetMap->WayByIndex(Index);
            //std::cout<<"Way ID: "<<way->ID()<<std::endl;
            //std::cout<<"nodes in way:"<<way->NodeCount()<<std::endl;
            bool Bikable = way->GetAttribute("bicycle") != "no"; //means that its not bikable/bikable 
            bool Bidirectional = way->GetAttribute("oneway") != "yes"; //decides if bidir flag is passed
            double speed = config->DefaultSpeedLimit();
            if(way->HasAttribute("maxspeed")) {
                std::string str = way->GetAttribute("maxspeed").substr(0, way->GetAttribute("maxspeed").length() - 4);//gets just the number part
                speed = stod(str);         //string to double      
            }

            for(size_t NodeIndex = 0; NodeIndex < way->NodeCount(); NodeIndex++) { //go through nodes in each way
                //std::cout << "-------" << std::endl;
                //std::cout<<"entering node loop: "<<std::endl;     testing lines
                auto currentNodeID = way->GetNodeID(NodeIndex); 
                auto nextNodeID = way->GetNodeID(NodeIndex + 1);

                auto currentNode = DStreetMap->NodeByID(currentNodeID);
                if(currentNode == nullptr) {break;} //null checks to solve segfaulting
                //std::cout << "Current Node: " << currentNode->ID() << std::endl;      testing line
                auto nextNode = DStreetMap->NodeByID(nextNodeID);
                
                if (nextNode == nullptr) {break;} //null check
                //std::cout << "Next Node: " << nextNode->ID() << std::endl;
                //std::cout << "---" << std::endl;          testing lines
                
                auto currentVertexID = DNodeToVertexID[currentNodeID];
                auto nextVertexID = DNodeToVertexID[nextNodeID];
                //std::cout << "Current Vertex ID: " << currentVertexID << std::endl;
                //std::cout << "Next Vertex ID: " << nextVertexID << std::endl;
                   
                double weightDist = SGeographicUtils::HaversineDistanceInMiles(currentNode->Location(), nextNode->Location());//get distance using geo utils fn
                double weightTimeWalk = weightDist/config->WalkSpeed();//divide dist/speed = time
                double weightTimeBike = weightDist/config->BikeSpeed();
                // std::cout << "Walk speed: " << config->WalkSpeed() << std::endl;     testing lines
                // std::cout << "Bike speed: " << config->BikeSpeed() << std::endl;
                // std::cout << "Distance weight: " << weightDist << std::endl;
                // std::cout << "Time weight (walk): " << weightTimeWalk << std::endl;
                // std::cout << "Time weight (bike): " << weightTimeBike << std::endl;

                BusSpeedLimits[currentNodeID] = speed; //set speed limit for later, bidir for later, transport type for later
                BIDIR[currentNodeID] = Bidirectional;
                TransportType[currentNodeID] = CTransportationPlanner::ETransportationMode::Walk;//default all walk

                //std::cout << "dist:" << std::endl;
                DShortestPathRouter.AddEdge(currentVertexID, nextVertexID, weightDist, Bidirectional);
                //std::cout << "walk time:" << std::endl;
                DFastestPathRouterWalkBus.AddEdge(currentVertexID, nextVertexID, weightTimeWalk, Bidirectional); //add walking edge everywhere
                if(Bikable == true) {//bikable edge always bidirectional
                    //std::cout << "bike time:" << std::endl;
                    DFastestPathRouterBike.AddEdge(currentVertexID, nextVertexID, weightTimeBike, true); //only add edge if bikable
                }
                

                /*
                std::cout<<"entering node loop: "<<std::endl;
                auto PreviousNodeID = Way->GetNodeID(NodeIndex); 
                auto NextNodeID = Way->GetNodeID(NodeIndex + 1); //get next node id 
                auto PreviousNode = DStreetMap->NodeByID(PreviousNodeID);
                if(PreviousNode == nullptr) {std::cout<<"Previous Node null"<<std::endl; break;}
                auto NextNode = DStreetMap->NodeByID(NextNodeID);
                if(NextNode == nullptr) {break;}

                CPathRouter::TVertexID PreviousVertexID = DNodeToVertexID[PreviousNodeID];
                CPathRouter::TVertexID NextVertexID = DNodeToVertexID[NextNodeID];
                
                auto stop = BusSystemIndexer.StopByNodeID(PreviousNodeID); 
                double weightDist = SGeographicUtils::HaversineDistanceInMiles(PreviousNode->Location(), NextNode->Location());
                double weightTimeWalk = weightDist/config->WalkSpeed();
                double weightTimeBike = weightDist/config->BikeSpeed();

                BusSpeedLimits[PreviousNodeID] = speed; 
                BIDIR[PreviousNodeID] = Bidirectional;
                TransportType[PreviousNodeID] = CTransportationPlanner::ETransportationMode::Walk;
                
                DShortestPathRouter.AddEdge(PreviousVertexID, NextVertexID, weightDist, Bidirectional);
                DFastestPathRouterWalkBus.AddEdge(PreviousVertexID, NextVertexID, weightTimeWalk, Bidirectional);
                if(Bikable == true) {//bikable edge always bidirectional
                    DFastestPathRouterBike.AddEdge(PreviousVertexID, NextVertexID, weightTimeBike, false);
                }
                */

                /*
                auto stop = BusSystemIndexer.StopByNodeID(PreviousNodeID); 
                BusSpeedLimits[PreviousNode->ID()] = speed;//store speed limit
                double weightDist = SGeographicUtils::HaversineDistanceInMiles(PreviousNode->Location(), NextNode->Location());
                double weightTimeWalk = weightDist/config->WalkSpeed();
                double weightTimeBike = weightDist/config->BikeSpeed();
                DShortestPathRouter.AddEdge(PreviousVertexID, NextVertexID, weightDist, Bidirectional);
                DFastestPathRouterWalkBus.AddEdge(PreviousVertexID, NextVertexID, weightTimeWalk, true);
                BusSpeedLimits[PreviousNodeID] = speed;
                BIDIR[PreviousNodeID] = Bidirectional;
                TransportType[PreviousNodeID] = CTransportationPlanner::ETransportationMode::Walk;
                if(Bikable == true) {//bikable edge always bidirectional
                    DFastestPathRouterBike.AddEdge(PreviousVertexID, NextVertexID, weightTimeBike, Bidirectional);
                    std::cout<<"Bike edge added"<< weightTimeBike<< PreviousVertexID<<" to "<<NextVertexID<<std::endl;
                }
                */
            }
        }
        //loop through routes/stops
        for(size_t index = 0; index < DBusSystem->RouteCount(); index ++) {
            //std::cout<<"entering route loop: "<<std::endl;
            auto route = DBusSystem->RouteByIndex(index);
            //std::cout << "Nodes in route: " << route->StopCount() <<std::endl;
            std::vector<CPathRouter::TVertexID> path;
            for (size_t routeIndex = 0; routeIndex < route->StopCount(); routeIndex++) {//get current and next stops
                auto currentStopID = route->GetStopID(routeIndex);
                auto nextStopID = route->GetStopID(routeIndex + 1);
                auto currentStop = DBusSystem->StopByID(currentStopID);
                if(currentStop == nullptr) {break;} //null check
                //std::cout << "Current Stop: " << currentStop->NodeID() << std::endl;
                auto nextStop = DBusSystem->StopByID(nextStopID);
                if (nextStop == nullptr) {break;} //null check
                //std::cout << "Next Stop: " << nextStop->NodeID() << std::endl;

                auto currentVertexID = DNodeToVertexID[currentStop->NodeID()];//get vertex ids
                auto nextVertexID = DNodeToVertexID[nextStop->NodeID()];

                auto search = BusSpeedLimits.find(currentStop->NodeID());//search through bus speed limits
                double busSpeed = config->DefaultSpeedLimit();
                if (search != BusSpeedLimits.end()) {
                    busSpeed = search->second;//if non-default found, update bus speed 
                }

                bool bidir = BIDIR[currentStop->NodeID()];//pull directionality from 
                //std::cout<<"current id: "<<currentVertexID<<"next id: "<<nextVertexID<<std::endl;
                double distance = DShortestPathRouter.FindShortestPath(currentVertexID, nextVertexID, path);
                double weightTime = distance/busSpeed + (config->BusStopTime()/3600);
                //std::cout << "bus time:" << std::endl;
                DFastestPathRouterWalkBus.AddEdge(currentVertexID, nextVertexID, weightTime, false);
                TransportType[nextStop->NodeID()] = CTransportationPlanner::ETransportationMode::Bus;
            }
            /*
            auto Route = DBusSystem->RouteByIndex(Index);
            std::vector <CPathRouter::TVertexID> Path;
            for(size_t RouteIndex = 0; RouteIndex < Route->StopCount(); RouteIndex++) {
                auto previousStopID = Route->GetStopID(RouteIndex);
                auto nextStopID = Route->GetStopID(RouteIndex + 1);
                auto prevstop = DBusSystem->StopByID(previousStopID);
                if(prevstop == nullptr) {std::cout<<"prev stop is null"<<std::endl; break;}
                auto nextstop = DBusSystem->StopByID(nextStopID);
                if(nextstop == nullptr) {break;}
                CPathRouter::TVertexID PreviousVertexID = DNodeToVertexID[previousStopID];
                CPathRouter::TVertexID NextVertexID = DNodeToVertexID[nextStopID];

                
                auto search = BusSpeedLimits.find(prevstop->NodeID());
                double busspeed = config->DefaultSpeedLimit();
                if(search != BusSpeedLimits.end()) {
                    busspeed = search->second;
                }
                bool bidir = BIDIR[prevstop->NodeID()];
                double distance = DShortestPathRouter.FindShortestPath(PreviousVertexID, NextVertexID, Path);
                double weightTime = distance/busspeed + (config->BusStopTime()/3600);
                DFastestPathRouterWalkBus.AddEdge(PreviousVertexID, NextVertexID, weightTime, bidir);
                TransportType[prevstop->NodeID()] = CTransportationPlanner::ETransportationMode::Bus;
            }
            */
        }
    }

    std::size_t NodeCount() const noexcept {
        return DStreetMap->NodeCount();
    }
    std::shared_ptr<CStreetMap::SNode> SortedNodeByIndex(std::size_t index) const { //DNodeToVertexID holds the sorted nodes for lookup
        if(!NodeVect.empty()) {
            auto nodeid = NodeVect[index];
            auto nodeptr = DStreetMap->NodeByID(nodeid);
            return nodeptr;
        }
        return nullptr;
    }

    double FindShortestPath(TNodeID src, TNodeID dest, std::vector< TNodeID > &path) {//calls FindShortestPath by looking up the vertices in DNodetoVertexID
        std::vector <CPathRouter::TVertexID > ShortestPath;//vector for path to be drawn in
        if (DNodeToVertexID.find(src) == DNodeToVertexID.end() || DNodeToVertexID.find(dest) == DNodeToVertexID.end()) {
            // Handle error: Source or destination node not found
            std::cout<<"error here 1"<<std::endl;
            return CPathRouter::NoPathExists; // or any appropriate error code
        }
        auto SourceVertexID = DNodeToVertexID[src];
        auto DestinationVertexID = DNodeToVertexID[dest];
        auto Distance = DShortestPathRouter.FindShortestPath(SourceVertexID, DestinationVertexID, ShortestPath);
        path.clear();
        for(auto VertexID : ShortestPath) {
            path.push_back(std::any_cast<TNodeID>(DShortestPathRouter.GetVertexTag(VertexID)));
        }
        return Distance;
    }
    double FindFastestPath(TNodeID src, TNodeID dest, std::vector< TTripStep > &path) {
        //compare times, and return paths as appropriate
        path.clear();
        std::vector <CPathRouter::TVertexID> ShortestPathBike;
        std::vector <CPathRouter::TVertexID> ShortestPathWalkBus;
        auto PreviousVertexID = DNodeToVertexID[src];
        auto NextVertexID  = DNodeToVertexID[dest];
        
        auto TimeBike = DFastestPathRouterBike.FindShortestPath(PreviousVertexID, NextVertexID, ShortestPathBike);
        auto TimeWalkBus = DFastestPathRouterWalkBus.FindShortestPath(PreviousVertexID, NextVertexID, ShortestPathWalkBus);
        // std::cout << "Bike time:" << TimeBike << std::endl;
        // for (int i = 0; i < ShortestPathBike.size(); i++) {
        //     std::cout << "Node: " << DVertexToNodeID[ShortestPathBike[i]] << " Transport: Bike" << std::endl;
        // }
        //std::cout << "Walk/bus time: " << TimeWalkBus << std::endl;
        //for (int i = 0; i < ShortestPathWalkBus.size(); i++) {
            // if (TransportType[DVertexToNodeID[ShortestPathBike[i]]] == 
            // CTransportationPlanner::ETransportationMode::Walk) {
            //     std::cout << "Node: " << DVertexToNodeID[ShortestPathWalkBus[i]] << " Transport: Walk" <<std::endl;
            // }
            // else {
            //     std::cout << "Node: " << DVertexToNodeID[ShortestPathWalkBus[i]] << " Transport: Bus" <<std::endl;
            // }
            
        //}

        if (TimeBike < TimeWalkBus) {
            for (int i = 0; i < ShortestPathBike.size(); i++) {
                auto nodeID = DVertexToNodeID[ShortestPathBike[i]];
                auto nodeType = CTransportationPlanner::ETransportationMode::Bike;
                auto nodePair = std::make_pair(nodeType, nodeID);
                path.push_back(nodePair);
            }
            return TimeBike;
        }
        else {
            for (int i = 0; i < ShortestPathWalkBus.size(); i++) {
                auto nodeID = DVertexToNodeID[ShortestPathWalkBus[i]];
                auto nodeType = TransportType[nodeID];
                if(nodeID == 1) {
                    nodeType = CTransportationPlanner::ETransportationMode::Walk;
                }
                auto nodePair = std::make_pair(nodeType, nodeID);
                path.push_back(nodePair);
            }
            return TimeWalkBus;
        }

        /*
        CStreetMap::TNodeID nodeID;
        if(TimeBike < TimeWalkBus) {
            for(auto VertexID : ShortestPathBike) {
                auto nodeSearch = DNodeToVertexID.find(VertexID);
                if(nodeSearch != DNodeToVertexID.end()) {
                    nodeID = nodeSearch->second;
                    std::cout << "Node ID: " << nodeID << std::endl; 
                }
            auto nodepair = std::make_pair(CTransportationPlanner::ETransportationMode::Bike, nodeID);
            path.push_back(nodepair);
            }
            return TimeBike;
        }
        else{
            for(auto VertexID : ShortestPathWalkBus) {
                auto nodeSearch = DNodeToVertexID.find(VertexID);
                if(nodeSearch != DNodeToVertexID.end()) {
                    nodeID = nodeSearch->second;
                }
                auto search = TransportType.find(nodeID);
                if(search != TransportType.end()) {
                    auto nodepair = std::make_pair(search->second, VertexID);
                    path.push_back(nodepair);
                }
            }
            return TimeWalkBus;
        }
        */
    }
    bool GetPathDescription(const std::vector< TTripStep > &path, std::vector< std::string > &desc) const {
        return true;
    }

};

CDijkstraTransportationPlanner::CDijkstraTransportationPlanner(std::shared_ptr<SConfiguration> config) {
    DImplementation = std::make_unique<SImplementation>(config);
}
CDijkstraTransportationPlanner::~CDijkstraTransportationPlanner(){
}

std::size_t CDijkstraTransportationPlanner::NodeCount() const noexcept{
    return DImplementation->NodeCount();
}

std::shared_ptr<CStreetMap::SNode> CDijkstraTransportationPlanner::SortedNodeByIndex(std::size_t index) const noexcept {
    return DImplementation->SortedNodeByIndex(index);
}

double CDijkstraTransportationPlanner::FindShortestPath(TNodeID src, TNodeID dest, std::vector< TNodeID > &path) {
    return DImplementation->FindShortestPath(src, dest, path);
}

double CDijkstraTransportationPlanner::FindFastestPath(TNodeID src, TNodeID dest, std::vector< TTripStep > &path) {
    return DImplementation->FindFastestPath(src, dest, path);
}

bool CDijkstraTransportationPlanner::GetPathDescription(const std::vector< TTripStep > &path, std::vector< std::string > &desc) const {
    return DImplementation->GetPathDescription(path, desc);
}