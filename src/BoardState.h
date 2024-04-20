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
    explicit BoardState(int whitesLeft, int blacksLeft,
                        vector<position> whites, vector<position> blacks, vector<bool> boardOccupation,
                        size_t lowerBound,
                        vector<pair<position,position>> solutionCandidate
                        ) :
        whitesLeft(whitesLeft),
        blacksLeft(blacksLeft),
        whites(std::move(whites)),
        blacks(std::move(blacks)),
        boardOccupation(std::move(boardOccupation)),
        lowerBound(lowerBound),
        solutionCandidate(std::move(solutionCandidate)) {
    }

    BoardState(const BoardState & o) = default;

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

    vector<int> serialize() {
        vector<int> buffer;

        buffer.push_back(whitesLeft);
        buffer.push_back(blacksLeft);

        buffer.push_back((int)whites.size());
        for (const auto& pos : whites)
            buffer.push_back(pos);

        buffer.push_back((int)blacks.size());
        for (const auto& pos : blacks)
            buffer.push_back(pos);

        buffer.push_back((int)boardOccupation.size());
        for (const bool bo : boardOccupation)
            buffer.push_back(bo);

        buffer.push_back((int)lowerBound);

        buffer.push_back((int)solutionCandidate.size());
        for (const auto& item : solutionCandidate) {
            buffer.push_back(item.first);
            buffer.push_back(item.second);
        }

        return buffer;
    }

    static BoardState deserialize(vector<int>& buffer) {
        int bufferIndex = 0;

        int whitesLeft = buffer[bufferIndex++];
        int blacksLeft = buffer[bufferIndex++];

        vector<position> whites;
        int size = buffer[bufferIndex++];
        for (int i = 0; i < size; ++i)
            whites.push_back(buffer[bufferIndex++]);

        vector<position> blacks;
        size = buffer[bufferIndex++];
        for (int i = 0; i < size; ++i)
            blacks.push_back(buffer[bufferIndex++]);

        vector<bool> boardOccupation;
        size = buffer[bufferIndex++];
        for (int i = 0; i < size; ++i)
            boardOccupation.push_back(buffer[bufferIndex++]);

        int lowerBound = buffer[bufferIndex++];

        vector<pair<position,position>> solutionCandidate;
        size = buffer[bufferIndex++];
        for (int i = 0; i < size; ++i) {
            int first = buffer[bufferIndex++];
            int second = buffer[bufferIndex++];
            solutionCandidate.emplace_back(first, second);
        }

        return BoardState(
                whitesLeft, blacksLeft,
                whites, blacks, boardOccupation,
                lowerBound,
                solutionCandidate
        );
    }
};

#endif //KNIGHT_SWAP_BOARDSTATE_H
