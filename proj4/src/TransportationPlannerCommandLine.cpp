#include "TransportationPlannerCommandLine.h"
#include <sstream>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <iomanip>
#include "GeographicUtils.h"

struct CTransportationPlannerCommandLine::SImplementation {
    std::shared_ptr<CDataSource> CmdSrc;
    std::shared_ptr<CDataSink> OutSink;
    std::shared_ptr<CDataSink> ErrSink;
    std::shared_ptr<CDataFactory> Results;
    std::shared_ptr<CTransportationPlanner> Planner;

    // The last calculated path, retained so "save" and "print" can act on it.
    enum class ELastPath { None, Shortest, Fastest };
    ELastPath DLastType = ELastPath::None;
    CTransportationPlanner::TNodeID DLastSrc = 0;
    CTransportationPlanner::TNodeID DLastDest = 0;
    double DLastMetric = 0.0; // miles for shortest, hours for fastest
    std::vector<CTransportationPlanner::TNodeID> DLastShortestPath;
    std::vector<CTransportationPlanner::TTripStep> DLastFastestPath;

    SImplementation(std::shared_ptr<CDataSource> cmdsrc, std::shared_ptr<CDataSink> outsink,
                    std::shared_ptr<CDataSink> errsink, std::shared_ptr<CDataFactory> results,
                    std::shared_ptr<CTransportationPlanner> planner)
        : CmdSrc(std::move(cmdsrc)), OutSink(std::move(outsink)), ErrSink(std::move(errsink)),
          Results(std::move(results)), Planner(std::move(planner)) {}

    void Write(const std::shared_ptr<CDataSink> &sink, const std::string &str) {
        std::vector<char> buf(str.begin(), str.end());
        sink->Write(buf);
    }

    // Read one line (up to '\n') from the command source. Returns false only on a
    // clean end-of-input with nothing read, which ends the command loop.
    bool ReadLine(std::string &line) {
        line.clear();
        char ch;
        bool any = false;
        while (CmdSrc->Get(ch)) {
            any = true;
            if (ch == '\n') return true;
            line += ch;
        }
        return any;
    }

    static std::vector<std::string> Tokenize(const std::string &line) {
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string tok;
        while (iss >> tok) tokens.push_back(tok);
        return tokens;
    }

    static std::string HelpText() {
        return "------------------------------------------------------------------------\n"
               "help     Display this help menu\n"
               "exit     Exit the program\n"
               "count    Output the number of nodes in the map\n"
               "node     Syntax \"node [0, count)\" \n"
               "         Will output node ID and Lat/Lon for node\n"
               "fastest  Syntax \"fastest start end\" \n"
               "         Calculates the time for fastest path from start to end\n"
               "shortest Syntax \"shortest start end\" \n"
               "         Calculates the distance for the shortest path from start to end\n"
               "save     Saves the last calculated path to file\n"
               "print    Prints the steps for the last calculated path\n";
    }

    // Format a duration in hours as "H hr M min S sec", omitting zero components.
    static std::string FormatTime(double hoursValue) {
        int hours = static_cast<int>(hoursValue);
        double remainder = (hoursValue - hours) * 60.0;
        int minutes = static_cast<int>(remainder);
        remainder = (remainder - minutes) * 60.0;
        int seconds = static_cast<int>(remainder);
        std::string out;
        if (hours > 0) out += " " + std::to_string(hours) + " hr";
        if (minutes > 0) out += " " + std::to_string(minutes) + " min";
        if (seconds > 0) out += " " + std::to_string(seconds) + " sec";
        if (out.empty()) out = " 0 sec";
        return out.substr(1);
    }

    static const char *ModeString(CTransportationPlanner::ETransportationMode mode) {
        switch (mode) {
            case CTransportationPlanner::ETransportationMode::Bike: return "Bike";
            case CTransportationPlanner::ETransportationMode::Bus:  return "Bus";
            default:                                                return "Walk";
        }
    }

    void HandleNode(const std::vector<std::string> &tokens) {
        if (tokens.size() < 2) {
            Write(ErrSink, "Invalid node command, see help.\n");
            return;
        }
        try {
            int index = std::stoi(tokens[1]);
            auto node = Planner->SortedNodeByIndex(index);
            if (!node) {
                Write(ErrSink, "Invalid node parameter, see help.\n");
                return;
            }
            Write(OutSink, "Node " + std::to_string(index) + ": id = " + std::to_string(node->ID()) +
                           " is at " + SGeographicUtils::ConvertLLToDMS(node->Location()) + "\n");
        } catch (...) {
            Write(ErrSink, "Invalid node parameter, see help.\n");
        }
    }

    void HandleShortest(const std::vector<std::string> &tokens) {
        if (tokens.size() < 3) {
            Write(ErrSink, "Invalid shortest command, see help.\n");
            return;
        }
        try {
            auto src = static_cast<CTransportationPlanner::TNodeID>(std::stoull(tokens[1]));
            auto dest = static_cast<CTransportationPlanner::TNodeID>(std::stoull(tokens[2]));
            std::vector<CTransportationPlanner::TNodeID> path;
            double distance = Planner->FindShortestPath(src, dest, path);
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << distance;
            Write(OutSink, "Shortest path is " + oss.str() + " mi.\n");
            DLastType = ELastPath::Shortest;
            DLastSrc = src;
            DLastDest = dest;
            DLastMetric = distance;
            DLastShortestPath = path;
        } catch (...) {
            Write(ErrSink, "Invalid shortest parameter, see help.\n");
        }
    }

    void HandleFastest(const std::vector<std::string> &tokens) {
        if (tokens.size() < 3) {
            Write(ErrSink, "Invalid fastest command, see help.\n");
            return;
        }
        try {
            auto src = static_cast<CTransportationPlanner::TNodeID>(std::stoull(tokens[1]));
            auto dest = static_cast<CTransportationPlanner::TNodeID>(std::stoull(tokens[2]));
            std::vector<CTransportationPlanner::TTripStep> steps;
            double time = Planner->FindFastestPath(src, dest, steps);
            Write(OutSink, "Fastest path takes " + FormatTime(time) + ".\n");
            DLastType = ELastPath::Fastest;
            DLastSrc = src;
            DLastDest = dest;
            DLastMetric = time;
            DLastFastestPath = steps;
        } catch (...) {
            Write(ErrSink, "Invalid fastest parameter, see help.\n");
        }
    }

    void HandlePrint() {
        if (DLastType == ELastPath::None) {
            Write(ErrSink, "No valid path to print, see help.\n");
            return;
        }
        if (DLastType == ELastPath::Fastest) {
            std::vector<std::string> desc;
            Planner->GetPathDescription(DLastFastestPath, desc);
            std::string out;
            for (const auto &line : desc) out += line + "\n";
            Write(OutSink, out);
        } else {
            std::string out;
            for (auto id : DLastShortestPath) out += "node " + std::to_string(id) + "\n";
            Write(OutSink, out);
        }
    }

    void HandleSave() {
        if (DLastType == ELastPath::None) {
            Write(ErrSink, "No valid path to save, see help.\n");
            return;
        }
        std::ostringstream fn;
        fn << DLastSrc << "_" << DLastDest << "_" << std::fixed << std::setprecision(6)
           << DLastMetric << "hr.csv";
        std::string filename = fn.str();

        std::string body = "mode,node_id";
        if (DLastType == ELastPath::Fastest) {
            for (const auto &step : DLastFastestPath) {
                body += "\n" + std::string(ModeString(step.first)) + "," + std::to_string(step.second);
            }
        } else {
            for (auto id : DLastShortestPath) {
                body += "\nWalk," + std::to_string(id);
            }
        }

        if (Results) {
            auto sink = Results->CreateSink(filename);
            if (sink) Write(sink, body);
        }
        Write(OutSink, "Path saved to <results>/" + filename + "\n");
    }

    bool ProcessCommands() {
        while (true) {
            Write(OutSink, "> ");
            std::string line;
            if (!ReadLine(line)) return true; // end of input

            auto tokens = Tokenize(line);
            if (tokens.empty()) continue;
            const std::string &cmd = tokens[0];

            if (cmd == "exit") {
                return true;
            } else if (cmd == "help") {
                Write(OutSink, HelpText());
            } else if (cmd == "count") {
                Write(OutSink, std::to_string(Planner->NodeCount()) + " nodes\n");
            } else if (cmd == "node") {
                HandleNode(tokens);
            } else if (cmd == "shortest") {
                HandleShortest(tokens);
            } else if (cmd == "fastest") {
                HandleFastest(tokens);
            } else if (cmd == "print") {
                HandlePrint();
            } else if (cmd == "save") {
                HandleSave();
            } else {
                Write(ErrSink, "Unknown command \"" + cmd + "\" type help for help.\n");
            }
        }
    }
};

CTransportationPlannerCommandLine::CTransportationPlannerCommandLine(std::shared_ptr<CDataSource> cmdsrc,
    std::shared_ptr<CDataSink> outsink, std::shared_ptr<CDataSink> errsink,
    std::shared_ptr<CDataFactory> results, std::shared_ptr<CTransportationPlanner> planner) {
    DImplementation = std::make_unique<SImplementation>(cmdsrc, outsink, errsink, results, planner);
}

CTransportationPlannerCommandLine::~CTransportationPlannerCommandLine() {
}

bool CTransportationPlannerCommandLine::ProcessCommands() {
    return DImplementation->ProcessCommands();
}
