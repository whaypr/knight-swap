#ifndef KNIGHT_SWAP_INSTANCEINFOBUILDER_H
#define KNIGHT_SWAP_INSTANCEINFOBUILDER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include "InputData.h"
#include "InstanceInfo.h"
#include "Types.h"

using namespace std;

/***
 * InstanceInfo builder which prepares static information about given instance based on the input datay
 */
class InstanceInfoBuilder {
public:
    explicit InstanceInfoBuilder(const InputData & inputData) :
            inputData(inputData),
            movesForPos(createMovesForPos()),
            nSquares(inputData.nCols * inputData.nRows),
            nKnightsInParty(inputData.nKnightsInParty),
            squareType(buildSquareType()),
            minDistancesWhites(calculateMinDistances(BLACK)),
            minDistancesBlacks(calculateMinDistances(WHITE))
    {
    }

    InstanceInfo build() {
        return InstanceInfo(
            movesForPos,
            nSquares,
            nKnightsInParty,
            squareType,
            minDistancesWhites,
            minDistancesBlacks
        );
    }

private:

    const InputData & inputData;
    /**
     * For each position on the game board, it says all the possible destinations a knight can goes to from that position
     */
    map<position,vector<position>> movesForPos;
    /**
     * Total amount of squares on the game board
     */
    const int nSquares;
    const int nKnightsInParty;
    /**
     * For each position on the game board, it says which type it is (black, white or basic)
     */
    const vector<SquareType> squareType;
    /**
     * For each position on the game board, it says the minimal distance to the destination area
     */
    const map<position, int> minDistancesWhites, minDistancesBlacks;

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

        // For each position, find the shortest path to the destination area for given color using BFS
        for (position pos = 0; pos < nSquares; ++pos) {
            int shortest = 99999999;
            set<position> visited;
            visited.insert(pos);
            queue<pair<position, int>> q; // first: position, second: number of steps traveled so far from that pos
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

    /**
     * Maps the 2D array coordinates into a 1D array index
     */
    position flatten(int row, int col) const {
        return row * inputData.nCols + col;
    }

    map<position,vector<position>> createMovesForPos() const {
        map<position,vector<position>> res;

        // all the possible pattern a knight can make
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

#endif //KNIGHT_SWAP_INSTANCEINFOBUILDER_H
