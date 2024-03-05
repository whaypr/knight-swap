#include "InputData.h"
#include "InstanceInfo.h"
#include "BoardState.h"
#include "Solver.h"

using namespace std;

int main() {
    const InputData inputData("../test/instances/in_0001.txt");
    const InstanceInfo instanceInfo(inputData);
    BoardState boardState(instanceInfo);
    Solver solver(instanceInfo);

    solver.solve(boardState, 0);
    solver.printSolution();

    return 0;
}
