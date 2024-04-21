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
        rank(rank),
        solutionSizeUpdateBuffer(1) {
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

        // send solution to the master - send even the empty solution to let master know this slave wants another task
        vector<int> buffer;
        buffer.push_back(solution.size());
        for (const auto& item : solution) {
            buffer.push_back(item.first);
            buffer.push_back(item.second);
        }
        MPI_Send(buffer.data(), (int)buffer.size(), MPI_INT, 0, TAG::SOLUTION, MPI_COMM_WORLD);

        if (!solution.empty())
            cout << "\t[SLAVE " << rank << "] solution of size " << solution.size() << " sent to the master" << endl;
    }

    void solveInner(BoardState & boardState, int step) {
        if (solution.size() == initLowerBound)
            return;

        // check if there is a better upper bound found by another slave
        // only one of the threads needs to actually read it - it will then update it for the other threads
        if (omp_get_thread_num() == 0) {

            // there can be multiple updates - read through all of them
            int flag = 1;
            while (flag) {
                MPI_Iprobe(0, TAG::SOLUTION_SIZE_UPDATE, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
                if (flag) {
                    int bufferSize = 16;
                    vector<int> message(bufferSize);
                    MPI_Recv(message.data(), bufferSize, MPI_INT, 0, TAG::SOLUTION_SIZE_UPDATE, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);

                    if (message[0] < upperBound) {
                        #pragma omp critical
                        upperBound = message[0];
                    }

                    cout << "\t[SLAVE " << rank << "] upper bound of size " << solution.size() << " received from the master" << endl;
                }
            }
        }

        // a (possibly not optimal but the best so far) solution is found
        if (boardState.whitesLeft + boardState.blacksLeft == 0)  {
            if (!boardState.solutionCandidate.empty() && boardState.solutionCandidate.size() < upperBound) {
                #pragma omp critical
                if (!boardState.solutionCandidate.empty() && boardState.solutionCandidate.size() < upperBound) {
                    solution = boardState.solutionCandidate;
                    upperBound = solution.size();

                    // send information about the size of the new solution to the master
                    // because the best upper bound known to the master was sent to this slave previously,
                    // this communication will happen only if this solution is better
                    solutionSizeUpdateBuffer[0] = (int)upperBound;
                    MPI_Request dummy_handle;
                    MPI_Isend(solutionSizeUpdateBuffer.data(), (int)solutionSizeUpdateBuffer.size(), MPI_INT, 0, TAG::SOLUTION_SIZE_UPDATE, MPI_COMM_WORLD, &dummy_handle);
                    cout << "\t[SLAVE " << rank << "] upper bound of size " << upperBound << " sent to the master" << endl;
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
            solveInner(newBoardState, step + 1);
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

    vector<int> solutionSizeUpdateBuffer;

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
