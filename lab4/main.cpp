#include <iostream>
#include <chrono>
#include <mpi.h>

#include "GameField.h"
#include "Rule.h"
#include "GameOfLive.h"
#include "Globals.h"

int Globals::rank = 0;
int Globals::size = 0;

int main(int, char **)
{
    MPI_Init(nullptr, nullptr);
    MPI_Comm_rank(MPI_COMM_WORLD, &Globals::rank);
    MPI_Comm_size(MPI_COMM_WORLD, &Globals::size);

    const int size = 12;

    if (size % Globals::size)
    {
        std::cerr << ("the field size must be divisible by the number of processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    GameField field(size, size / Globals::size);
    Rule rule({3}, {2, 3});
    if (Globals::rank == 0)
    {
        field.bornCell(1, 0);
        field.bornCell(2, 1);
        field.bornCell(0, 2);
        field.bornCell(1, 2);
        field.bornCell(2, 2);
    }

    field.applyChanges();
    GameOfLive game(&rule, &field);

    auto start = std::chrono::high_resolution_clock::now();
    while (game.nextGeneration()){}
    auto end = std::chrono::high_resolution_clock::now();

    if (Globals::rank == 0)
    {
        if (game.generation() == size * 4)
        {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            std::cout << "Time: " << (duration.count() * 1e-6) << std::endl;
        }
        else std::cerr << "FAIL" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
