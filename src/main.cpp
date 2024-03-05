#include "InputData.h"
#include "InstanceInfo.h"
#include "BoardState.h"
#include "Solver.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "The input file path must be provided as an argument!" << endl;
        return 1;
    }

    const InputData inputData(argv[1]);
    const InstanceInfo instanceInfo(inputData);
    BoardState boardState(instanceInfo);
    Solver solver(instanceInfo);

    solver.solve(boardState, 0);
    solver.printSolution();

    return 0;
}
