#ifndef KNIGHT_SWAP_INPUTDATA_H
#define KNIGHT_SWAP_INPUTDATA_H

#include <string>
#include <fstream>

using namespace std;

/***
 * Input information loaded from a file
 */
struct InputData {
    explicit InputData(const string& path) {
        // read data
        ifstream instanceFile(path);
        instanceFile
                >> nCols >> nRows >> nKnightsInParty >> nKnightsInParty
                >> whiteArea1_col >> whiteArea1_row >> whiteArea2_col >> whiteArea2_row
                >> blackArea1_col >> blackArea1_row >> blackArea2_col >> blackArea2_row;

        // canonize white area
        int tmp;
        if (whiteArea2_row < whiteArea1_row) {
            tmp = whiteArea2_row;
            whiteArea2_row = whiteArea1_row;
            whiteArea1_row = tmp;
        }
        if (whiteArea2_col < whiteArea1_col) {
            tmp = whiteArea2_col;
            whiteArea2_col = whiteArea1_col;
            whiteArea1_col = tmp;
        }

        // canonize black area
        if (blackArea2_row < blackArea1_row) {
            tmp = blackArea2_row;
            blackArea2_row = blackArea1_row;
            blackArea1_row = tmp;
        }
        if (blackArea2_col < blackArea1_col) {
            tmp = blackArea2_col;
            blackArea2_col = blackArea1_col;
            blackArea1_col = tmp;
        }
    }

    int
            nRows{}, nCols{}, nKnightsInParty{},
            whiteArea1_row{}, whiteArea1_col{}, whiteArea2_row{}, whiteArea2_col{},
            blackArea1_row{}, blackArea1_col{}, blackArea2_row{}, blackArea2_col{};
};

#endif //KNIGHT_SWAP_INPUTDATA_H
