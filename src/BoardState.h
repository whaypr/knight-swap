#ifndef KNIGHT_SWAP_BOARDSTATE_H
#define KNIGHT_SWAP_BOARDSTATE_H

#include <queue>
#include "InstanceInfo.h"

using namespace std;

/***
 * Current state of the game board in given step
 */
class BoardState {
public:
    explicit BoardState(const InstanceInfo & instanceInfo) :
            instanceInfo(instanceInfo),
            whitesLeft(instanceInfo.nKnightsInParty),
            blacksLeft(instanceInfo.nKnightsInParty),
            solutionCandidate(vector<pair<position,position>>()),
            solution(vector<pair<position,position>>()) {

        for (position pos = 0; pos < instanceInfo.nSquares; ++pos) {
            SquareType type = instanceInfo.squareType[pos];
            switch (type) {
                case WHITE:
                    whites.emplace_back(pos);
                    boardOccupation.emplace_back(true);
                    break;
                case BLACK:
                    blacks.emplace_back(pos);
                    boardOccupation.emplace_back(true);
                    break;
                case BASIC:
                    boardOccupation.emplace_back(false);
                    break;
            }
        }

        lowerBound = getInitLowerBound();
        upperBound = getInitUpperBound();
    }

    BoardState(const BoardState & o) = default;

    const InstanceInfo & instanceInfo;
    /**
     * How many knights of given color are still not in a destination area
     */
    int whitesLeft, blacksLeft;
    /**
     * Positions of knights of given color
     */
    vector<position> whites, blacks;
    /**
     * For each position on the game board, it says whether it is occupied or not
     */
    vector<bool> boardOccupation;
    size_t lowerBound, upperBound;
    /**
     * Vector of pairs where the first item is a starting point and the second item is an ending point of given move
     */
    vector<pair<position,position>> solutionCandidate, solution;

private:

    /**
     * A sum of minimal distances to destination areas of all knights
     */
    int getInitLowerBound() {
        int res = 0;
        for (auto const & w : whites)
            res += instanceInfo.minDistancesWhites.find(w)->second;
        for (auto const & b : blacks)
            res += instanceInfo.minDistancesBlacks.find(b)->second;
        return res;
    }

    /**
     * A sum of minimal distances to the most distant squares in destination areas of all knights
     */
    int getInitUpperBound() {
        int res = 0;

        // For each position, find the shortest path to the most distant square in the destination area for given color using BFS
        for (const auto& opt : {make_pair(whites, BLACK), make_pair(blacks, WHITE)}) {
            for (const int& pos: opt.first) {
                vector<bool> visited(instanceInfo.nKnightsInParty, false);
                int mostDistantDestPathLen = 0;

                // first: position
                // second.first: number of steps traveled so far from that pos
                queue<pair<position, int>> q;
                q.emplace(pos, 0);
                visited[pos] = true;

                while (!q.empty()) {
                    position current = q.front().first;
                    int length = q.front().second;
                    q.pop();

                    if (instanceInfo.squareType[current] == opt.second) {
                        mostDistantDestPathLen = length;
                    }

                    for (const position &next: instanceInfo.movesForPos.find(current)->second) {
                        if (!visited[next]) {
                            visited[next] = true;
                            q.emplace(next, length + 1);
                        }
                    }
                }

                res += mostDistantDestPathLen;
            }
        }

        return res+1;
    }
};

#endif //KNIGHT_SWAP_BOARDSTATE_H
