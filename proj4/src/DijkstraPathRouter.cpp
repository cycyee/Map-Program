#include "DijkstraPathRouter.h"
#include<vector>
#include<algorithm>
#include<queue>
#include<functional>
#include <iostream>
//algorithm sorting functions, using prebuilt min heap + pop_heap fn
struct CDijkstraPathRouter::SImplementation {
    using TEdge = std::pair<double, TVertexID>; 
    struct SVertex {
        std::any DTag;
        std::vector <TEdge> DEdges;
    };

    std::vector <SVertex> DVertices; 

    std::size_t VertexCount() const noexcept{
        return DVertices.size();
    }
    TVertexID AddVertex(std::any tag) noexcept {
        TVertexID NewVertexID = DVertices.size();//push vertex to DVertices
        DVertices.push_back({tag, {}});
        return NewVertexID;
    }
    std::any GetVertexTag(TVertexID id) const noexcept {
        if(id < DVertices.size()) {//vertex tag in range
            return DVertices[id].DTag;
        }
        return std::any();//tag of any type
    }
    bool AddEdge(TVertexID src, TVertexID dest, double weight, bool bidir) noexcept {
        if((src < DVertices.size()) && (dest < DVertices.size()))  { //check for negative edge weight, check if IDs are in range of DVertices
            //std::cout<<"edge added: "<<weight<<" "<<src<< " to "<<dest <<std::endl;
            DVertices[src].DEdges.push_back(std::make_pair(weight, dest));//opposite direction edge
            if(bidir == true) { //add directional edge in oppposite dir
                DVertices[dest].DEdges.push_back(std::make_pair(weight, src));
                //std::cout<<"edge added: "<<weight<<" "<<dest<< " to "<<src<<std::endl;
            }
            return true;
        }
        std::cout<<"edge failed"<<std::endl;
        return false;
    }
    bool Precompute(std::chrono::steady_clock::time_point deadline) noexcept {
        return true;
    }

    double FindShortestPath(TVertexID src, TVertexID dest, std::vector<TVertexID> &path) noexcept {
        path.clear();
        if(DVertices.empty() || src >= DVertices.size() || dest >= DVertices.size()) {
            return NoPathExists;
        }

        std::vector<double> Distances(DVertices.size(), CPathRouter::NoPathExists);
        std::vector<TVertexID> Previous(DVertices.size(), CPathRouter::InvalidVertexID);

        // Proper min-heap keyed by distance, with lazy deletion of stale entries.
        // Using a priority_queue keeps this O((V + E) log V) instead of the
        // previous O(V^2) make-heap-per-iteration, and avoids the inconsistent
        // heap ordering that could produce cyclic Previous pointers.
        using SQueueEntry = std::pair<double, TVertexID>;
        // Min-heap by distance; on equal distances prefer the higher vertex id,
        // which keeps tie-breaking deterministic and consistent with the tests.
        auto Cmp = [](const SQueueEntry &a, const SQueueEntry &b) {
            if(a.first != b.first) return a.first > b.first;
            return a.second < b.second;
        };
        std::priority_queue<SQueueEntry, std::vector<SQueueEntry>, decltype(Cmp)> Queue(Cmp);

        Distances[src] = 0.0;
        Queue.push({0.0, src});

        while(!Queue.empty()) {
            auto [dist, current] = Queue.top();
            Queue.pop();

            if(current == dest) {
                break;
            }
            if(dist > Distances[current]) {
                continue; // stale entry, a shorter path to current was already processed
            }

            for(const auto &edge : DVertices[current].DEdges) {
                double weight = edge.first;
                TVertexID next = edge.second;
                double total = Distances[current] + weight;
                if(total < Distances[next]) {
                    Distances[next] = total;
                    Previous[next] = current;
                    Queue.push({total, next});
                }
            }
        }

        if(CPathRouter::NoPathExists == Distances[dest]) {
            return CPathRouter::NoPathExists;
        }

        // Reconstruct the path from dest back to src, guarded against malformed
        // Previous chains so a bad link can never spin into an unbounded loop.
        double PathDistance = Distances[dest];
        TVertexID node = dest;
        while(node != src) {
            path.push_back(node);
            TVertexID prev = Previous[node];
            if(prev == CPathRouter::InvalidVertexID || path.size() > DVertices.size()) {
                path.clear();
                return CPathRouter::NoPathExists;
            }
            node = prev;
        }
        path.push_back(src);
        std::reverse(path.begin(), path.end());
        return PathDistance;
    }
};

//defer all implementations
CDijkstraPathRouter::CDijkstraPathRouter() {
    DImplementation = std::make_unique<SImplementation>();
}
CDijkstraPathRouter::~CDijkstraPathRouter() {

}

std::size_t CDijkstraPathRouter::VertexCount() const noexcept {
    return DImplementation->VertexCount();
}

CDijkstraPathRouter::TVertexID CDijkstraPathRouter::AddVertex(std::any tag) noexcept {
    return DImplementation->AddVertex(tag);
}
std::any CDijkstraPathRouter::GetVertexTag(TVertexID id) const noexcept {
    return DImplementation->GetVertexTag(id);
}

bool CDijkstraPathRouter::AddEdge(TVertexID src, TVertexID dest, double weight, bool bidir) noexcept {
    return DImplementation->AddEdge(src, dest, weight, bidir);
}

bool CDijkstraPathRouter::Precompute(std::chrono::steady_clock::time_point deadline) noexcept {
    return DImplementation->Precompute(deadline);
}

double CDijkstraPathRouter::FindShortestPath(TVertexID src, TVertexID dest, std::vector<TVertexID> &path) noexcept {
    return DImplementation->FindShortestPath(src, dest, path);
}