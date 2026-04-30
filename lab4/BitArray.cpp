#include "BitArray.h"
#include <cstring>

#pragma region ConstructorsDestrucor

BitArray::BitArray()
{
}
BitArray::BitArray(const BitArray& b)
{
	*this = b;
}
BitArray::BitArray(int num_bits, unsigned long value)
{
	if (num_bits <= 0) throw std::invalid_argument("num_bits must be greater then zero");

	bytesAllocated = num_bits / 8 + ((num_bits % 8) > 0);
	array = (unsigned char*)malloc(bytesAllocated);
	if (array == nullptr) throw std::bad_alloc();
	bitsCount = num_bits;

	if (num_bits < 8 * sizeof(long)) value <<= 8 * sizeof(long) - num_bits;
	unsigned char* bytes = (unsigned char*)(&value);
	for (int i = 0; i < bytesAllocated && i < sizeof(long); ++i) array[i] = bytes[sizeof(long) - i - 1];
}

BitArray::~BitArray()
{
	free(array);
	array = nullptr;
	bytesAllocated = 0;
	bitsCount = 0;
}

#pragma endregion

#pragma region Interacting

void BitArray::setBit(unsigned char* b, int index, bool v) {
	setBit(b[index / 8], index % 8, v);
}
void BitArray::setBit(unsigned char& b, int index, bool v) {
	unsigned char mask = 0b10000000 >> index;
	if (v) b |= mask;
	else b &= ~mask;
}

bool BitArray::getBit(const unsigned char* b, int index) const {
	return getBit(b[index / 8], index % 8);
}
bool BitArray::getBit(unsigned char b, int index) const {
	unsigned char mask = 0b10000000 >> index;
	return (mask & b);
}

BitArray& BitArray::set(int n, bool val) {
	if (n >= bitsCount || n < 0) throw std::out_of_range("Array out of bounds");
	setBit(array, n, val);
	return *this;
}
BitArray& BitArray::set() {
	for (int i = 0; i < bytesAllocated; i++)
	{
		array[i] = 0b11111111;
	}
	return *this;
}

BitArray& BitArray::reset(int n) {
	if (n >= bitsCount || n < 0) throw std::out_of_range("Array out of bounds");
	setBit(array, n, false);
	return *this;
}
BitArray& BitArray::reset() {
	for (int i = 0; i < bytesAllocated; i++)
	{
		array[i] = 0;
	}
	return *this;
}

void BitArray::swap(BitArray& b)
{
	int allocTmp = bytesAllocated;
	int bitsTmp = bitsCount;
	auto* arrayTmp = array;
	bytesAllocated = b.bytesAllocated;
	bitsCount = b.bitsCount;
	array = b.array;
	b.bytesAllocated = allocTmp;
	b.bitsCount = bitsTmp;
	b.array = arrayTmp;
}

void BitArray::resize(int num_bits, bool value) {
	if (num_bits <= 0) throw std::invalid_argument("num_bits must be greater then zero");
	if ((num_bits <= bitsCount) || (bytesAllocated * 8 < num_bits))
	{
		int newAlloc = num_bits / 8 + ((num_bits % 8) > 0);
		unsigned char* chars = (unsigned char*)malloc(newAlloc);
		if (chars == nullptr) throw std::bad_alloc();
		memcpy(chars, array, bytesAllocated > newAlloc ? newAlloc : bytesAllocated);
		free(array);
		array = chars;
		bytesAllocated = newAlloc;
	}
	int bitsAllocated = bytesAllocated * 8;
	for (int i = bitsCount; i < bitsAllocated; i++) setBit(array, i, value);
	bitsCount = num_bits;
}

void BitArray::clear() {
	free(array);
	array = nullptr;
	bytesAllocated = 0;
	bitsCount = 0;
}

void BitArray::push_back(bool bit) {
	if (bytesAllocated * 8 <= bitsCount) {
		int oldBitsCount = bitsCount;
		resize(bitsCount * 2 + 1);
		bitsCount = oldBitsCount;
	}
	set(bitsCount++, bit);
}

#pragma endregion

#pragma region Informational

bool BitArray::any() const {
	int fullBytes = bitsCount / 8;
	for (int i = 0; i < fullBytes; i++)
	{
		if (array[i] != 0)return true;
	}
	for (int i = fullBytes * 8; i < bitsCount; i++)
	{
		if (getBit(array, i)) return true;
	}
	return false;
}
bool BitArray::none() const {
	return !any();
}

int BitArray::count() const {
	int count = 0;
	for (int i = 0; i < bitsCount; i++)
	{
		count += getBit(array, i);
	}
	return count;
}

int BitArray::size() const {
	return bitsCount;
}

bool BitArray::empty() const {
	return bitsCount == 0;
}

std::string BitArray::to_string() const {
	std::string str;
	for (int i = 0; i < bitsCount; i++)
	{
		bool bit = getBit(array, i);
		str.push_back(bit ? '1' : '0');
	}
	return str;
}

#pragma endregion

#pragma region Operators

BitArray BitArray::operator~() const {
	BitArray invers = BitArray(bitsCount, 0);
	for (int i = 0; i < bytesAllocated; i++)
	{
		invers.array[i] = ~array[i];
	}
	return invers;
}

BitArray& BitArray::operator=(const BitArray& b)
{
	if (this == &b) return *this;
	free(array);
	if (b.bytesAllocated == 0) {
		array = nullptr;
		bytesAllocated = 0;
		bitsCount = 0;
		return *this;
	}
	array = (unsigned char*)malloc(b.bytesAllocated);
	if (array == nullptr)
	{
		bitsCount = 0;
		bytesAllocated = 0;
		throw std::bad_alloc();
	}
	bitsCount = b.bitsCount;
	bytesAllocated = b.bytesAllocated;
	memcpy(array, b.array, bytesAllocated);
	return *this;
}
BitArray& BitArray::operator&=(const BitArray& b) {
	if (bitsCount != b.bitsCount) throw std::invalid_argument("Arrays of different sizes");
	for (int i = 0; i < bytesAllocated; i++)
	{
		array[i] &= b.array[i];
	}
	return *this;
}
BitArray& BitArray::operator|=(const BitArray& b) {
	if (bitsCount != b.bitsCount) throw std::invalid_argument("Arrays of different sizes");
	for (int i = 0; i < bytesAllocated; i++)
	{
		array[i] |= b.array[i];
	}
	return *this;
}
BitArray& BitArray::operator^=(const BitArray& b) {
	if (bitsCount != b.bitsCount) throw std::invalid_argument("Arrays of different sizes");
	for (int i = 0; i < bytesAllocated; i++)
	{
		array[i] ^= b.array[i];
	}
	return *this;
}

BitArray& BitArray::operator<<=(int n) {
	for (int i = 0; i < bitsCount - n; i++)
	{
		set(i, getBit(array, i + n));
	}
	for (int i = bitsCount - n; i < bitsCount; i++)
	{
		reset(i);
	}
	return *this;
}
BitArray& BitArray::operator>>=(int n) {
	for (int i = bitsCount - 1; i >= n; --i)
	{
		set(i, getBit(array, i - n));
	}
	for (int i = 0; i < n; i++)
	{
		reset(i);
	}
	return *this;
}
BitArray BitArray::operator<<(int n) const {
	BitArray arr = BitArray(bitsCount, 0);
	for (int i = 0; i < bitsCount - n; i++)
	{
		arr.set(i, getBit(array, i + n));
	}
	for (int i = bitsCount - n; i < bitsCount; i++)
	{
		arr.reset(i);
	}
	return arr;
}
BitArray BitArray::operator>>(int n) const {
	BitArray arr = BitArray(bitsCount, 0);
	for (int i = 0; i < n; i++)
	{
		arr.reset(i);
	}
	for (int i = n; i < bitsCount; i++)
	{
		arr.set(i, getBit(array, i - n));
	}
	return arr;
}

BitArrayIndexOperator BitArray::operator[](int i) const{
	if (i >= bitsCount || i < 0) throw std::out_of_range("Array out of bounds");
	BitArrayIndexOperator oper(array[i / 8], i%8);
	return oper;
}
bool BitArrayIndexOperator::operator=(bool v) {
	unsigned char mask = 0b10000000 >> index;
	if (v) byte |= mask;
	else byte &= ~mask;
	return v;
}
BitArrayIndexOperator::operator bool() const {
	unsigned char mask = 0b10000000 >> index;
	return byte & mask;
}

bool operator==(const BitArray& a, const BitArray& b) {
	if (a.size() != b.size()) return false;
	for (int i = 0; i < a.size(); i++)
	{
		if ((bool)(a[i]) != (bool)(b[i])) return false;
	}
	return true;
}
bool operator!=(const BitArray& a, const BitArray& b) {
	return !(a == b);
}

BitArray operator&(const BitArray& b1, const BitArray& b2) {
	BitArray arr(b1);
	arr &= b2;
	return arr;
}
BitArray operator|(const BitArray& b1, const BitArray& b2) {
	BitArray arr(b1);
	arr |= b2;
	return arr;
}
BitArray operator^(const BitArray& b1, const BitArray& b2) {
	BitArray arr(b1);
	arr ^= b2;
	return arr;
}

#pragma endregion