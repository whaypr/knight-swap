#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <map>
#include <queue>
#include <set>

using namespace std;


typedef int position;

/***
 * Input information loaded from a file
 */
struct InputData {
    explicit InputData(const string& path) {
        // read data
        ifstream instanceFile(path);
        instanceFile
            >> nCols >> nRows >> nKnightsInParty >> nKnightsInParty
            >> whiteArea1_col >> whiteArea1_row >> whiteArea2_col >> whiteArea2_row
            >> blackArea1_col >> blackArea1_row >> blackArea2_col >> blackArea2_row;

        // canonize white area
        int tmp;
        if (whiteArea2_row < whiteArea1_row) {
            tmp = whiteArea2_row;
            whiteArea2_row = whiteArea1_row;
            whiteArea1_row = tmp;
        }
        if (whiteArea2_col < whiteArea1_col) {
            tmp = whiteArea2_col;
            whiteArea2_col = whiteArea1_col;
            whiteArea1_col = tmp;
        }

        // canonize black area
        if (blackArea2_row < blackArea1_row) {
            tmp = blackArea2_row;
            blackArea2_row = blackArea1_row;
            blackArea1_row = tmp;
        }
        if (blackArea2_col < blackArea1_col) {
            tmp = blackArea2_col;
            blackArea2_col = blackArea1_col;
            blackArea1_col = tmp;
        }
    }

    int
        nRows{}, nCols{}, nKnightsInParty{},
        whiteArea1_row{}, whiteArea1_col{}, whiteArea2_row{}, whiteArea2_col{},
        blackArea1_row{}, blackArea1_col{}, blackArea2_row{}, blackArea2_col{};
};


const InputData inputData("../test/instances/in_0001.txt");


/**
 * Helpers for solving the problem
 */
struct Helpers {
    explicit Helpers() :
        nCols(inputData.nCols),
        movesForPos(createMovesForPos()){
    }

    int nCols;
    map<position,vector<int>> movesForPos;

    enum SquareType {
        BASIC,
        WHITE,
        BLACK
    };

    position flatten(int row, int col) const {
        return row * nCols + col;
    }

private:

    map<position,vector<int>> createMovesForPos() const {
        map<position,vector<int>> res;

        vector<pair<int,int>> patterns = {
            make_pair(-2,-1),
            make_pair(-2, 1),
            make_pair(-1, -2),
            make_pair(-1, 2),
            make_pair(1, -2),
            make_pair(1, 2),
            make_pair(2, -1),
            make_pair(2, 1)
        };

        for (int row = 0; row < inputData.nRows; ++row) {
            for (int col = 0; col < inputData.nCols; ++col) {
                vector<int> moves;

                for (const auto& pattern : patterns) {
                    int rowNew = row + pattern.first;
                    int colNew = col + pattern.second;

                    if (colNew >= 0 && colNew < inputData.nCols && rowNew >= 0 && rowNew < inputData.nRows) {
                        moves.emplace_back(flatten(rowNew, colNew));
                    }
                }

                res.emplace(flatten(row, col), std::move(moves));
            }
        }

        return res;
    }
};


const Helpers helpers;


/***
 * Instance information based on the input data
 */
class InstanceInfo {
public:
    explicit InstanceInfo() :
            nSquares(inputData.nCols * inputData.nRows),
            nKnightsInParty(inputData.nKnightsInParty),
            squareType(buildSquareType()),
            minDistancesWhites(calculateMinDistances(Helpers::BLACK)),
            minDistancesBlacks(calculateMinDistances(Helpers::WHITE))
    {
    }

    const int nSquares;
    const int nKnightsInParty;
    const vector<Helpers::SquareType> squareType;
    const map<position, int> minDistancesWhites, minDistancesBlacks;

private:

    // helpers for init only

    static vector<Helpers::SquareType> buildSquareType() {
        vector<Helpers::SquareType> res;

        for (int row = 0; row < inputData.nRows; ++row) {
            for (int col = 0; col < inputData.nCols; ++col) {
                if (row >= inputData.whiteArea1_row && row <= inputData.whiteArea2_row &&
                    col >= inputData.whiteArea1_col && col <= inputData.whiteArea2_col) {
                    res.push_back(Helpers::WHITE);
                } else if (row >= inputData.blackArea1_row && row <= inputData.blackArea2_row &&
                           col >= inputData.blackArea1_col && col <= inputData.blackArea2_col) {
                    res.push_back(Helpers::BLACK);
                } else {
                    res.push_back(Helpers::BASIC);
                }
            }
        }

        return res;
    }

    map<position, int> calculateMinDistances(const Helpers::SquareType & color) const {
        map<position, int> res;

        for (position pos = 0; pos < nSquares; ++pos) {
            // find the shortest path using BFS
            int shortest = 99999999;
            set<position> visited;
            visited.insert(pos);
            queue<pair<position, int>> q; // position, traveled so far from that pos
            q.emplace(pos, 0);
            while (!q.empty()) {
                position current = q.front().first;
                int length = q.front().second;
                q.pop();

                if (squareType[current] == color) {
                    shortest = length;
                    break;
                }

                for (const position & next : helpers.movesForPos.find(current)->second) {
                    if (visited.find(next) == visited.end()) {
                        q.emplace(next, length + 1);
                        visited.insert(next);
                    }
                }
            }

            res.insert({pos, shortest});
        }

        return res;
    }
};


const InstanceInfo instanceInfo;


/***
 * Current state of the board in given step
 */
class BoardState {
public:
    BoardState() :
            whitesLeft(instanceInfo.nKnightsInParty),
            blacksLeft(instanceInfo.nKnightsInParty),
            solutionCandidate(vector<pair<position,position>>()),
            solution(vector<pair<position,position>>()) {
        for (position pos = 0; pos < instanceInfo.nSquares; ++pos) {
            Helpers::SquareType type = instanceInfo.squareType[pos];
            switch (type) {
                case Helpers::WHITE:
                    whites.emplace_back(pos);
                    boardOccupation.emplace_back(true);
                    break;
                case Helpers::BLACK:
                    blacks.emplace_back(pos);
                    boardOccupation.emplace_back(true);
                    break;
                case Helpers::BASIC:
                    boardOccupation.emplace_back(false);
                    break;
            }
        }

        lowerBound = getInitLowerBound();
        upperBound = getInitUpperBound();
    }

    int whitesLeft, blacksLeft;
    vector<position> whites, blacks;
    vector<bool> boardOccupation;
    size_t lowerBound, upperBound;
    vector<pair<position,position>> solutionCandidate, solution;

    BoardState(const BoardState & o) = default;

private:

    int getInitLowerBound() {
        int res = 0;
        for (auto const & w : whites)
            res += instanceInfo.minDistancesWhites.find(w)->second;
        for (auto const & b : blacks)
            res += instanceInfo.minDistancesBlacks.find(b)->second;
        return res;
    }

    int getInitUpperBound() {
        int res = 0;

        for (const auto& opt : {make_pair(whites, Helpers::BLACK), make_pair(blacks, Helpers::WHITE)}) {
            for (const int& pos: opt.first) {
                // find the shortest path (to the most distant destination) using DFS
                int mostDistantDestPathLen;
                queue<pair<position, pair<int, int>>> q; // position, traveled so far from that pos, number of destination squares visited
                q.emplace(pos, make_pair(0, 0));
                while (true) {
                    position current = q.front().first;
                    int length = q.front().second.first;
                    int destVisited = q.front().second.second;
                    q.pop();

                    if (instanceInfo.squareType[current] == opt.second) {
                        destVisited++;
                    }
                    if (destVisited == instanceInfo.nKnightsInParty) {
                        mostDistantDestPathLen = length;
                        break;
                    }

                    for (const position &next: helpers.movesForPos.find(current)->second) {
                        q.emplace(next, make_pair(length + 1, destVisited));
                    }
                }

                res += mostDistantDestPathLen;
            }
        }

        return res;
    }
};

struct NextCall {
    NextCall(int knightIndex, position currentPos, position nextPos) :
    knightIndex(knightIndex), currentPos(currentPos), nextPos(nextPos) {
    }

    int knightIndex, currentPos, nextPos;
};


/***
 * Solves the problem
 */
vector<pair<position,position>> solve(BoardState & boardState, int step) {
    if (boardState.whitesLeft + boardState.blacksLeft == 0) {
        return boardState.solutionCandidate;
    }

    // preparation for nextPos calls

    map<int, NextCall> nextMoves;
    bool areWhitesOnTurn = (step % 2 == 1) && boardState.whitesLeft > 0;
    const vector<position> & knights = areWhitesOnTurn ? boardState.whites : boardState.blacks;
    const map<position, int> & knightDistances = areWhitesOnTurn ? instanceInfo.minDistancesWhites : instanceInfo.minDistancesBlacks;

    for (int i = 0; i < knights.size(); ++i) {
        position current = knights[i];

        for (const position & next: helpers.movesForPos.find(current)->second) {
            if (boardState.boardOccupation[next])
                continue;

            // calculate steps lower bound
            size_t nextLowerBound = boardState.lowerBound - knightDistances.find(current)->second + knightDistances.find(next)->second;
            if (step + nextLowerBound + 1 >= boardState.upperBound) {
                continue;
            }

            nextMoves.insert({nextLowerBound, NextCall(i, current, next)});
        }
    }

    // call viable options
    for (const auto & item : nextMoves) {
        position current = item.second.currentPos;
        position next = item.second.nextPos;
        int i = item.second.knightIndex;
        int nextLowerBound = item.first;

        BoardState newBoardState(boardState);
        if (instanceInfo.squareType[current] == Helpers::WHITE)
            newBoardState.whitesLeft++;
        else if (instanceInfo.squareType[current] == Helpers::BLACK)
            newBoardState.blacksLeft++;
        if (instanceInfo.squareType[next] == Helpers::WHITE)
            newBoardState.whitesLeft--;
        else if (instanceInfo.squareType[next] == Helpers::BLACK)
            newBoardState.blacksLeft--;

        newBoardState.lowerBound = nextLowerBound;
        newBoardState.boardOccupation[current] = false;
        newBoardState.boardOccupation[next] = true;
        if (areWhitesOnTurn)
            newBoardState.whites[i] = next;
        else
            newBoardState.blacks[i] = next;
        newBoardState.solutionCandidate.emplace_back(current, next);

        auto res = solve(newBoardState, step+1);

        if (!res.empty() && res.size() < boardState.upperBound) {
            if (res.size() == boardState.lowerBound)
                return res;

            boardState.solutionCandidate = res;
            boardState.solution = res;
            boardState.upperBound = res.size();
        }
    }

    return boardState.solution;
}


/* ------------------------------------------------------------------------------------------ */


int main() {
    BoardState boardState;
    auto res = solve(boardState, 0);

    return 0;
}
