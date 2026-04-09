#include <mpi.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>

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
            if (_values && _allocated)delete[] _values;
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
        for(int y = 0; y < other._height; y++){
            for(int x = 0; x < other._width; x++){
                get(posX+x,posY+y) = other.get(x,y);
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

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rows = 3, cols = 3;
    MPI_Comm cart_comm, row_comm, col_comm;

    int dims[2] = {cols, rows};
    int periods[2] = {0, 0};
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);

    int coords[2];
    MPI_Cart_coords(cart_comm, rank, 2, coords);

    int row_dims[2] = {1, 0};
    MPI_Cart_sub(cart_comm, row_dims, &row_comm);

    int col_dims[2] = {0, 1};
    MPI_Cart_sub(cart_comm, col_dims, &col_comm);

    Matrix* aSplited = nullptr;
    Matrix* bSplited = nullptr;

    Matrix* C = nullptr;
    int* offsets = nullptr;
    int* recvCount = nullptr;

    if (coords[0] == 0 && coords[1] == 0) {
        Matrix A(2, 3);
        Matrix B(3, 2);

        A(0,0)=1; A(1,0)=0;
        A(0,1)=0; A(1,1)=1;
        A(0,2)=1; A(1,2)=1;
        
        B(0,0)=1; B(1,0)=0; B(2,0)=1; 
        B(0,1)=0; B(1,1)=1; B(2,1)=-1;
        
        C = new Matrix(3,3);
        offsets = new int[rows*cols];
        recvCount = new int[rows*cols];
        
        aSplited = A.splitHorizontal(0, rows);
        for(int x = 0; x < cols; x++){
            recvCount[x] = aSplited->height();
        }
        for (int y = 1; y < rows; y++) {
            Matrix* part = A.splitHorizontal(y, rows);
            int sizes[2] = {part->width(), part->height()};

            for(int x = 0; x < cols; x++){
                recvCount[x+y*cols] = sizes[1];
            }

            MPI_Send(sizes, 2, MPI_INT, y, 0, col_comm);
            MPI_Send(part->data(), part->width() * part->height(), MPI_DOUBLE, y, 1, col_comm);
            delete part;
        }
        
        bSplited = B.splitVertical(0, cols);
        for (int y = 1; y < rows; y++) {
            recvCount[y*cols]*= bSplited->width();
        }
        for (int x = 1; x < cols; x++) {
            Matrix* part = B.splitVertical(x, cols);
            int sizes[2] = {part->width(), part->height()};

            for (int y = 0; y < rows; y++) {
                recvCount[x + y*cols]*= bSplited->width();
            }
            
            MPI_Send(sizes, 2, MPI_INT, x, 2, row_comm);
            MPI_Send(part->data(), part->width() * part->height(), MPI_DOUBLE, x, 3, row_comm);
            delete part;
        }

        offsets[0]=0;
        for(int i = 1; i < rows*cols; i++){
            offsets[i]=recvCount[i-1]+offsets[i-1];
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

    int rootCoord[2] = {0,0};
    int rootRank;
    MPI_Cart_rank(cart_comm, rootCoord, &rootRank);

    double* recvBuffer = nullptr;
    if(coords[1] == 0 && coords[0] == 0) {
        recvBuffer = new double[C->width()*C->height()];
    }

    MPI_Gatherv(cSplited.data(), cSplited.width()*cSplited.height(), MPI_DOUBLE, recvBuffer, recvCount, offsets, MPI_DOUBLE, rootRank, cart_comm);

    if(coords[1] == 0 && coords[0] == 0) {

        for(int y = 0; y < rows; y++){
            for(int x = 0; x < cols; x++){
                //C->insertBlock(Matrix(,,&recvBuffer[]),x,y);
            }
        }

        std::cout << C->toString();

        delete[] recvBuffer;
        delete[] offsets;
        delete[] recvCount;
        delete C;
    }

    MPI_Comm_free(&row_comm);
    MPI_Comm_free(&col_comm);
    MPI_Comm_free(&cart_comm);
    MPI_Finalize();
    return 0;
}