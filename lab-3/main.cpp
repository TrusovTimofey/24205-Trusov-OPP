#include <mpi.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include <random>

int RANK = 0;
int SIZE = 1;

class Matrix
{
protected:
    int _width;
    int _height;
    double *_values = nullptr;
    bool _allocated = true;

    double &get(int x, int y) { return _values[y * _width + x]; }
    const double &get(int x, int y) const { return _values[y * _width + x]; }

public:
    Matrix(int width, int height) : _width(width), _height(height)
    {
        _values = new double[width * height]();
    }

    Matrix(int width, int height, double *data) : _width(width), _height(height), _values(data)
    {
        _allocated = false;
    }

    ~Matrix()
    {
        if (_values && _allocated)
            delete[] _values;
        _values = nullptr;
    }

    Matrix(const Matrix &other) : _width(other._width), _height(other._height)
    {
        _values = new double[_width * _height];
        std::memcpy(_values, other._values, sizeof(double) * _width * _height);
    }

    Matrix &operator=(const Matrix &other)
    {
        if (this != &other)
        {
            if (_values && _allocated)
                delete[] _values;
            _width = other._width;
            _height = other._height;
            _values = new double[_width * _height];
            std::memcpy(_values, other._values, sizeof(double) * _width * _height);
        }
        return *this;
    }

    Matrix(Matrix &&other) noexcept : _width(other._width), _height(other._height), _values(other._values)
    {
        other._values = nullptr;
        other._width = 0;
        other._height = 0;
    }

    Matrix &operator=(Matrix &&other) noexcept
    {
        if (this != &other)
        {
            if (_values && _allocated)
                delete[] _values;
            _width = other._width;
            _height = other._height;
            _values = other._values;
            _allocated = other._allocated;
            other._values = nullptr;
            other._width = 0;
            other._height = 0;
        }
        return *this;
    }

    void fillRandom()
    {
        std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count() + RANK);
        std::uniform_int_distribution<int> dist(-10, 10);

        for (int y = 0; y < _height; y++)
        {
            for (int x = 0; x < _width; x++)
            {
                get(x, y) = static_cast<double>(dist(gen));
            }
        }
    }

    double *data() { return _values; }
    const double *data() const { return _values; }
    int width() const { return _width; }
    int height() const { return _height; }

    double &operator()(int x, int y)
    {
        if (x < 0 || y < 0 || x >= _width || y >= _height)
            throw std::invalid_argument("Index out of range");
        return get(x, y);
    }

    static Matrix multiply(const Matrix &a, const Matrix &b)
    {
        if (a._width != b._height)
            throw std::runtime_error("Matrix multiplication: incompatible dimensions");
        Matrix result(b._width, a._height);

        for (int y = 0; y < a._height; y++)
        {
            for (int k = 0; k < a._width; k++)
            {
                double temp = a.get(k, y);
                for (int x = 0; x < b._width; x++)
                {
                    result.get(x, y) += temp * b.get(x, k);
                }
            }
        }
        return result;
    }

    static Matrix *multiplyMPI(const Matrix *const a, const Matrix *const b)
    {
        int N[3];
        if (RANK == 0)
        {
            N[0] = a->height();
            N[1] = a->width();
            N[2] = b->width();
        }
        MPI_Bcast(N, 3, MPI_INT, 0, MPI_COMM_WORLD);

        int dims[2] = {0, 0};
        MPI_Dims_create(SIZE, 2, dims);
        int rows = dims[0];
        int cols = dims[1];

        if (N[0] % rows != 0 || N[2] % cols != 0)
        {
            if (RANK == 0)
                std::cerr << "Matrix dimensions must be divisible by the process grid dimensions\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        int blockRows = N[0] / rows;
        int blockCols = N[2] / cols;

        int periods[2] = {0, 0};

        MPI_Comm cart_comm, row_comm, col_comm;
        MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);

        int coords[2];
        MPI_Cart_coords(cart_comm, RANK, 2, coords);

        MPI_Comm_split(cart_comm, coords[0], RANK, &row_comm);
        MPI_Comm_split(cart_comm, coords[1], RANK, &col_comm);

        Matrix localA(N[1], blockRows);
        Matrix localB(blockCols, N[1]);


        std::vector<int> aSendCounts(rows, 0);
        std::vector<int> aDispls(rows, 0);
        if (RANK == 0)
        {
            int temp = blockRows * N[1];
            for (int i = 0; i < rows; i++)
            {
                aSendCounts[i] = temp;
                aDispls[i] = i * temp;
            }
        }

        if (coords[1] == 0)
        {
            MPI_Scatterv(RANK == 0 ? a->data() : nullptr, aSendCounts.data(), aDispls.data(), MPI_DOUBLE,
                         localA.data(), blockRows * N[1], MPI_DOUBLE, 0, col_comm);
        }


        MPI_Datatype colType, resizedColType;
        MPI_Type_vector(N[1], blockCols, N[2], MPI_DOUBLE, &colType);
        MPI_Type_create_resized(colType, 0, blockCols * sizeof(double), &resizedColType);
        MPI_Type_free(&colType);
        MPI_Type_commit(&resizedColType);

        if (coords[0] == 0)
        {
            MPI_Scatter(RANK == 0 ? b->data() : nullptr, 1, resizedColType,
                        localB.data(), N[1] * blockCols, MPI_DOUBLE, 0, row_comm);
        }

        MPI_Type_free(&resizedColType);


        MPI_Bcast(localA.data(), blockRows * N[1], MPI_DOUBLE, 0, row_comm);
        MPI_Bcast(localB.data(), N[1] * blockCols, MPI_DOUBLE, 0, col_comm);

        Matrix localC = Matrix::multiply(localA, localB);


        Matrix *c = nullptr;
        if (RANK == 0) c = new Matrix(N[2], N[0]);

        MPI_Datatype blockType, resizedBlockType;
        int matrixSizes[2] = {N[0], N[2]};
        int gridSizes[2] = {blockRows, blockCols};
        int starts[2] = {0, 0};
        MPI_Type_create_subarray(2, matrixSizes, gridSizes, starts, MPI_ORDER_C, MPI_DOUBLE, &blockType);
        MPI_Type_create_resized(blockType, 0, sizeof(double), &resizedBlockType);
        MPI_Type_free(&blockType);
        MPI_Type_commit(&resizedBlockType);

        std::vector<int> recvcounts(SIZE, 1);
        std::vector<int> displs(SIZE, 0);
        for (int y = 0; y < rows; ++y){
            for (int x = 0; x < cols; ++x){
                displs[y * cols + x] = y * blockRows * N[2] + x * blockCols;
            }
        }

        MPI_Gatherv(localC.data(), blockRows * blockCols, MPI_DOUBLE,
                    (RANK == 0) ? c->data() : nullptr,
                    recvcounts.data(), displs.data(), resizedBlockType,
                    0, cart_comm);

        MPI_Type_free(&resizedBlockType);

        MPI_Comm_free(&row_comm);
        MPI_Comm_free(&col_comm);
        MPI_Comm_free(&cart_comm);

        return c;
    }
    std::string
    toString() const
    {
        std::string str;
        for (int y = 0; y < _height; y++)
        {
            str += "| ";
            for (int x = 0; x < _width - 1; x++)
                str += std::to_string(get(x, y)) + ", ";
            str += std::to_string(get(_width - 1, y)) + " |\n";
        }
        return str;
    }
};

bool test(Matrix &A, Matrix &B, Matrix &C)
{
    Matrix linear = Matrix::multiply(A, B);

    double epsilon = 1e-9;
    for (int y = 0; y < linear.height(); y++)
    {
        for (int x = 0; x < linear.width(); x++)
        {
            if (std::fabs(linear(x, y) - C(x, y)) > epsilon)
                return false;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);
    MPI_Comm_size(MPI_COMM_WORLD, &SIZE);

    Matrix *A = nullptr;
    Matrix *B = nullptr;

    if (RANK == 0)
    {
        A = new Matrix(3600, 120);
        B = new Matrix(120, 3600);

        A->fillRandom();
        B->fillRandom();
    }

    std::chrono::microseconds duration = std::chrono::microseconds::max();
    for (int i = 0; i < 5; i++)
    {
        MPI_Barrier(MPI_COMM_WORLD);

        auto start = std::chrono::high_resolution_clock::now();

        Matrix *C = Matrix::multiplyMPI(A, B);

        auto end = std::chrono::high_resolution_clock::now();

        if (RANK == 0)
        {
            if (!test(*A, *B, *C))
            {
                std::cout << "FAIL\n";
                delete A;
                delete B;
                delete C;
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
                return 1;
            }
            delete C;
        }

        auto local = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        duration = duration > local ? local : duration;
    }

    if (RANK == 0)
    {
        delete A;
        delete B;
        std::cout << "Time: " << (duration.count() * 1e-6) << std::endl;
    }

    MPI_Finalize();
    return 0;
}
