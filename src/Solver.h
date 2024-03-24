#ifndef KNIGHT_SWAP_SOLVER_H
#define KNIGHT_SWAP_SOLVER_H

#include <utility>
#include <algorithm>
#include <iostream>
#include "Types.h"
#include <omp.h>
#include "BoardState.h"

using namespace std;

/**
 * Used to finding and printing a solution for given problem instance
 */
class Solver {
public:
    explicit Solver(const InstanceInfo & instanceInfo) :
        instanceInfo(instanceInfo) {
    }

    /**
     * Finds a solution and stores it internally
     */
    void solve(BoardState & boardState, int step) {
        initLowerBound = boardState.lowerBound;
        upperBound = getInitUpperBound(boardState);

        #pragma omp parallel
        {
            #pragma omp single
            solveInner(boardState, step);
        }
    }

    /**
     * Prints the internally stored solution
     */
    void printSolution() {
        if (solution.empty()) {
            cout << "Solution either does not exist or it is trivial (zero moves)!" << endl;
            return;
        }

        cout << "Solution length: " << solution.size() << endl;
        cout << "Found after " << nIterations << " iterations" << endl;
        int moveNum = 0;

        // get and print init board state
        vector<char> board;
        for (size_t i = 0; i < instanceInfo.nSquares; ++i) {
            if (instanceInfo.squareType[i] == WHITE)
                board.emplace_back('W');
            else if (instanceInfo.squareType[i] == BLACK)
                board.emplace_back('B');
            else
                board.emplace_back('.');
        }
        cout << "-------- MOVE " << moveNum++ << " --------" << endl;
        printBoard(board);

        // get and print next board states according to the solution
        for (const auto & move: solution) {
            board[move.second] = board[move.first];
            board[move.first] = '.';
            cout << "-------- MOVE " << moveNum++ << " --------" << endl;
            printBoard(board);
        }
    }

private:
    const InstanceInfo & instanceInfo;

    /**
     * To let all threads know they can stop searching
     * when current solution size reaches the initial board state's lower bound
     */
    size_t initLowerBound;
    /**
     * First, it is set by the getInitUpperBound method
     * Then, it is the size of the current solution
     */
    size_t upperBound{};
    vector<pair<position,position>> solution;

    size_t nIterations = 0;

    void solveInner(BoardState & boardState, int step) {
        #pragma omp critical
        nIterations++;

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

    /**
     * A sum of minimal distances to the most distant squares in destination areas of all knights
     */
    int getInitUpperBound(BoardState & boardState) {
        int res = 0;

        // For each position, find the shortest path to the most distant square in the destination area for given color using BFS
        for (const auto& opt : {make_pair(boardState.whites, BLACK), make_pair(boardState.blacks, WHITE)}) {
            for (const int& pos: opt.first) {
                int mostDistantDestPathLen;
                // first: position
                // second.first: number of steps traveled so far from that pos
                // second.second: number of destination squares visited
                queue<pair<position, pair<int, int>>> q;
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

        return res+1;
    }

    /**
     * Converts the 1D game board representation back to 2D
     */
    void printBoard(const vector<char> & board) const {
        int i = instanceInfo.inputData.nCols;
        for (const auto & square : board) {
            if (i == 0) {
                cout << endl;
                i = instanceInfo.inputData.nCols;
            }
            cout << square;
            i--;
        }
        cout << endl;
    }
};

#endif //KNIGHT_SWAP_SOLVER_H
