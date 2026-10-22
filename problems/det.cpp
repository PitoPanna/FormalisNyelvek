#include "det.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace {
using State = std::string;
using Symbol = std::string;
using StateSet = std::set<State>;
using TransitionKey = std::pair<State, Symbol>;
using Transitions = std::map<TransitionKey, StateSet>;

std::vector<std::string> splitWords(const std::string &line) {
    std::istringstream stream(line);
    std::vector<std::string> words;
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}
}

void DetProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("det", "Determinize the automaton");
}

bool DetProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("det") > 0;
}

int DetProblem::run(const cxxopts::ParseResult &args) {
    const std::string inputFilename = args["input"].as<std::string>();
    const std::string outputFilename = args["output"].as<std::string>();

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
        std::cerr << "Invalid automaton input: expected at least four header lines" << std::endl;
        return 1;
    }

    const std::vector<State> states = splitWords(lines[0]);
    std::vector<Symbol> alphabet = splitWords(lines[1]);
    const std::vector<State> startStates = splitWords(lines[2]);
    const std::vector<State> acceptingStatesList = splitWords(lines[3]);
    const std::set<State> stateSet(states.begin(), states.end());
    const std::set<Symbol> alphabetSet(alphabet.begin(), alphabet.end());

    if (states.empty() || stateSet.size() != states.size()) {
        std::cerr << "Invalid automaton input: states must be non-empty and unique" << std::endl;
        return 1;
    }
    if (alphabetSet.size() != alphabet.size()) {
        std::cerr << "Invalid automaton input: alphabet symbols must be unique" << std::endl;
        return 1;
    }
    if (startStates.size() != 1 || stateSet.count(startStates.front()) == 0) {
        std::cerr << "Invalid automaton input: start state must be listed in the states line"
                  << std::endl;
        return 1;
    }

    const State startState = startStates.front();
    const StateSet acceptingStates(
        acceptingStatesList.begin(),
        acceptingStatesList.end()
    );
    for (const State &state : acceptingStates) {
        if (stateSet.count(state) == 0) {
            std::cerr << "Invalid automaton input: accepting state '" << state
                      << "' is not listed in the states line" << std::endl;
            return 1;
        }
    }

    Transitions transitions;
    for (size_t i = 4; i < lines.size(); ++i) {
        const std::vector<std::string> fields = splitWords(lines[i]);
        if (fields.empty()) {
            continue;
        }
        if (fields.size() != 3) {
            std::cerr << "Invalid transition on input line " << (i + 1) << std::endl;
            return 1;
        }
        if (stateSet.count(fields[0]) == 0 || stateSet.count(fields[2]) == 0) {
            std::cerr << "Invalid transition on input line " << (i + 1)
                      << ": state is not listed in the states line" << std::endl;
            return 1;
        }
        if (alphabetSet.count(fields[1]) == 0) {
            std::cerr << "Invalid transition on input line " << (i + 1)
                      << ": symbol is not listed in the alphabet line" << std::endl;
            return 1;
        }
        transitions[{fields[0], fields[1]}].insert(fields[2]);
    }

    std::sort(alphabet.begin(), alphabet.end());

    std::vector<StateSet> deterministicStates = {{startState}};
    std::map<StateSet, size_t> stateIndices;
    stateIndices.emplace(deterministicStates.front(), 0);
    std::vector<std::tuple<size_t, Symbol, size_t>> deterministicTransitions;

    for (size_t i = 0; i < deterministicStates.size(); ++i) {
        for (const Symbol &symbol : alphabet) {
            StateSet destinations;
            for (const State &state : deterministicStates[i]) {
                const auto transition = transitions.find({state, symbol});
                if (transition != transitions.end()) {
                    destinations.insert(
                        transition->second.begin(),
                        transition->second.end()
                    );
                }
            }
            if (destinations.empty()) {
                continue;
            }

            auto destinationIndex = stateIndices.find(destinations);
            if (destinationIndex == stateIndices.end()) {
                const size_t newIndex = deterministicStates.size();
                deterministicStates.push_back(destinations);
                destinationIndex = stateIndices.emplace(destinations, newIndex).first;
            }
            deterministicTransitions.emplace_back(i, symbol, destinationIndex->second);
        }
    }

    std::ofstream outputFile(outputFilename);
    if (!outputFile) {
        std::cerr << "Error opening output file: " << outputFilename << std::endl;
        return 1;
    }

    for (size_t i = 0; i < deterministicStates.size(); ++i) {
        if (i > 0) {
            outputFile << ' ';
        }
        outputFile << 's' << i;
    }
    outputFile << '\n';

    for (size_t i = 0; i < alphabet.size(); ++i) {
        if (i > 0) {
            outputFile << ' ';
        }
        outputFile << alphabet[i];
    }
    outputFile << "\ns0\n";

    bool firstAcceptingState = true;
    for (size_t i = 0; i < deterministicStates.size(); ++i) {
        const bool accepting = std::any_of(
            deterministicStates[i].begin(),
            deterministicStates[i].end(),
            [&acceptingStates](const State &state) {
                return acceptingStates.count(state) > 0;
            }
        );
        if (accepting) {
            if (!firstAcceptingState) {
                outputFile << ' ';
            }
            outputFile << 's' << i;
            firstAcceptingState = false;
        }
    }
    outputFile << '\n';

    for (const auto &transition : deterministicTransitions) {
        outputFile << 's' << std::get<0>(transition) << ' '
                   << std::get<1>(transition) << ' '
                   << 's' << std::get<2>(transition) << '\n';
    }

    if (!outputFile) {
        std::cerr << "Error writing output file: " << outputFilename << std::endl;
        return 1;
    }
    return 0;
}
