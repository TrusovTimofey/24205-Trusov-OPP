#include <mpi.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include <algorithm>

static int RANK = 0;
static int SIZE = 0;

class Vector {
private:
    int _size;
    double* _values = nullptr;

public:
    Vector(int size) : _size(size) {
        if (size <= 0) throw std::invalid_argument("Index must be a positive integer");
        _values = new double[size]();
    }
    Vector(const Vector& other) : _size(other._size), _values(new double[other._size]) {
        std::memcpy(_values, other._values, sizeof(double) * _size);
    }
    Vector(int size, double* data) : _size(size) {
        if (size <= 0) throw std::invalid_argument("Index must be a positive integer");
        _values = new double[size]();
        std::memcpy(_values, data, sizeof(double) * _size);
    }
    ~Vector() {
        if (_values == nullptr) return;
        delete[] _values;
        _values = nullptr;
    }

    double* data() { return _values; }
    const double* data() const { return _values; }

    void fill() {
        for (int i = 0; i < _size; i++) {
            _values[i] = _size + 1;
        }
    }

    int size() const { return _size; }

    double& operator()(int i) {
        if (_values == nullptr) throw std::runtime_error("Use after free");
        if (i < 0 || i >= _size) throw std::invalid_argument("Index out of range");
        return _values[i];
    }
    double operator()(int i) const {
        if (_values == nullptr) throw std::runtime_error("Use after free");
        if (i < 0 || i >= _size) throw std::invalid_argument("Index out of range");
        return _values[i];
    }

    Vector& operator-=(const Vector& other) {
        if (_values == nullptr) throw std::runtime_error("Use after free");
        if (other._values == nullptr) throw std::invalid_argument("Vector is empty");
        if (other._size != _size) throw std::invalid_argument("Vectors must have the same size");
        for (int i = 0; i < _size; i++) {
            _values[i] -= other._values[i];
        }
        return *this;
    }

    Vector& operator*=(double value) {
        if (_values == nullptr) throw std::runtime_error("Use after free");
        for (int i = 0; i < _size; i++) {
            _values[i] *= value;
        }
        return *this;
    }

    Vector& operator=(const Vector& other) {
        if (other._values == nullptr) throw std::invalid_argument("Vector is empty");
        if (_size != other._size) {
            if (_values) delete[] _values;
            _values = new double[other.size()];
            _size = other._size;
        }
        std::memcpy(_values, other._values, sizeof(double) * _size);
        return *this;
    }

    double norm() const {
        if (_values == nullptr) throw std::runtime_error("Use after free");
        double value = 0;
        for (int i = 0; i < _size; i++) {
            value += _values[i] * _values[i];
        }
        return std::sqrt(value);
    }

    Vector split(int part, int parts) const {
        if (parts > size()) throw std::invalid_argument("can`t split vector to less parts than it`s size");
        if (part >= parts) throw std::invalid_argument("part can`t be greater than parts");
        if (part < 0) throw std::invalid_argument("part can`t be less than zero");
        if (parts < 1) throw std::invalid_argument("parts can`t be less than one");

        int baseSize = _size / parts;
        int remainder = _size % parts;

        int partSize = baseSize + (part < remainder ? 1 : 0);
        int offset = part * baseSize + std::min(part, remainder);

        Vector vec(partSize);
        std::memcpy(vec._values, &_values[offset], sizeof(double) * partSize);

        return vec;
    }

    void insert(int part, int parts, const Vector& other) {
        if (parts > size()) throw std::invalid_argument("can`t split vector to less parts than it`s size");
        if (part >= parts) throw std::invalid_argument("part can`t be greater than parts");
        if (part < 0) throw std::invalid_argument("part can`t be less than zero");
        if (parts < 1) throw std::invalid_argument("parts can`t be less than one");

        int baseSize = _size / parts;
        int remainder = _size % parts;

        int partSize = baseSize + (part < remainder ? 1 : 0);
        int offset = part * baseSize + std::min(part, remainder);

        std::memcpy(&_values[offset], other._values, sizeof(double) * partSize);
    }

    std::string toString() const {
        if (_values == nullptr) throw std::runtime_error("Use after free");
        std::string str = "(";
        for (int i = 0; i < _size; i++) {
            str += std::to_string(_values[i]) + ", ";
        }
        str += ")\n";
        return str;
    }
};

class Matrix {
private:
    int _sizeX;
    int _sizeY;
    double* _values = nullptr;

    double& get(int x, int y) { return _values[y * _sizeX + x]; }
    const double& get(int x, int y) const { return _values[y * _sizeX + x]; }

public:
    Matrix(int sizeX, int sizeY) : _sizeX(sizeX), _sizeY(sizeY) {
        if (sizeX <= 0 || sizeY <= 0) throw std::invalid_argument("Matrix size must be greater then zero");
        _values = new double[sizeX * sizeY];
    }

    ~Matrix() {
        if (_values == nullptr) return;
        delete[] _values;
        _values = nullptr;
    }

    Matrix(const Matrix& other) : _sizeX(other._sizeX), _sizeY(other._sizeY), _values(new double[other._sizeX * other._sizeY]) {
        std::memcpy(_values, other._values, sizeof(double) * _sizeX * _sizeY);
    }

    double* data() { return _values; }
    const double* data() const { return _values; }

    void fill() {
        for (int y = 0; y < _sizeY; y++) {
            for (int x = 0; x < _sizeX; x++) {
                get(x, y) = (x == y) ? 2 : 1;
            }
        }
    }

    int sizeX() const { return _sizeX; }
    int sizeY() const { return _sizeY; }

    double& operator()(int x, int y) {
        if (_values == nullptr) throw std::runtime_error("Use after free");
        if (x < 0 || y < 0 || x >= _sizeX || y >= _sizeY) throw std::invalid_argument("Index out of range");
        return get(x, y);
    }

    Matrix& operator=(const Matrix& other) {
        if (other._values == nullptr) throw std::invalid_argument("Matrix is empty");
        if ((_sizeX * _sizeY) != (other._sizeX * other._sizeY)) {
            if (_values) delete[] _values;
            _sizeX = other._sizeX;
            _sizeY = other._sizeY;
            _values = new double[other._sizeX * other._sizeY];
        }
        std::memcpy(_values, other._values, sizeof(double) * _sizeX * _sizeY);
        return *this;
    }

    void multiply(const Vector& vec, Vector& result) const {
        if (vec.size() != _sizeX) throw std::invalid_argument("Vector size must be equal to matrix x size");
        if (result.size() != _sizeY) throw std::invalid_argument("Result vector size must be equal to matrix y size");

        for (int y = 0; y < _sizeY; y++) {
            double sum = 0.0;
            for (int x = 0; x < _sizeX; x++) {
                sum += get(x, y) * vec(x);
            }
            result(y) = sum;
        }
    }

    Matrix split(int part, int parts) {
        if (parts > sizeY()) throw std::invalid_argument("can`t split Matrix to less parts than it`s size");
        if (part >= parts) throw std::invalid_argument("part can`t be greater than parts");
        if (part < 0) throw std::invalid_argument("part can`t be less than zero");
        if (parts < 1) throw std::invalid_argument("parts can`t be less than one");

        int baseSize = _sizeY / parts;
        int remainder = _sizeY % parts;

        int partSize = baseSize + (part < remainder ? 1 : 0);
        int offset = part * baseSize + std::min(part, remainder);

        Matrix mat(_sizeX, partSize);
        std::memcpy(mat._values, &(get(0, offset)), sizeof(double) * _sizeX * partSize);

        return mat;
    }

    void insert(int part, int parts, const Matrix& other) {
        if (parts > sizeY()) throw std::invalid_argument("can`t split Matrix to less parts than it`s size");
        if (part >= parts) throw std::invalid_argument("part can`t be greater than parts");
        if (part < 0) throw std::invalid_argument("part can`t be less than zero");
        if (parts < 1) throw std::invalid_argument("parts can`t be less than one");

        int baseSize = _sizeY / parts;
        int remainder = _sizeY % parts;

        int offset = part * baseSize + std::min(part, remainder);

        std::memcpy(&(get(0, offset)), other._values, sizeof(double) * _sizeX * other.sizeY());
    }

    std::string toString() const {
        if (_values == nullptr) throw std::runtime_error("Use after free");
        std::string str;
        for (int y = 0; y < _sizeY; y++) {
            str += "| ";
            for (int x = 0; x < _sizeX; x++) {
                str += std::to_string(get(x, y)) + ", ";
            }
            str += "|\n";
        }
        return str;
    }
};

class RingIterator {
private:
    static double* current_x;
    static double* temp_x;

    static void ringMatMult(const Matrix& A, const Vector& x_local, Vector& result, const int* const sizes, const int* const offsets, int maxLocalSize) {
        int localSize = A.sizeY();

        for (int i = 0; i < localSize; i++) {
            current_x[i] = x_local(i);
        }
        for (int i = localSize; i < maxLocalSize; i++) {
            current_x[i] = 0.0;
        }

        for (int i = 0; i < localSize; i++) {
            result(i) = 0.0;
        }

        for (int i = 0; i < localSize; i++) {
            double sum = 0.0;
            for (int j = 0; j < localSize; j++) {
                int globalCol = offsets[RANK] + j;
                sum += A.data()[i * A.sizeX() + globalCol] * x_local(j);
            }
            result(i) += sum;
        }

        for (int step = 1; step < SIZE; step++) {
            int dest = (RANK + 1) % SIZE;
            int src  = (RANK - 1 + SIZE) % SIZE;

            MPI_Sendrecv(current_x, maxLocalSize, MPI_DOUBLE, dest, 0, temp_x, maxLocalSize, MPI_DOUBLE, src,  0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            int srcRank = (RANK - step + SIZE) % SIZE;
            int srcCount = sizes[srcRank];
            int srcStart = offsets[srcRank];

            for (int i = 0; i < localSize; i++) {
                double sum = 0.0;
                for (int j = 0; j < srcCount; j++) {
                    int globalCol = srcStart + j;
                    sum += A.data()[i * A.sizeX() + globalCol] * temp_x[j];
                }
                result(i) += sum;
            }

            std::copy(temp_x, temp_x + maxLocalSize, current_x);
        }
    }
    
public:
    static Vector solve(const Matrix& A, const Vector& b) {
        const double epsilon = 1e-100;
        double tau = 1.5 / (A.sizeX() + 1);

        int N = A.sizeX();
        int localSize = A.sizeY();

        int* sizes = new int[SIZE];
        MPI_Allgather(&localSize, 1, MPI_INT, sizes, 1, MPI_INT, MPI_COMM_WORLD);

        int* offsets = new int[SIZE];
        offsets[0] = 0;
        for (int i = 1; i < SIZE; i++) {
            offsets[i] = offsets[i - 1] + sizes[i - 1];
        }

        int maxLocalSize = 0;
        for (int i = 0; i < SIZE; i++) {
            if (sizes[i] > maxLocalSize) maxLocalSize = sizes[i];
        }

        current_x = new double[maxLocalSize];
        temp_x    = new double[maxLocalSize];

        double bNorm;
        {
            double localSqrBNorm = 0;
            for (int i = 0; i < localSize; i++) {
                localSqrBNorm += b(i) * b(i);
            }
            double globalSqrBNorm;
            MPI_Allreduce(&localSqrBNorm, &globalSqrBNorm, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
            bNorm = 1.0 / std::sqrt(globalSqrBNorm);
        }

        Vector x_local(localSize);
        Vector localRes(localSize);
        Vector localAx(localSize);

        bool isLess = false;

        while (!isLess) {
            ringMatMult(A, x_local, localAx, sizes, offsets, maxLocalSize);

            for (int i = 0; i < localSize; i++) {
                localRes(i) = localAx(i) - b(i);
            }

            double globalNorm;
            {
                double localSqrNorm = 0;
                for (int i = 0; i < localSize; i++) {
                    localSqrNorm += localRes(i) * localRes(i);
                }
                double globalSqrNorm;
                MPI_Allreduce(&localSqrNorm, &globalSqrNorm, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
                globalNorm = std::sqrt(globalSqrNorm) * bNorm;
            }

            localRes *= tau;
            x_local -= localRes;

            isLess = (globalNorm < epsilon);
        }

        Vector x_global(N);
        MPI_Allgatherv(x_local.data(), localSize, MPI_DOUBLE, x_global.data(), sizes, offsets, MPI_DOUBLE, MPI_COMM_WORLD);

        delete[] sizes;
        delete[] offsets;
        delete[] current_x;
        delete[] temp_x;
        return x_global;
    }

};

double* RingIterator::current_x = nullptr;
double* RingIterator::temp_x = nullptr;

bool checkRes(const Vector& x, double epsilon = 1e-5) {
    if (RANK != 0) return true;
    double maxDiff = 0.0;
    for (int i = 0; i < x.size(); i++) {
        double diff = std::abs(x(i) - 1.0);
        maxDiff = std::max(maxDiff, diff);
    }
    bool isGood = (maxDiff < epsilon);
    if (isGood) std::cout << "check PASSED." << std::endl;
    else std::cout << "check FAILED." << std::endl;
    return isGood;
}

std::chrono::microseconds Calculate() {
    std::chrono::microseconds duration;
    int N = 15000;

    if (RANK == 0) {
        Matrix A(N, N);
        Vector b(N);
        A.fill();
        b.fill();

        auto start = std::chrono::high_resolution_clock::now();

        Matrix localA = A.split(0, SIZE);
        Vector localB = b.split(0, SIZE);

        for (int i = 1; i < SIZE; i++) {
            Matrix local = A.split(i, SIZE);
            int localSize = local.sizeY();
            MPI_Send(&localSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(local.data(), N * localSize, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
            MPI_Send(b.split(i, SIZE).data(), localSize, MPI_DOUBLE, i, 2, MPI_COMM_WORLD);
        }

        Vector x = RingIterator::solve(localA, localB);
        checkRes(x);

        auto end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    }
    else {
        int localSize;
        MPI_Recv(&localSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        Matrix localA(N, localSize);
        MPI_Recv(localA.data(), N * localSize, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        Vector localB(localSize);
        MPI_Recv(localB.data(), localSize, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        RingIterator::solve(localA, localB);
    }
    return duration;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);

    std::chrono::microseconds minDuration = std::chrono::microseconds::max();
    for (int i = 0; i < 5; i++) {
        auto d = Calculate();
        if (d < minDuration) minDuration = d;
    }

    if (RANK == 0) {
        std::cout << "Min time: " << (minDuration.count() * 1e-6) << " s" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
