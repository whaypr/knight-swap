#ifndef KNIGHT_SWAP_SOLVERSLAVE_H
#define KNIGHT_SWAP_SOLVERSLAVE_H

#include <algorithm>
#include <iostream>
#include <omp.h>
#include <mpi.h>
#include "Types.h"
#include "BoardState.h"

using namespace std;

/**
 * Used to finding and printing a solution for given problem instance
 */
class SolverSlave {
public:
    explicit SolverSlave(const InstanceInfo & instanceInfo, size_t initLowerBound, size_t upperBound, int rank) :
        instanceInfo(instanceInfo),
        initLowerBound(initLowerBound),
        upperBound(upperBound),
        rank(rank){
    }

    /**
     * Finds a solution and stores it internally
     */
    void solve(BoardState & boardState, int step) {
        #pragma omp parallel
        {
            #pragma omp single
            solveInner(boardState, step);
        }

        // if found, send solution to the master
        if (!solution.empty()) {
            vector<int> buffer;
            buffer.push_back(solution.size());
            for (const auto& item : solution) {
                buffer.push_back(item.first);
                buffer.push_back(item.second);
            }
            MPI_Send(buffer.data(), (int)buffer.size(), MPI_INT, 0, TAG::SOLUTION, MPI_COMM_WORLD);
            cout << "[SLAVE " << rank << "] solution of size " << solution.size() << " sent to the master" << endl;
        }
    }

    void solveInner(BoardState & boardState, int step) {
        if (solution.size() == initLowerBound)
            return;

        // a (possibly not optimal) solution is found
        if (boardState.whitesLeft + boardState.blacksLeft == 0)  {
            if (!boardState.solutionCandidate.empty() && boardState.solutionCandidate.size() < upperBound) {
                #pragma omp critical
                if (!boardState.solutionCandidate.empty() && boardState.solutionCandidate.size() < upperBound) {
                    solution = boardState.solutionCandidate;
                    upperBound = boardState.solutionCandidate.size();
                }
            }
        }

        /* prepare information for all viable next moves (recursive calls) */

        vector<NextMoveInfo> nextMovesInfo;

        bool areWhitesOnTurn = ((step % 2 == 1) && (boardState.whitesLeft > 0)) || (boardState.blacksLeft == 0);
        const vector<position> & knights = areWhitesOnTurn ? boardState.whites : boardState.blacks;
        const map<position, int> & knightDistances = areWhitesOnTurn ? instanceInfo.minDistancesWhites : instanceInfo.minDistancesBlacks;

        for (int i = 0; i < knights.size(); ++i) {
            position current = knights[i];

            for (const position & next: instanceInfo.movesForPos.find(current)->second) {
                if (boardState.boardOccupation[next])
                    continue;

                size_t nextLowerBound = boardState.lowerBound - knightDistances.find(current)->second + knightDistances.find(next)->second;
                if (step + nextLowerBound + 1 >= upperBound) {
                    continue;
                }

                nextMovesInfo.emplace_back(nextLowerBound, i, current, next);
            }
        }

        /* perform all viable next moves (recursive calls) */

        sort(nextMovesInfo.begin(), nextMovesInfo.end(), nextCallComparator);

        for (const auto & item : nextMovesInfo) {
            int i = item.knightIndex;
            position current = item.currentPos;
            position next = item.nextPos;
            int nextLowerBound = item.nextLowerBound;

            /* prepare a board state for the next call */

            BoardState newBoardState(boardState);

            if (areWhitesOnTurn) {
                newBoardState.whites[i] = next;

                if (instanceInfo.squareType[current] == BLACK)
                    newBoardState.whitesLeft++;
                if (instanceInfo.squareType[next] == BLACK)
                    newBoardState.whitesLeft--;
            } else {
                newBoardState.blacks[i] = next;

                if (instanceInfo.squareType[current] == WHITE)
                    newBoardState.blacksLeft++;
                if (instanceInfo.squareType[next] == WHITE)
                    newBoardState.blacksLeft--;
            }

            newBoardState.boardOccupation[current] = false;
            newBoardState.boardOccupation[next] = true;
            newBoardState.lowerBound = nextLowerBound;
            newBoardState.solutionCandidate.emplace_back(current, next);

            /* do the call */

            #pragma omp task
            {
                solveInner(newBoardState, step + 1);
                #pragma omp cancel taskgroup if (solution.size() == initLowerBound)
            }
        }
    }

private:
    const InstanceInfo & instanceInfo;

    /**
     * To let all threads know they can stop searching
     * when current solution size reaches the initial board state's lower bound
     */
    const size_t initLowerBound;
    size_t upperBound{};
    const int rank;
    vector<pair<position,position>> solution;

    /**
     * Helper structure holding information needed for recursive calls
     */
    struct NextMoveInfo {
        NextMoveInfo(int nextLowerBound, int knightIndex, position currentPos, position nextPos) :
                nextLowerBound(nextLowerBound), knightIndex(knightIndex), currentPos(currentPos), nextPos(nextPos) {
        }

        int nextLowerBound, knightIndex, currentPos, nextPos;
    };

    static bool nextCallComparator(const NextMoveInfo &a, const NextMoveInfo &b) {
        return a.nextLowerBound < b.nextLowerBound;
    }
};

#endif //KNIGHT_SWAP_SOLVERSLAVE_H
