#ifndef KNIGHT_SWAP_SOLVER_H
#define KNIGHT_SWAP_SOLVER_H

#include <utility>
#include <algorithm>
#include <iostream>
#include "Types.h"
#include "BoardState.h"

using namespace std;

class Solver {
public:
    explicit Solver(const InstanceInfo & instanceInfo) :
        instanceInfo(instanceInfo) {
    }

    void solve(BoardState & boardState, int step) {
        optimalSolution = solveInner(boardState, step);
    }

    void printSolution() {
        if (optimalSolution.empty()) {
            cout << "Solution either does not exist or it is trivial (zero moves)!" << endl;
            return;
        }

        cout << "Solution length: " << optimalSolution.size() << endl;
        cout << "Found after " << nIterations << " iterations" << endl;
        int i = 0;

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
        cout << "-------- MOVE " << i++ << " --------" << endl;
        printBoard(board);

        // get and print next board states according to the solution
        for (const auto & move: optimalSolution) {
            board[move.second] = board[move.first];
            board[move.first] = '.';
            cout << "-------- MOVE " << i++ << " --------" << endl;
            printBoard(board);
        }
    }

private:
    const InstanceInfo & instanceInfo;
    vector<pair<position,position>> optimalSolution;
    size_t nIterations = 0;

    vector<pair<position,position>> solveInner(BoardState & boardState, int step) {
        nIterations++;

        if (boardState.whitesLeft + boardState.blacksLeft == 0)
            return boardState.solutionCandidate;


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
                if (step + nextLowerBound + 1 >= boardState.upperBound) {
                    continue;
                }

                nextMovesInfo.emplace_back(nextLowerBound, i, current, next);
            }
        }

        // call viable options

        sort(nextMovesInfo.begin(), nextMovesInfo.end(), nextCallComparator);

        for (const auto & item : nextMovesInfo) {
            int i = item.knightIndex;
            position current = item.currentPos;
            position next = item.nextPos;
            int nextLowerBound = item.nextLowerBound;

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

            auto res = solveInner(newBoardState, step + 1);

            if (!res.empty() && res.size() < boardState.upperBound) {
                if (res.size() == boardState.lowerBound) {
                    return res;
                }

                boardState.solution = res;
                boardState.upperBound = res.size();
            }
        }

        return boardState.solution;
    }

    struct NextMoveInfo {
        NextMoveInfo(int nextLowerBound, int knightIndex, position currentPos, position nextPos) :
                nextLowerBound(nextLowerBound), knightIndex(knightIndex), currentPos(currentPos), nextPos(nextPos) {
        }

        int nextLowerBound, knightIndex, currentPos, nextPos;
    };

    static bool nextCallComparator(const NextMoveInfo &a, const NextMoveInfo &b) {
        return a.nextLowerBound < b.nextLowerBound;
    }

    void printBoard(const vector<char> & board) {
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
