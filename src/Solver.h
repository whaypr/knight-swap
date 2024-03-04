#ifndef KNIGHT_SWAP_SOLVER_H
#define KNIGHT_SWAP_SOLVER_H

#include <utility>
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

        map<int, NextCall> nextMoves;
        bool areWhitesOnTurn = (step % 2 == 1) && boardState.whitesLeft > 0;
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
            if (instanceInfo.squareType[current] == WHITE)
                newBoardState.whitesLeft++;
            else if (instanceInfo.squareType[current] == BLACK)
                newBoardState.blacksLeft++;
            if (instanceInfo.squareType[next] == WHITE)
                newBoardState.whitesLeft--;
            else if (instanceInfo.squareType[next] == BLACK)
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

private:
    const InstanceInfo & instanceInfo;

    struct NextCall {
        NextCall(int knightIndex, position currentPos, position nextPos) :
                knightIndex(knightIndex), currentPos(currentPos), nextPos(nextPos) {
        }

        int knightIndex, currentPos, nextPos;
    };
};

#endif //KNIGHT_SWAP_SOLVER_H
