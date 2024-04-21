#ifndef KNIGHT_SWAP_SOLVERMASTER_H
#define KNIGHT_SWAP_SOLVERMASTER_H

#include <algorithm>
#include <iostream>
#include <omp.h>
#include <mpi.h>
#include<chrono>
#include<thread>
#include "Types.h"
#include "BoardState.h"

using namespace std;

/**
 * Used to finding and printing a solution for given problem instance
 */
class SolverMaster {
public:
    explicit SolverMaster(const InputData & inputData, const InstanceInfo & instanceInfo, int nSlaves) :
        inputData(inputData),
        instanceInfo(instanceInfo),
        nSlaves(nSlaves) {
    }

    /**
     * Finds a solution and stores it internally
     */
    void solve(BoardState & boardState, int step) {
        initLowerBound = boardState.lowerBound;
        upperBound = getInitUpperBound(boardState);

        set<int> slaves;
        for (int i = 1; i <= nSlaves; ++i)
            slaves.emplace(i);

        // prepare init tasks which will be sent to the slaves to be processed
        queue<pair<BoardState, int>> initStates = getInitStates(boardState, step);

        // init work of the slaves by sending them the first task
        for (const auto& slave : slaves) {
            auto state = initStates.front();
            initStates.pop();

            vector<int> bufferBoardState = boardState.serialize();
            MPI_Send(bufferBoardState.data(), (int)bufferBoardState.size(), MPI_INT, slave, TAG::BOARD_STATE, MPI_COMM_WORLD);

            vector<int> buffer;
            buffer.push_back(initLowerBound);
            buffer.push_back(upperBound);
            buffer.push_back(step);
            MPI_Send(buffer.data(), (int)buffer.size(), MPI_INT, slave, TAG::BOARD_STATE_OTHERS, MPI_COMM_WORLD);
        }

        cout << "[MASTER] init batch sent" << endl;

        vector<int> solutionSizeUpdateBuffer(1);
        int bufferSize = 200 * 2;

        // process the rest of the tasks
        while (!slaves.empty()) {
            std::vector<int> message(bufferSize);
            MPI_Status status;
            int flag;

            // probe for updates from slaves periodically
            while (true) {
                MPI_Iprobe(MPI_ANY_SOURCE, TAG::SOLUTION_SIZE_UPDATE, MPI_COMM_WORLD, &flag, &status);

                // one of the slaves found a solution
                if (flag) {
                    MPI_Recv(message.data(), bufferSize, MPI_INT, MPI_ANY_SOURCE, TAG::SOLUTION_SIZE_UPDATE, MPI_COMM_WORLD, &status);

                    // solution is better than the best one so far - update and notify other slaves
                    if (message[0] < upperBound) {
                        upperBound = message[0];
                        solutionSizeUpdateBuffer[0] = (int)upperBound;
                        cout << "[MASTER] upper bound updated to " << upperBound << endl;

                        for (const auto& slave : slaves) {
                            if (slave == status.MPI_SOURCE)
                                continue;
                            MPI_Request dummy_handle;
                            MPI_Isend(solutionSizeUpdateBuffer.data(), (int)solutionSizeUpdateBuffer.size(), MPI_INT, slave, TAG::SOLUTION_SIZE_UPDATE, MPI_COMM_WORLD, &dummy_handle);
                        }
                    }

                    message.clear();
                }

                MPI_Iprobe(MPI_ANY_SOURCE, TAG::SOLUTION, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);

                // one of the slaves finished his work
                if (flag) {
                    MPI_Recv(message.data(), bufferSize, MPI_INT, MPI_ANY_SOURCE, TAG::SOLUTION, MPI_COMM_WORLD, &status);
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            int bufferIndex = 0;
            int size = message[bufferIndex++];

            // better solution found
            if (size != 0 && size <= upperBound) {
                solution.clear();
                for (int i = 0; i < size; ++i) {
                    int first = message[bufferIndex++];
                    int second = message[bufferIndex++];
                    solution.emplace_back(first, second);
                }

                upperBound = solution.size();
                cout << "[MASTER] upper bound updated to " << upperBound << endl;

                nIterations += message[bufferIndex++];
            }

            // no more work to do - notify the slave who sent the solution
            if (initStates.empty()) {
                MPI_Request dummy_handle;
                MPI_Isend(nullptr, 0, MPI_INT, status.MPI_SOURCE, TAG::END, MPI_COMM_WORLD, &dummy_handle);
                slaves.erase(status.MPI_SOURCE);
            // still soe work to do - give the slave who sent the solution another task
            } else {
                auto state = initStates.front();
                initStates.pop();

                if (initStates.empty())
                    cout << "[MASTER] last init state pop" << endl;

                vector<int> bufferBoardState = boardState.serialize();
                MPI_Send(bufferBoardState.data(), (int)bufferBoardState.size(), MPI_INT, status.MPI_SOURCE, TAG::BOARD_STATE, MPI_COMM_WORLD);

                vector<int> buffer;
                buffer.push_back(initLowerBound);
                buffer.push_back(upperBound);
                buffer.push_back(step);
                MPI_Send(buffer.data(), (int)buffer.size(), MPI_INT, status.MPI_SOURCE, TAG::BOARD_STATE_OTHERS, MPI_COMM_WORLD);
            }
        }

        cout << "[MASTER] end" << endl;
    }

    /**
     * Prints the internally stored solution
     */
    void printSolution() {
        cout << "\nSOLUTION\n----------------------" << endl;

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
    const InputData & inputData;
    const InstanceInfo & instanceInfo;
    int nSlaves;

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
     * From one initial state, gets many of them
     *
     * A modification of the solverInner method
     * without doing recursive calls and with storing and returning states using BFS
     *
     * TODO refactor this to avoid code duplication while preserving efficiency
     */
    queue<pair<BoardState, int>> getInitStates(BoardState & initState, int initStep) {
        queue<pair<BoardState, int>> q; // board state and the corresponding step
        q.emplace(initState, initStep);

        int minNumOfStates = omp_get_max_threads() * 3;
        while (q.size() < minNumOfStates) {
            BoardState state = q.front().first;
            int step = q.front().second;
            q.pop();

            // a (possibly not optimal) solution is found
            if (state.whitesLeft + state.blacksLeft == 0)  {
                if (!state.solutionCandidate.empty() && state.solutionCandidate.size() < upperBound) {
                    solution = state.solutionCandidate;
                    upperBound = state.solutionCandidate.size();
                }
            }

            /* prepare all viable next states */

            bool areWhitesOnTurn = ((step % 2 == 1) && (state.whitesLeft > 0)) || (state.blacksLeft == 0);
            const vector<position> & knights = areWhitesOnTurn ? state.whites : state.blacks;
            const map<position, int> & knightDistances = areWhitesOnTurn ? instanceInfo.minDistancesWhites : instanceInfo.minDistancesBlacks;

            for (int i = 0; i < knights.size(); ++i) {
                position current = knights[i];

                for (const position & next: instanceInfo.movesForPos.find(current)->second) {
                    if (state.boardOccupation[next])
                        continue;

                    size_t nextLowerBound = state.lowerBound - knightDistances.find(current)->second + knightDistances.find(next)->second;
                    if (step + nextLowerBound + 1 >= upperBound) {
                        continue;
                    }

                    /* prepare a new board state */

                    BoardState newBoardState(state);

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

                    /* push it to the result */

                    q.emplace(newBoardState, step + 1);
                }
            }
        }

        return q;
    }

    /**
     * Converts the 1D game board representation back to 2D
     */
    void printBoard(const vector<char> & board) const {
        int i = inputData.nCols;
        for (const auto & square : board) {
            if (i == 0) {
                cout << endl;
                i = inputData.nCols;
            }
            cout << square;
            i--;
        }
        cout << endl;
    }
};

#endif //KNIGHT_SWAP_SOLVERMASTER_H
