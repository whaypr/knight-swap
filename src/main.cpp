#include "InputData.h"
#include "InstanceInfoBuilder.h"
#include "InstanceInfo.h"
#include "BoardStateBuilder.h"
#include "BoardState.h"
#include "SolverMaster.h"
#include "SolverSlave.h"

using namespace std;

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, nSlaves;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nSlaves);
    nSlaves--; // do not count master

    /* master */
    if (rank == 0) {
        cout << "[MASTER] spawn" << endl;

        if (argc != 2) {
            cerr << "The input file path must be provided as an argument!" << endl;

            // tell the slaves to end and exit
            for (int i = 1; i <= nSlaves; ++i) {
                MPI_Request dummy_handle;
                MPI_Isend(nullptr, 0, MPI_INT, i, TAG::END, MPI_COMM_WORLD, &dummy_handle);
            }

            MPI_Finalize();
            return 1;
        }

        // parse input
        const InputData inputData(argv[1]);
        const InstanceInfo instanceInfo = InstanceInfoBuilder({inputData}).build();
        BoardState boardState = BoardStateBuilder({instanceInfo}).build();

        // send parsed instance info to the slaves
        vector<int> message = instanceInfo.serialize();
        for (int i = 1; i <= nSlaves; ++i) {
            MPI_Send(message.data(), (int)message.size(), MPI_INT, i, TAG::INSTANCE_INFO, MPI_COMM_WORLD);
        }

        // start solving
        SolverMaster master(inputData, instanceInfo, nSlaves);
        master.solve(boardState, 0);
        master.printSolution();

    /* slaves */
    } else {
        // check whether end or not - if input was not specified, the master will send a command to end
        MPI_Status status;
        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == TAG::END) {
            MPI_Finalize();
            return 1;
        }

        int bufferSize = 2 + 400 * 11 + 1500;
        vector<int> message(bufferSize);

        // get instance info
        MPI_Recv(message.data(), bufferSize, MPI_INT, 0, TAG::INSTANCE_INFO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        const InstanceInfo instanceInfo = InstanceInfo::deserialize(message);

        // keep receiving and solving subtasks as long as there are some
        while (true) {

            // check whether end or not - if all work is done, the master will send a command to end
            MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == TAG::END)
                break;

            // get a board state to be worked on
            MPI_Recv(message.data(), bufferSize, MPI_INT, 0, TAG::BOARD_STATE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            BoardState boardState = BoardState::deserialize(message);
            cout << "\t[SLAVE " << rank << "] board received" << endl;

            // get some additional info about state of the solution-finding process
            MPI_Recv(message.data(), bufferSize, MPI_INT, 0, TAG::BOARD_STATE_OTHERS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            int bufferIndex = 0;
            size_t initLowerBound = message[bufferIndex++];
            size_t upperBound = message[bufferIndex++];
            int step = message[bufferIndex++];
            cout << "\t[SLAVE " << rank << "] additional info received" << endl;

            // solve
            SolverSlave slave(instanceInfo, initLowerBound, upperBound, rank);
            slave.solve(boardState, step);
        }
    }

    if (rank != 0)
        cout << "\t[SLAVE " << rank <<  "] end" << endl;
    MPI_Finalize();

    return 0;
}
