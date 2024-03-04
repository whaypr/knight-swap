#ifndef KNIGHT_SWAP_BOARDSTATE_H
#define KNIGHT_SWAP_BOARDSTATE_H

#include <queue>
#include "InstanceInfo.h"

using namespace std;

/***
 * Current state of the board in given step
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

    const InstanceInfo & instanceInfo;
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

        for (const auto& opt : {make_pair(whites, BLACK), make_pair(blacks, WHITE)}) {
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

                    for (const position &next: instanceInfo.movesForPos.find(current)->second) {
                        q.emplace(next, make_pair(length + 1, destVisited));
                    }
                }

                res += mostDistantDestPathLen;
            }
        }

        return res;
    }
};

#endif //KNIGHT_SWAP_BOARDSTATE_H
