#include <mpi.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include <fstream>

static int RANK=0;
static int SIZE=0;

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
        delete[] _values;
        _values = nullptr;
    }

    Vector(const Vector& other) : _size(other._size), _values(new double[other._size]) {
        std::memcpy(_values, other._values, sizeof(double) * _size);
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

    Vector split(int part, int parts) const {
        if(parts > size()) throw std::invalid_argument("can`t split vector to less parts than it`s size");
        if(part >= parts) throw std::invalid_argument("part can`t be greater than parts");
        if(part < 0) throw std::invalid_argument("part can`t be less than zero");
        if(parts < 1) throw std::invalid_argument("parts can`t be less than one");

        int vSize = _size/parts;
        int mod = _size%parts;
        int index = 0;

        if(part < mod) vSize++;  
        else index = mod;

        index += vSize*part;

        Vector vec(vSize);

        std::memcpy(vec._values, &_values[index], sizeof(double)*vSize);

        return vec;
    }

    void insert(int part, int parts, const Vector& other){
        if(parts > size()) throw std::invalid_argument("can`t split vector to less parts than it`s size");
        if(part >= parts) throw std::invalid_argument("part can`t be greater than parts");
        if(part < 0) throw std::invalid_argument("part can`t be less than zero");
        if(parts < 1) throw std::invalid_argument("parts can`t be less than one");

        int vSize = _size/parts;
        int mod = _size%parts;
        int index = 0;

        if(part < mod) vSize++;  
        else index = mod;

        index += vSize*part;
        
        std::memcpy(&_values[index], other._values, sizeof(double)*vSize);
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
        _values = new double[sizeX*sizeY];
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

    Vector operator*(const Vector& other) const{
        if(_values == nullptr) throw std::runtime_error("Use after free");
        if(other.size() != _sizeX) throw std::invalid_argument("Vector size must be equal to matrix x size");
        int size=std::min(_sizeX,_sizeY);
        Vector temp(size);
        for(int y = 0; y < size; y++){
            for(int x = 0; x < _sizeX; x++){
                temp(y) += get(x,y) * other(x);
            }
        }
        return temp;
    }

    Matrix split(int part, int parts){
        if(parts > sizeY()) throw std::invalid_argument("can`t split Matrix to less parts than it`s size");
        if(part >= parts) throw std::invalid_argument("part can`t be greater than parts");
        if(part < 0) throw std::invalid_argument("part can`t be less than zero");
        if(parts < 1) throw std::invalid_argument("parts can`t be less than one");

        int ySize = _sizeY/parts;
        int mod = _sizeY%parts;
        int index = 0;

        if(part < mod) ySize++;  
        else index = mod;

        index += ySize*part;

        Matrix mat(sizeX(), ySize);
        
        std::memcpy(mat._values, &(get(0,index)), sizeof(double)*_sizeX*ySize);

        return mat;
    }

    void insert(int part, int parts, const Matrix& other){
        if(parts > sizeY()) throw std::invalid_argument("can`t split Matrix to less parts than it`s size");
        if(part >= parts) throw std::invalid_argument("part can`t be greater than parts");
        if(part < 0) throw std::invalid_argument("part can`t be less than zero");
        if(parts < 1) throw std::invalid_argument("parts can`t be less than one");

        int ySize = _sizeY/parts;
        int mod = _sizeY%parts;
        int index = 0;

        if(part < mod) ySize++;  
        else index = mod;

        index += ySize*part;

        
        std::memcpy(&(get(0,index)), other._values, sizeof(double)*_sizeX*ySize);
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
        const double epsilon = 0.00001;
        double tau = 1.9/(A.sizeX()+1);
        
        Vector x(A.sizeX());        
        double bNorm = 1/b.norm();
        double prevNorm = 1/epsilon;
        bool isLess = false;
        
        while (!isLess) {
            MPI_Bcast(x.data(), x.size(), MPI_DOUBLE, 0, MPI_COMM_WORLD);

            Vector localResult = A*x;
            localResult -= b;
            
            if (RANK == 0) {
                Vector fullResult(x.size());
                fullResult.insert(0, SIZE, localResult);
                
                for (int source = 1; source < SIZE; source++) {
                    int partSize;
                    MPI_Recv(&partSize, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    Vector tempPart(partSize);
                    MPI_Recv(tempPart.data(), partSize, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    fullResult.insert(source, SIZE, tempPart);
                }
                
                double norm = fullResult.norm() * bNorm;
                if (prevNorm < norm) tau *= -1;
                prevNorm = norm;
                
                fullResult *= tau;
                x -= fullResult;
                
                isLess = norm < epsilon;

            } else {
                int partSize = localResult.size();
                MPI_Send(&partSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(localResult.data(), partSize, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
            }
            
            MPI_Bcast(&isLess, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);
        }
        
        return x;
    }
};


std::chrono::microseconds Calculate(){

    std::chrono::microseconds duration;

    int N = 10000;

    if(RANK == 0){
        Matrix A(N,N);
        Vector b(N);
        A.fill();
        b.fill();
        
        auto start = std::chrono::high_resolution_clock::now();

        Matrix localA = A.split(0,SIZE);
        Vector localB = b.split(0,SIZE);
        
        for (int i = 1; i < SIZE; i++)
        {
            Matrix local = A.split(i,SIZE);
            int localSize = local.sizeY();
            MPI_Send(&localSize,1,MPI_INT,i,0,MPI_COMM_WORLD);
            MPI_Send(local.data(),N*localSize,MPI_DOUBLE,i,1,MPI_COMM_WORLD);
            MPI_Send(b.split(i,SIZE).data(),localSize,MPI_DOUBLE,i,2,MPI_COMM_WORLD);
        }

        Vector x = SimpleIterator::solve(localA,localB);

        auto end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
    }
    else{
        int localSize;
        MPI_Recv(&localSize,1,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        
        Matrix localA(N,localSize);
        MPI_Recv(localA.data(),N*localSize,MPI_DOUBLE,0,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        Vector localB(localSize);
        MPI_Recv(localB.data(),localSize,MPI_DOUBLE,0,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

        SimpleIterator::solve(localA,localB);
    }

    return duration;
}


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &SIZE);
    MPI_Comm_rank(MPI_COMM_WORLD, &RANK);

    std::chrono::microseconds min = std::chrono::microseconds::max();
    for(int i=0; i < 10; i++){
        auto duration = Calculate();
        if(min > duration) min = duration;
    }
    
    if(RANK == 0){
        std::ofstream file("V1_test.csv", std::ios::app | std::ios::out);
        if (file.is_open()) {
            file << std::to_string(SIZE) << "," << (min.count()*0.001) << std::endl;
            file.close();
        } 
        else std::cerr << "Не удалось открыть файл" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
