#ifndef KNIGHT_SWAP_TYPES_H
#define KNIGHT_SWAP_TYPES_H

/***
 * Position in 1D array representing given square on the game board
 */
typedef int position;

/***
 * Type of a square on the game board
 */
enum SquareType {
    BASIC,
    WHITE,
    BLACK
};

/***
 * MPI TAG
 */
enum TAG {
    INSTANCE_INFO,
    BOARD_STATE,
    BOARD_STATE_OTHERS,
    SOLUTION_SIZE_UPDATE,
    SOLUTION,
    END
};

#endif //KNIGHT_SWAP_TYPES_H
