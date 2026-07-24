//#include "BusSystem.h"
#include "CSVBusSystem.h"
#include "DSVReader.h"
#include <unordered_map>
#include <vector>
#include "StreetMap.h"
#include <string>
#include <iostream>

struct CCSVBusSystem::SImplementation {

    struct SStop : public CBusSystem::SStop{
        TStopID DStopID;
        CStreetMap::TNodeID DNodeID;
        SStop(TStopID stopid, TStopID nodeid) {
            DStopID = stopid;
            DNodeID = nodeid;
        }
        ~SStop(){
        }  
        TStopID ID() const noexcept override{
            return DStopID;
        }
        CStreetMap::TNodeID NodeID() const noexcept override {
            return DNodeID;
        }
    };
    
    struct SRoute: public CBusSystem::SRoute {
        std::string DRouteID;
        std::vector<TStopID> DStopID;
        SRoute(std::string routeid) {
            DRouteID = routeid;
            //DStopID.push_back(stopid);
        }
        ~SRoute(){
        }
        std::string Name() const noexcept override {
            return DRouteID;
        }
        std::size_t StopCount() const noexcept override {
            return DStopID.size();
        }
        TStopID GetStopID(std::size_t index) const noexcept override {
            if(DStopID.size() > index) {
                return (DStopID[index]);
            }
            return InvalidStopID;
        }
        void AddStopID(TStopID stopID) {
            DStopID.push_back(stopID);
        }
    };
    
    std::vector<std::shared_ptr<CBusSystem::SStop>> DStop;
    std::unordered_map<TStopID, std::shared_ptr<CBusSystem::SStop>> DStopIdToStop;

    std::vector<std::shared_ptr<CBusSystem::SRoute>> DRoute;
    std::unordered_map<std::string, std::shared_ptr<CBusSystem::SRoute> > DRouteIdToRoute;
    bool firstrow = true;
    SImplementation(std::shared_ptr< CDSVReader > stopsrc, std::shared_ptr< CDSVReader > routesrc) {
        std::vector<std::string> row;
        //store stop
        if(firstrow){
            stopsrc->ReadRow(row);
            routesrc->ReadRow(row);
            firstrow = false;
        }

        while (stopsrc->ReadRow(row)) {
            if (row.size() >= 2){
                //std::cout<<row[0]<<std::endl;//this is a testing line
                TStopID stopID = std::stoull(row[0]);
                //std::cout<<row[1]<<std::endl;//this is a testing line
                CStreetMap::TNodeID nodeID = std::stoull(row[1]);
                auto stop = std::make_shared<SStop>(stopID, nodeID);
                DStop.push_back(stop); //stored to DStop for index search
                DStopIdToStop[stopID] = stop; //stored to DStopIdToStop for id search
            }
        }
        //store route
        routesrc->ReadRow(row);
        std::string routeID;
        TStopID stopID;
        std::shared_ptr<SRoute> route;
        if(!row.empty()) {
            routeID = row[0];
            stopID = std::stoull(row[1]);
            route = std::make_shared<SRoute>(routeID);
            route->AddStopID(stopID);
            DRoute.push_back(route);
            DRouteIdToRoute[routeID] = route;
            //std::cout<<"stop id 1: "<<stopID<<std::endl;
        }
        while (routesrc->ReadRow(row)) {
            if (!row.empty()) {
                routeID = row[0];
                stopID = std::stoull(row[1]);
                auto routeIter = DRouteIdToRoute.find(routeID);
                if (routeIter != DRouteIdToRoute.end()) {
                    // If the route exists, add in stop
                    route->AddStopID(stopID);
                    //std::cout<<"stop id: "<<stopID<<std::endl;
                    //DStopID.push_back(stopID);
                }
                else {
                    // If the route does not exist, create new route and add stop in
                    auto newroute = std::make_shared<SRoute>(routeID);
                    //std::cout<<"stop id: "<<stopID<<std::endl;
                    newroute->AddStopID(stopID);
                    DRoute.push_back(newroute);
                    DRouteIdToRoute[routeID] = newroute;
                    route = newroute;
                }
            }    
        }
    }

    std::size_t StopCount() const {
        return DStop.size();
    }


    std::size_t RouteCount() const {
        return DRoute.size();

    }

    std::shared_ptr<CBusSystem::SStop> StopByIndex(std::size_t index) const {
        if(index < DStop.size()) {
            return DStop[index];
        }
        return nullptr;
    }


    std::shared_ptr<CBusSystem::SStop> StopByID(TStopID id) const {
        auto search = DStopIdToStop.find(id);
        if(search != DStopIdToStop.end()) {
            return search->second;
        }
        return nullptr;
    }


    std::shared_ptr<CBusSystem::SRoute> RouteByIndex(std::size_t index) const {
        if(index < DRoute.size()) {
            return DRoute[index];
        }
        return nullptr;
    }


    std::shared_ptr<CBusSystem::SRoute> RouteByName(const std::string &name) const {
        auto search = DRouteIdToRoute.find(name);
        if(search != DRouteIdToRoute.end()) {
            return search->second;
        }
        return nullptr;

    }
};



CCSVBusSystem::CCSVBusSystem(std::shared_ptr< CDSVReader > stopsrc, std::shared_ptr< CDSVReader > routesrc){
    DImplementation = std::make_unique<SImplementation>(stopsrc, routesrc);
}

CCSVBusSystem::~CCSVBusSystem(){
}


std::size_t CCSVBusSystem::StopCount() const noexcept {
    return DImplementation->StopCount();
}

std::size_t CCSVBusSystem::RouteCount() const noexcept {
    return DImplementation->RouteCount();
}

std::shared_ptr<CBusSystem::SStop> CCSVBusSystem::StopByIndex(std::size_t index) const noexcept {
    return DImplementation->StopByIndex(index);
}

std::shared_ptr<CBusSystem::SStop> CCSVBusSystem::StopByID(TStopID id) const noexcept {
    return DImplementation->StopByID(id);
}

std::shared_ptr<CBusSystem::SRoute> CCSVBusSystem::RouteByIndex(std::size_t index) const noexcept {
    return DImplementation->RouteByIndex(index);
}

std::shared_ptr<CBusSystem::SRoute> CCSVBusSystem::RouteByName(const std::string &name) const noexcept{
    return DImplementation->RouteByName(name);
}