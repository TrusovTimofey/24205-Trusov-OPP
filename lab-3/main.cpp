#include <mpi.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include <random>

int RANK=0;
int SIZE=1;

class Matrix {
protected:
    int _width;
    int _height;
    double* _values = nullptr;
    bool _allocated = true;

    double& get(int x, int y) { return _values[y * _width + x]; }
    const double& get(int x, int y) const { return _values[y * _width + x]; }

public:
    Matrix(int width, int height) : _width(width), _height(height) {
        _values = new double[width * height]();
    }

    Matrix(int width, int height, double* data) : _width(width), _height(height), _values(data) {
        _allocated = false;
    }

    ~Matrix() {
        if (_values && _allocated) delete[] _values;
        _values = nullptr;
    }

    Matrix(const Matrix& other) : _width(other._width), _height(other._height) {
        _values = new double[_width * _height];
        std::memcpy(_values, other._values, sizeof(double) * _width * _height);
    }

    Matrix& operator=(const Matrix& other) {
        if (this != &other) {
            if (_values && _allocated) delete[] _values;
            _width = other._width;
            _height = other._height;
            _values = new double[_width * _height];
            std::memcpy(_values, other._values, sizeof(double) * _width * _height);
        }
        return *this;
    }

    Matrix(Matrix&& other) noexcept : _width(other._width), _height(other._height), _values(other._values) {
        other._values = nullptr;
        other._width = 0;
        other._height = 0;
    }

    Matrix& operator=(Matrix&& other) noexcept {
        if (this != &other) {
            if (_values && _allocated) delete[] _values;
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

    void fillRandom() {
        std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count() + RANK);
        std::uniform_int_distribution<int> dist(-10, 10);

        for (int y = 0; y < _height; y++) {
            for (int x = 0; x < _width; x++) {
                get(x, y) = static_cast<double>(dist(gen));
            }
        }
    }

    double* data() { return _values; }
    int width() const { return _width; }
    int height() const { return _height; }

    double& operator()(int x, int y) {
        if (x < 0 || y < 0 || x >= _width || y >= _height)
            throw std::invalid_argument("Index out of range");
        return get(x, y);
    }

    Matrix* splitVertical(int part, int parts) const {
        int baseSize = _width / parts;
        int remainder = _width % parts;
        int partSize = baseSize + (part < remainder ? 1 : 0);
        int offset = part * baseSize + std::min(part, remainder);
        Matrix* mat = new Matrix(partSize, _height);
        for (int y = 0; y < _height; y++)
            for (int x = 0; x < partSize; x++)
                mat->get(x, y) = get(x + offset, y);
        return mat;
    }

    Matrix* splitHorizontal(int part, int parts) const {
        int baseSize = _height / parts;
        int remainder = _height % parts;
        int partSize = baseSize + (part < remainder ? 1 : 0);
        int offset = part * baseSize + std::min(part, remainder);
        Matrix* mat = new Matrix(_width, partSize);
        std::memcpy(mat->_values, &(get(0, offset)), sizeof(double) * _width * partSize);
        return mat;
    }

    void insertBlock(const Matrix& other, int posX, int posY) {
        for (int y = 0; y < other._height; y++) {
            for (int x = 0; x < other._width; x++) {
                get(posX + x, posY + y) = other.get(x, y);
            }
        }
    }

    static Matrix multiply(const Matrix& a, const Matrix& b) {
        if (a._width != b._height)
            throw std::runtime_error("Matrix multiplication: incompatible dimensions");
        Matrix result(b._width, a._height);
        for (int y = 0; y < a._height; y++) {
            for (int x = 0; x < b._width; x++) {
                double sum = 0;
                for (int i = 0; i < a._width; i++)
                    sum += a.get(i, y) * b.get(x, i);
                result.get(x, y) = sum;
            }
        }
        return result;
    }

    static Matrix* multiplyMPI(const Matrix* const a, const Matrix* const b) {
        int rows = SIZE;
        int cols = 1;
        for (int i = sqrt(SIZE); i >= 1; i--) {
            if (SIZE % i == 0) {
                rows = SIZE / i;
                cols = i;
                break;
            }  
        }

        MPI_Comm cart_comm, row_comm, col_comm;

        int dims[2] = {cols, rows};
        int periods[2] = {0, 0};
        MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);

        int coords[2];
        MPI_Cart_coords(cart_comm, RANK, 2, coords);

        int row_dims[2] = {1, 0};
        MPI_Cart_sub(cart_comm, row_dims, &row_comm);

        int col_dims[2] = {0, 1};
        MPI_Cart_sub(cart_comm, col_dims, &col_comm);

        Matrix* aSplited = nullptr;
        Matrix* bSplited = nullptr;

        Matrix* c = nullptr;
        int* recvCount = nullptr;
        int* offsets = nullptr;
        int* blockW = nullptr;
        int* blockH = nullptr;
        int* offsetX = nullptr;
        int* offsetY = nullptr;

        if (coords[0] == 0 && coords[1] == 0) {
            c = new Matrix(b->width(), a->height());

            blockW = new int[cols];
            blockH = new int[rows];
            offsetX = new int[cols + 1];
            offsetY = new int[rows + 1];

            offsetY[0] = 0;
            for (int y = 0; y < rows; y++) {
                Matrix* part = a->splitHorizontal(y, rows);
                blockH[y] = part->height();
                offsetY[y + 1] = offsetY[y] + blockH[y];

                if (y == 0) {
                    aSplited = part;
                } else {
                    int sizes[2] = {part->width(), part->height()};
                    MPI_Send(sizes, 2, MPI_INT, y, 0, col_comm);
                    MPI_Send(part->data(), part->width() * part->height(), MPI_DOUBLE, y, 1, col_comm);
                    delete part;
                }
            }

            offsetX[0] = 0;
            for (int x = 0; x < cols; x++) {
                Matrix* part = b->splitVertical(x, cols);
                blockW[x] = part->width();
                offsetX[x + 1] = offsetX[x] + blockW[x];

                if (x == 0) {
                    bSplited = part;
                } else {
                    int sizes[2] = {part->width(), part->height()};
                    MPI_Send(sizes, 2, MPI_INT, x, 2, row_comm);
                    MPI_Send(part->data(), part->width() * part->height(), MPI_DOUBLE, x, 3, row_comm);
                    delete part;
                }
            }

            recvCount = new int[rows * cols];
            offsets = new int[rows * cols];
            int idx = 0;
            for (int col = 0; col < cols; col++) {
                for (int row = 0; row < rows; row++) {
                    recvCount[idx] = blockW[col] * blockH[row];
                    idx++;
                }
            }
            offsets[0] = 0;
            for (int i = 1; i < rows * cols; i++) {
                offsets[i] = offsets[i - 1] + recvCount[i - 1];
            }

        } else {

            if (coords[0] == 0) {

                int sizes[2] = {0, 0};
                MPI_Recv(sizes, 2, MPI_INT, 0, 0, col_comm, MPI_STATUS_IGNORE);

                aSplited = new Matrix(sizes[0], sizes[1]);
                MPI_Recv(aSplited->data(), sizes[0] * sizes[1], MPI_DOUBLE, 0, 1, col_comm, MPI_STATUS_IGNORE);
            }
            else if (coords[1] == 0) {

                int sizes[2] = {0, 0};
                MPI_Recv(sizes, 2, MPI_INT, 0, 2, row_comm, MPI_STATUS_IGNORE);

                bSplited = new Matrix(sizes[0], sizes[1]);
                MPI_Recv(bSplited->data(), sizes[0] * sizes[1], MPI_DOUBLE, 0, 3, row_comm, MPI_STATUS_IGNORE);
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        int sizes[2];
        if (coords[0] == 0) {
            sizes[0] = aSplited->width();
            sizes[1] = aSplited->height();
        }
        MPI_Bcast(sizes, 2, MPI_INT, 0, row_comm);
        if (coords[0] != 0) {
            aSplited = new Matrix(sizes[0], sizes[1]);
        }
        MPI_Bcast(aSplited->data(), sizes[0] * sizes[1], MPI_DOUBLE, 0, row_comm);

        if (coords[1] == 0) {
            sizes[0] = bSplited->width();
            sizes[1] = bSplited->height();
        }
        MPI_Bcast(sizes, 2, MPI_INT, 0, col_comm);
        if (coords[1] != 0) {
            bSplited = new Matrix(sizes[0], sizes[1]);
        }
        MPI_Bcast(bSplited->data(), sizes[0] * sizes[1], MPI_DOUBLE, 0, col_comm);

        Matrix cSplited = Matrix::multiply(*aSplited, *bSplited);

        delete aSplited;
        delete bSplited;

        int rootCoord[2] = {0, 0};
        int rootRank;
        MPI_Cart_rank(cart_comm, rootCoord, &rootRank);

        double* recvBuffer = nullptr;
        if (coords[0] == 0 && coords[1] == 0) {
            recvBuffer = new double[c->width() * c->height()];
        }

        MPI_Gatherv(cSplited.data(), cSplited.width() * cSplited.height(), MPI_DOUBLE, recvBuffer, recvCount, offsets, MPI_DOUBLE, rootRank, cart_comm);

        if (coords[0] == 0 && coords[1] == 0) {
            for (int col = 0; col < cols; col++) {
                for (int row = 0; row < rows; row++) {
                    int idx = col * rows + row;
                    Matrix subBlock(blockW[col], blockH[row], recvBuffer + offsets[idx]);
                    c->insertBlock(subBlock, offsetX[col], offsetY[row]);
                }
            }

            delete[] recvBuffer;
            delete[] offsets;
            delete[] recvCount;
            delete[] blockW;
            delete[] blockH;
            delete[] offsetX;
            delete[] offsetY;
        }
        
        MPI_Comm_free(&row_comm);
        MPI_Comm_free(&col_comm);
        MPI_Comm_free(&cart_comm);

        return c;
    }


    std::string toString() const {
        std::string str;
        for (int y = 0; y < _height; y++) {
            str += "| ";
            for (int x = 0; x < _width - 1; x++)
                str += std::to_string(get(x, y)) + ", ";
            str += std::to_string(get(_width - 1, y)) + " |\n";
        }
        return str;
    }
};

bool test(Matrix& A, Matrix& B, Matrix& C) {
    Matrix linear = Matrix::multiply(A,B);
    //std::cout << "Clinear:\n" << linear.toString() << std::endl;

    double epsilon = 1e-9;
    for (int y = 0; y < linear.height(); y++) {
        for (int x = 0; x < linear.width(); x++) {
            if(std::fabs(linear(x,y) - C(x,y)) > epsilon) return false;
        }
    }
    return true;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);
    MPI_Comm_size(MPI_COMM_WORLD, &SIZE);
    
    if(RANK == 0){
        Matrix A(100, 100);
        Matrix B(100, 100);

        A.fillRandom();
        B.fillRandom();

        //std::cout << "A:\n" << A.toString() << std::endl;
        //std::cout << "B:\n" << B.toString() << std::endl;

        Matrix* C = Matrix::multiplyMPI(&A,&B);

        //std::cout << "C:\n" << C->toString() << std::endl;

        std::cout << test(A,B,*C) << std::endl;

        delete C;
    }
    else{
        Matrix* C = Matrix::multiplyMPI(nullptr,nullptr);
    }
    
    MPI_Finalize();
    return 0;
}