#ifndef KNIGHT_SWAP_BOARDSTATEBUILDER_H
#define KNIGHT_SWAP_BOARDSTATEBUILDER_H

#include <queue>
#include "InstanceInfo.h"
#include "BoardState.h"

/***
 * Current state of the game board in given step
 */
class BoardStateBuilder {
public:
    BoardState build() {
        return BoardState(
                whitesLeft, blacksLeft,
                whites, blacks, boardOccupation,
                lowerBound,
                solutionCandidate
        );
    }

    explicit BoardStateBuilder(const InstanceInfo & instanceInfo) :
            instanceInfo(instanceInfo),
            whitesLeft(instanceInfo.nKnightsInParty),
            blacksLeft(instanceInfo.nKnightsInParty),
            solutionCandidate(vector<pair<position,position>>()) {

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
    }

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
    size_t lowerBound;
    /**
     * Vector of pairs where the first item is a starting point and the second item is an ending point of given move
     */
    vector<pair<position,position>> solutionCandidate;

private:

    const InstanceInfo & instanceInfo;

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
};

#endif //KNIGHT_SWAP_BOARDSTATEBUILDER_H
