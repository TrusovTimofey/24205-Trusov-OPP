#include <mpi.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cmath>

class Vector{
private:
    int _size;
    double* _values = nullptr;

public:
    Vector(int size): _size(size){
        if(size <= 0) throw std::invalid_argument("Index must be a positive integer");
        _values = new double[size]();
    }
    ~Vector(){
        if(_values == nullptr) return;
        delete _values;
        _values = nullptr;
    }

    void fill(){
        for(int i = 0; i < _size; i++){
            _values[i] = _size+1;
        }
    }

    int size() const{
        return _size;
    }

    double& operator() (int i){
        if(_values == nullptr) throw std::runtime_error("Use after free");
        if(i < 0 || i >= _size) throw std::invalid_argument("Index out of range");
        return _values[i];
    }

    Vector& operator-= (const Vector& other){
        if(_values == nullptr) throw std::runtime_error("Use after free");
        if(other._values == nullptr) throw std::invalid_argument("Vector is empty");
        if(other._size != _size) throw std::invalid_argument("Vectors must have the same size");
        for(int i = 0; i < _size; i++){
            _values[i]-= other._values[i];
        }
        return *this;
    }

    Vector& operator*= (double value){
        if(_values == nullptr) throw std::runtime_error("Use after free");
        for(int i = 0; i < _size; i++){
            _values[i]*= value;
        }
        return *this;
    }

    Vector& operator= (const Vector& other){
        if(other._values == nullptr) throw std::invalid_argument("Vector is empty");
        if(_size != other._size){
            if(_values) delete _values;
            _values = new double[other.size()];
            _size = other._size;
        }
        std::memcpy(_values, other._values, sizeof(double)*_size);
        return *this;
    }

    double norm() const{
        if(_values == nullptr) throw std::runtime_error("Use after free");
        double value = 0;
        for(int i = 0; i < _size; i++){
            value+= _values[i] * _values[i];
        }
        return std::sqrt(value);
    }

    std::string toString() const{
        if(_values == nullptr) throw std::runtime_error("Use after free");
        std::string str = "(";
        for(int i = 0; i < _size; i++){
            str+=std::to_string(_values[i]) + ", ";
        }
        str+=")";
        return str;
    }
};

class Matrix{
private:
    int _sizeX;
    int _sizeY;
    double* _values = nullptr;

    double& get(int x, int y){
        return _values[y*_sizeX + x];
    }
    const double& get(int x, int y) const{
        return _values[y*_sizeX + x];
    }

public:
Matrix(int sizeX, int sizeY): _sizeX(sizeX), _sizeY(sizeY) {
    if(sizeX <= 0 || sizeY <=0) throw std::invalid_argument("Matrix size must be greater then zero");
    _values = new double[sizeX*sizeY];
}

~Matrix(){
    if(_values == nullptr) return;
    delete _values;
    _values = nullptr;
}

void fill(){
    for(int y = 0; y < _sizeY; y++){
        for(int x = 0; x < _sizeX; x++){
            get(x,y) = x==y ? 2 : 1;
        }
    }
}

int sizeX() const{
    return _sizeX;
}
int sizeY() const{
    return _sizeY;
}

double& operator() (int x, int y){
    if(_values == nullptr) throw std::runtime_error("Use after free");
    if(x < 0 || y < 0 || x >= _sizeX || y >= _sizeY ) throw std::invalid_argument("Index out of range");
    return get(x,y);
}

Vector& operator* (Vector& other) const{
    if(_values == nullptr) throw std::runtime_error("Use after free");
    if(other.size() != _sizeX) throw std::invalid_argument("Vector size must be equal to matrix x size");
    Vector temp(_sizeX);
    int size=std::min(_sizeX,_sizeY);
    for(int y = 0; y < size; y++){
        for(int x = 0; x < _sizeX; x++){
            temp(y) += get(x,y) * other(x);
        }
    }
    if(_sizeX == _sizeY){
        other=temp;
        return other;
    }
    for(int i = 0; i < size; i++){
        other(i) = temp(i);
    }
    return other;
}

std::string to_string() const{
    if(_values == nullptr) throw std::runtime_error("Use after free");
    std::string str;
    for(int y = 0; y < _sizeY; y++){
        str+="| ";
        for(int x = 0; x < _sizeX; x++){
            str+=std::to_string(get(x,y)) + ", ";
        }
        str+="|\n";
    }
    return str;
}

};

class SimpleIterator{
public:
    static Vector solve(const Matrix& A, const Vector& b){
        if(A.sizeX() != b.size()) throw std::invalid_argument("Conteiners must have the same size");
        const double epsilon = 0.00001;
        double tau  = 0.001; 

        Vector xNext(b.size());
        Vector xPrev(b.size());
        double bNorm = 1/b.norm();
        double prevNorm = 1/epsilon;
        bool isLess = false;
        do{
            A*xPrev;
            xPrev -= b;

            double norm = xPrev.norm()*bNorm;
            isLess = norm < epsilon;

            if(prevNorm < norm) tau*=-1;
            prevNorm = norm;

            xPrev*=tau;
            xNext-=xPrev;

            xPrev = xNext;
        }while(!isLess);
        return xNext;
    }
};


int main(int argc, char** argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("i`m a %d of %d\n",rank+1,size);

    int N = 3;

    Matrix A(N,N);
    Vector b(N);
    A.fill();
    b.fill();

    Vector x = SimpleIterator::solve(A,b);

    std::cout << x.toString();

    MPI_Finalize();
    return 0;
}
