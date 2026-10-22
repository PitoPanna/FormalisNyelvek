#include "dfa.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cctype>

namespace {
void trim(std::string &value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
}
}

void DFAProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("check", "Comma-separated words to check", cxxopts::value<std::string>());
}

bool DFAProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("check") > 0;
}

int DFAProblem::run(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();

    std::ifstream inputFile(inputFilename);

    if (!inputFile) {
        std::cerr << "Error opening input file: " << inputFilename << std::endl;
        return 1;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(inputFile, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }

    if (lines.size() < 4) {
        std::cerr << "Invalid DFA input: expected at least four header lines" << std::endl;
        return 1;
    }

    std::string startState;
    std::istringstream startStateLine(lines[2]);
    if (!(startStateLine >> startState)) {
        std::cerr << "Invalid DFA input: missing start state" << std::endl;
        return 1;
    }

    std::istringstream ssAccept(lines[3]);
    std::set<std::string> acceptStates;
    std::string state;
    while (ssAccept >> state) {
        acceptStates.insert(state);
    }

    std::map<std::pair<std::string, char>, std::string> transitions;
    std::string src, dst;
    char symbol;
    for (size_t i = 4; i < lines.size(); ++i) {
        std::istringstream transitionLine(lines[i]);
        if (!(transitionLine >> src)) {
            continue;
        }
        if (!(transitionLine >> symbol >> dst)) {
            std::cerr << "Invalid DFA transition on input line " << (i + 1) << std::endl;
            return 1;
        }
        transitions[{src, symbol}] = dst;
    }

    const std::string checkWords = args["check"].as<std::string>();
    std::vector<std::string> words;
    size_t wordStart = 0;
    while (true) {
        const size_t separator = checkWords.find(',', wordStart);
        std::string word = checkWords.substr(
            wordStart,
            separator == std::string::npos ? std::string::npos : separator - wordStart
        );
        trim(word);
        words.push_back(word);
        if (separator == std::string::npos) {
            break;
        }
        wordStart = separator + 1;
    }

    std::ofstream outputFile(outputFilename);
    if (!outputFile) {
        std::cerr << "Error opening output file: " << outputFilename << std::endl;
        return 1;
    }

    for (const std::string &word : words) {
        std::string currentState = startState;
        bool valid = true;

        for (char c : word) {
            auto it = transitions.find({currentState, c});
            if (it == transitions.end()) {
                valid = false;
                break;
            }
            currentState = it->second;
        }

        if (valid && acceptStates.count(currentState) > 0) {
            outputFile << "IGEN\n";
        } else {
            outputFile << "NEM\n";
        }
    }

    if (!outputFile) {
        std::cerr << "Error writing output file: " << outputFilename << std::endl;
        return 1;
    }
    return 0;
}