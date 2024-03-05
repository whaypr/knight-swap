#ifndef KNIGHT_SWAP_SOLVER_H
#define KNIGHT_SWAP_SOLVER_H

#include <utility>
#include <algorithm>
#include "Types.h"
#include "BoardState.h"

using namespace std;

class Solver {
public:
    explicit Solver(const InstanceInfo & instanceInfo) :
        instanceInfo(instanceInfo) {
    }

    vector<pair<position,position>> solve(BoardState & boardState, int step) {
        if (boardState.whitesLeft + boardState.blacksLeft == 0) {
            return boardState.solutionCandidate;
        }

        // preparation for nextPos calls

        vector<NextCall> nextMoves;
        bool areWhitesOnTurn = ((step % 2 == 1) && (boardState.whitesLeft > 0)) || (boardState.blacksLeft == 0);
        const vector<position> & knights = areWhitesOnTurn ? boardState.whites : boardState.blacks;
        const map<position, int> & knightDistances = areWhitesOnTurn ? instanceInfo.minDistancesWhites : instanceInfo.minDistancesBlacks;

        for (int i = 0; i < knights.size(); ++i) {
            position current = knights[i];

            for (const position & next: instanceInfo.movesForPos.find(current)->second) {
                if (boardState.boardOccupation[next])
                    continue;

                // calculate steps lower bound
                size_t nextLowerBound = boardState.lowerBound - knightDistances.find(current)->second + knightDistances.find(next)->second;
                if (step + nextLowerBound + 1 >= boardState.upperBound) {
                    continue;
                }

                nextMoves.emplace_back(nextLowerBound, i, current, next);
            }
        }

        // call viable options
        sort(nextMoves.begin(), nextMoves.end(), nextCallComparator);
        for (const auto & item : nextMoves) {
            position current = item.currentPos;
            position next = item.nextPos;
            int i = item.knightIndex;
            int nextLowerBound = item.nextLowerBound;

                 BoardState newBoardState(boardState);
            if (instanceInfo.squareType[current] == WHITE && !areWhitesOnTurn)
                newBoardState.blacksLeft++;
            else if (instanceInfo.squareType[current] == BLACK && areWhitesOnTurn)
                newBoardState.whitesLeft++;
            if (instanceInfo.squareType[next] == WHITE && !areWhitesOnTurn)
                newBoardState.blacksLeft--;
            else if (instanceInfo.squareType[next] == BLACK && areWhitesOnTurn)
                newBoardState.whitesLeft--;

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
                if (res.size() == boardState.lowerBound) {
                    return res;
                }

                //boardState.solutionCandidate = res;
                boardState.solution = res;
                boardState.upperBound = res.size();
            }
        }

        return boardState.solution;
    }

private:
    const InstanceInfo & instanceInfo;

    struct NextCall {
        NextCall(int nextLowerBound, int knightIndex, position currentPos, position nextPos) :
                nextLowerBound(nextLowerBound), knightIndex(knightIndex), currentPos(currentPos), nextPos(nextPos) {
        }

        int nextLowerBound, knightIndex, currentPos, nextPos;
    };

    static bool nextCallComparator(const NextCall &a, const NextCall &b) {
        return a.nextLowerBound < b.nextLowerBound;
    }
};

#endif //KNIGHT_SWAP_SOLVER_H
