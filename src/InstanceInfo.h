#ifndef KNIGHT_SWAP_INSTANCEINFO_H
#define KNIGHT_SWAP_INSTANCEINFO_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include "InputData.h"
#include "Types.h"

using namespace std;

/***
 * Static instance information that is based on the input data
 * It stays the same across all iterations
 */
class InstanceInfo {
public:
    explicit InstanceInfo(map<position,vector<position>> movesForPos,
                          const int nSquares, const int nKnightsInParty,
                          vector<SquareType> squareType,
                          map<position, int> minDistancesWhites, map<position, int> minDistancesBlacks) :
            movesForPos(std::move(movesForPos)),
            nSquares(nSquares),
            nKnightsInParty(nKnightsInParty),
            squareType(std::move(squareType)),
            minDistancesWhites(std::move(minDistancesWhites)),
            minDistancesBlacks(std::move(minDistancesBlacks))
    {
    }

    /**
     * For each position on the game board, it says all the possible destinations a knight can goes to from that position
     */
    const map<position,vector<position>> movesForPos;
    /**
     * Total amount of squares on the game board
     */
    const int nSquares;
    const int nKnightsInParty;
    /**
     * For each position on the game board, it says which type it is (black, white or basic)
     */
    const vector<SquareType> squareType;
    /**
     * For each position on the game board, it says the minimal distance to the destination area
     */
    const map<position, int> minDistancesWhites, minDistancesBlacks;

    vector<int> serialize() const {
        vector<int> buffer;

        buffer.push_back(nSquares);
        buffer.push_back(nKnightsInParty);

        for (const auto& item : movesForPos) {
            buffer.push_back(item.first); // position
            buffer.push_back((int)item.second.size()); // number of moves on that position

            for (const auto& move : item.second)
                buffer.push_back(move); // moves
        }

        for (const auto& st : squareType) {
            buffer.push_back(st);
        }

        for (const auto& item : minDistancesWhites) {
            buffer.push_back(item.first);
            buffer.push_back(item.second);
        }

        for (const auto& item : minDistancesBlacks) {
            buffer.push_back(item.first);
            buffer.push_back(item.second);
        }

        return buffer;
    }

    static InstanceInfo deserialize(vector<int>& buffer) {
        int bufferIndex = 0;

        int nSquares = buffer[bufferIndex++];
        int nKnightsInParty = buffer[bufferIndex++];

        map<position,vector<position>> movesForPos;
        for (int i = 0; i < nSquares; ++i) {
            int pos = buffer[bufferIndex++];
            int nMoves = buffer[bufferIndex++];

            vector<position> moves;
            for (int j = 0; j < nMoves; ++j)
                moves.push_back(buffer[bufferIndex++]);

            movesForPos[pos] = std::move(moves);
        }

        vector<SquareType> squareType;
        for (int i = 0; i < nSquares; ++i) {
            squareType.push_back(static_cast<SquareType>(buffer[bufferIndex++]));
        }

        map<position, int> minDistancesWhites;
        for (int i = 0; i < nSquares; ++i) {
            int id = buffer[bufferIndex++];
            minDistancesWhites[id] = buffer[bufferIndex++];
        }

        map<position, int> minDistancesBlacks;
        for (int i = 0; i < nSquares; ++i) {
            int id = buffer[bufferIndex++];
            minDistancesBlacks[id] = buffer[bufferIndex++];
        }

        return InstanceInfo(
                movesForPos,
                nSquares,
                nKnightsInParty,
                squareType,
                minDistancesWhites,
                minDistancesBlacks
        );
    }
};

#endif //KNIGHT_SWAP_INSTANCEINFO_H
