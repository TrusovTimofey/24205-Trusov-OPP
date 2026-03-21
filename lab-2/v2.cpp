#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <omp.h>

class Vector{
private:
    int _size;
    double* _values = nullptr;

public:
    Vector(int size): _size(size){
        if(size <= 0) throw std::invalid_argument("Index must be a positive integer");
        _values = new double[size]();
    }
    Vector(const Vector& other) : _size(other._size), _values(new double[other._size]) {
        std::memcpy(_values, other._values, sizeof(double) * _size);
    }
    Vector(int size, double* data): _size(size){
        if(size <= 0) throw std::invalid_argument("Index must be a positive integer");
        _values = new double[size]();
        std::memcpy(_values, data, sizeof(double) * _size);
    }
    ~Vector(){
        if(_values == nullptr) return;
        delete[] _values;
        _values = nullptr;
    }

    double* data(){
        return _values;
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
    double operator() (int i) const{
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
            if(_values) delete[] _values;
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
        str+=")\n";
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
        _values = new double[sizeX*sizeY]();
    }

    ~Matrix(){
        if(_values == nullptr) return;
        delete[] _values;
        _values = nullptr;
    }

    Matrix(const Matrix& other) : _sizeX(other._sizeX), _sizeY(other._sizeY), _values(new double[other._sizeX*other._sizeY]) {
        std::memcpy(_values, other._values, sizeof(double) * _sizeX* _sizeY);
    }

    double* data(){
        return _values;
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

    double operator() (int x, int y) const{
        if(_values == nullptr) throw std::runtime_error("Use after free");
        if(x < 0 || y < 0 || x >= _sizeX || y >= _sizeY ) throw std::invalid_argument("Index out of range");
        return get(x,y);
    }

    double& operator() (int x, int y){
        if(_values == nullptr) throw std::runtime_error("Use after free");
        if(x < 0 || y < 0 || x >= _sizeX || y >= _sizeY ) throw std::invalid_argument("Index out of range");
        return get(x,y);
    }

    Matrix& operator= (const Matrix& other){
        if(other._values == nullptr) throw std::invalid_argument("Matrix is empty");
        if((_sizeX * _sizeY) != (other._sizeX * other._sizeY)){
            if(_values) delete[]_values;
            _sizeX = other._sizeX;
            _sizeY = other._sizeY;
            _values = new double[other._sizeX * other._sizeY];
        }
        std::memcpy(_values, other._values, sizeof(double)*_sizeX*_sizeY);
        return *this;
    }

    void multiply(const Vector& vec, Vector& result) const {
        if(vec.size() != _sizeX) throw std::invalid_argument("Vector size must be equal to matrix x size");
        if(result.size() != _sizeY) throw std::invalid_argument("Result vector size must be equal to matrix y size");

        for(int y = 0; y < _sizeY; y++){
            double sum = 0.0;
            for(int x = 0; x < _sizeX; x++){
                sum += get(x,y) * vec(x);
            }
            result(y) = sum;
        }
    }

    std::string toString() const{
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
    static Vector solve(const Matrix& A, const Vector& b) {
        const double epsilon = 1e-100;
        double tau = 1.5/(A.sizeX()+1);
        
        int N = A.sizeX();
        double bNorm = 1.0 / b.norm();
        
        Vector x(N);
        Vector residual(N);
        
        bool isLess = false;
        int iteration = 0;
        
        while (!isLess) {
            double localNorm = 0.0;
            #pragma omp parallel
            {

                #pragma omp for schedule(dynamic) nowait
                for(int y = 0; y < A.sizeY(); y++){
                    double sum = 0.0;
                    for(int xi = 0; xi < A.sizeX(); xi++){
                        sum += A(xi,y) * x(xi);
                    }
                    residual(y) = sum;
                }
                
                #pragma omp barrier
                #pragma omp for schedule(static)
                for(int i = 0; i < N; i++){
                    residual(i) -= b(i);
                }
                
                #pragma omp barrier
                #pragma omp for reduction(+:localNorm) schedule(static)
                for(int i = 0; i < N; i++){
                    localNorm += residual(i) * residual(i);
                }
                
                #pragma omp single
                {
                    double globalNorm = std::sqrt(localNorm) * bNorm;
                    isLess = globalNorm < epsilon;
                }
                
                #pragma omp barrier
                #pragma omp for schedule(static)
                for(int i = 0; i < N; i++){
                    x(i) -= tau * residual(i);
                }
            }
        }
        
        return x;
    }
};

bool checkRes(const Vector& x, double epsilon = 1e-5) {
    double maxDiff = 0.0;
    for (int i = 0; i < x.size(); i++) {
        double diff = std::abs(x(i) - 1.0);
        maxDiff = std::max(maxDiff, diff);
    }

    bool isGood = maxDiff < epsilon;

    if (isGood) std::cout << "check PASSED." << std::endl;
    else std::cout << "check FAILED. Max diff: " << maxDiff << std::endl;

    return isGood;
}

int main(int argc, char** argv) {
    int N = 15000;
    
    int num_threads = 12;
    if (argc > 1) {
        num_threads = std::atoi(argv[1]);
    }
    omp_set_num_threads(num_threads);
    
    std::cout << "threads: " << omp_get_max_threads() << std::endl;
    Matrix A(N, N);
    Vector b(N);

    A.fill();
    b.fill();
    
    std::chrono::microseconds duration = std::chrono::microseconds::max();
    for(int i = 0; i < 1; i++){

        auto start = std::chrono::high_resolution_clock::now();
        
        Vector x = SimpleIterator::solve(A, b);
            
        auto end = std::chrono::high_resolution_clock::now();
        auto local = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        duration = duration > local ? local : duration;
        checkRes(x);
    }
        
    std::cout << "Time: " << (duration.count() * 0.000001) << " seconds" << std::endl;

    return 0;
}