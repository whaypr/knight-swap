#ifndef KNIGHT_SWAP_INSTANCEINFO_H
#define KNIGHT_SWAP_INSTANCEINFO_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include "InputData.h"
#include "Types.h"

using namespace std;

/***
 * Instance information based on the input data
 */
class InstanceInfo {
public:
    explicit InstanceInfo(const InputData & inputData) :
            inputData(inputData),
            movesForPos(createMovesForPos()),
            nSquares(inputData.nCols * inputData.nRows),
            nKnightsInParty(inputData.nKnightsInParty),
            squareType(buildSquareType()),
            minDistancesWhites(calculateMinDistances(BLACK)),
            minDistancesBlacks(calculateMinDistances(WHITE))
    {
    }

    const InputData & inputData;
    map<position,vector<position>> movesForPos;
    const int nSquares;
    const int nKnightsInParty;
    const vector<SquareType> squareType;
    const map<position, int> minDistancesWhites, minDistancesBlacks;

private:

    // helpers for init only

    vector<SquareType> buildSquareType() const {
        vector<SquareType> res;

        for (int row = 0; row < inputData.nRows; ++row) {
            for (int col = 0; col < inputData.nCols; ++col) {
                if (row >= inputData.whiteArea1_row && row <= inputData.whiteArea2_row &&
                    col >= inputData.whiteArea1_col && col <= inputData.whiteArea2_col) {
                    res.push_back(WHITE);
                } else if (row >= inputData.blackArea1_row && row <= inputData.blackArea2_row &&
                           col >= inputData.blackArea1_col && col <= inputData.blackArea2_col) {
                    res.push_back(BLACK);
                } else {
                    res.push_back(BASIC);
                }
            }
        }

        return res;
    }

    map<position, int> calculateMinDistances(const SquareType & color) const {
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

                for (const position & next : movesForPos.find(current)->second) {
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

    position flatten(int row, int col) const {
        return row * inputData.nCols + col;
    }

    map<position,vector<position>> createMovesForPos() const {
        map<position,vector<position>> res;

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
                vector<position> jumpDestinations;

                for (const auto& pattern : patterns) {
                    int rowNew = row + pattern.first;
                    int colNew = col + pattern.second;

                    if (colNew >= 0 && colNew < inputData.nCols && rowNew >= 0 && rowNew < inputData.nRows) {
                        jumpDestinations.emplace_back(flatten(rowNew, colNew));
                    }
                }

                res.emplace(flatten(row, col), std::move(jumpDestinations));
            }
        }

        return res;
    }
};

#endif //KNIGHT_SWAP_INSTANCEINFO_H
