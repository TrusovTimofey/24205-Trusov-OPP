#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include <vector>
#include <algorithm>
#include <omp.h>
#include <random>

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
    ~Vector(){
        if(_values == nullptr) return;
        delete[] _values;
        _values = nullptr;
    }

    double* data(){
        return _values;
    }

    void fill(){
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dis(0.0, 1.0);

        #pragma omp parallel for schedule(static)
        for(int i = 0; i < _size; i++){
            _values[i] = dis(gen);
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
        
        #pragma omp parallel for schedule(runtime)
        for(int i = 0; i < _size; i++){
            _values[i] -= other._values[i];
        }
        return *this;
    }

    Vector& operator*= (double value){
        if(_values == nullptr) throw std::runtime_error("Use after free");
        
        #pragma omp parallel for schedule(runtime)
        for(int i = 0; i < _size; i++){
            _values[i] *= value;
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
        
        #pragma omp parallel for reduction(+:value) schedule(runtime)
        for(int i = 0; i < _size; i++){
            value += _values[i] * _values[i];
        }
        return std::sqrt(value);
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

    void fill(){

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dis(0.0, 1.0);

        #pragma omp parallel for collapse(2) schedule(static)
        for(int y = 0; y < _sizeY; y++){
            for(int x = 0; x < _sizeX; x++){
                get(x,y) = dis(gen);
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

        #pragma omp parallel for schedule(runtime)
        for(int y = 0; y < _sizeY; y++){
            double sum = 0.0;
            for(int x = 0; x < _sizeX; x++){
                sum += get(x,y) * vec(x);
            }
            result(y) = sum;
        }
    }

};

int main(int argc, char** argv) {
    int N = 20000;
    
    int num_threads = 12;
    if (argc > 1) {
        num_threads = std::atoi(argv[1]);
    }
    omp_set_num_threads(num_threads);
    
    std::cout << "threads: " << omp_get_max_threads() << std::endl;

    struct ScheduleType {
        std::string name;
        omp_sched_t type;
        int chunk;
    };
    
    std::vector<ScheduleType> schedules = {
        {"static,chunk=0", omp_sched_static, 0},
        {"static,chunk=1", omp_sched_static, 1},
        {"static,chunk=10", omp_sched_static, 10},
        {"static,chunk=100", omp_sched_static, 100},
        {"static,chunk=1000", omp_sched_static, 1000},
        {"dynamic,chunk=0", omp_sched_dynamic, 0},
        {"dynamic,chunk=1", omp_sched_dynamic, 1},
        {"dynamic,chunk=10", omp_sched_dynamic, 10},
        {"dynamic,chunk=100", omp_sched_dynamic, 100},
        {"dynamic,chunk=1000", omp_sched_dynamic, 1000},
        {"guided,chunk=0", omp_sched_guided, 0},
        {"guided,chunk=1", omp_sched_guided, 1},
        {"guided,chunk=10", omp_sched_guided, 10},
        {"guided,chunk=100", omp_sched_guided, 100},
        {"guided,chunk=100", omp_sched_guided, 100},
        {"auto", omp_sched_auto, 0}
    };

    for (const auto& sched : schedules) {
        omp_set_schedule(sched.type, sched.chunk);

        std::cout << sched.name << ": \n";

        Matrix A(N, N);
        Vector b(N);
        Vector temp(N);
    
        A.fill();
        b.fill();
        temp.fill();

        std::chrono::microseconds duration = std::chrono::microseconds::max();
        for(int i = 0; i < 100; i++){

            auto start = std::chrono::high_resolution_clock::now();
        
            temp-=b;
            
            auto end = std::chrono::high_resolution_clock::now();
            auto local = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            duration = duration > local ? local : duration;
        }
        std::cout << "operator-= time: " << (duration.count() ) << " ms" << std::endl;

        duration = std::chrono::microseconds::max();
        for(int i = 0; i < 100; i++){

            auto start = std::chrono::high_resolution_clock::now();
        
            temp*=-1;
            
            auto end = std::chrono::high_resolution_clock::now();
            auto local = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            duration = duration > local ? local : duration;
        }
        std::cout << "operator*= time: " << (duration.count() ) << " ms" << std::endl;

        duration = std::chrono::microseconds::max();
        for(int i = 0; i < 100; i++){

            auto start = std::chrono::high_resolution_clock::now();
        
            temp.norm();
            
            auto end = std::chrono::high_resolution_clock::now();
            auto local = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            duration = duration > local ? local : duration;
        }
        std::cout << "norm time: " << (duration.count() ) << " ms" << std::endl;

        duration = std::chrono::microseconds::max();
        for(int i = 0; i < 100; i++){

            auto start = std::chrono::high_resolution_clock::now();
        
            A.multiply(b,temp);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto local = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            duration = duration > local ? local : duration;
        }
        std::cout << "multiply time: " << (duration.count() ) << " ms" << std::endl;

        std::cout << std::endl;
    }

    return 0;
}