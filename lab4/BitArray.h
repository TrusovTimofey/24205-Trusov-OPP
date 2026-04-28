#pragma once

#include <iostream>
#include <string>

class BitArrayIndexOperator {
private:
	unsigned char index;
	unsigned char& byte;

public:
	BitArrayIndexOperator(unsigned char& byte, unsigned char index) : byte(byte), index(index) {};
	bool operator=(bool v);
	operator bool() const;
};

class BitArray
{
private:
	unsigned char* array = nullptr;
	int bytesAllocated = 0;
	int bitsCount = 0;

	/**
	* @brief Sets a single bit in a byte array to the specified value.
	* @param b Pointer to the byte array.
	* @param index 0-based index of the Bit to set.
	* @param v Value to set .
	*/
	void setBit(unsigned char* b, int index, bool v);

	/**
	* @brief Sets a single bit in a byte to the specified value.
	* @param b Reference to the byte to modify.
	* @param index Index of the Bit in the byte in range 0-7.
	* @param v Value to set.
	*/
	void setBit(unsigned char& b, int index, bool v);

	/**
	* @brief Gets the value of a single bit from a byte array.
	* @param b Pointer to the byte array.
	* @param index 0-based index of the Bit to set.
	* @return Boolean value of the specified bit.
	*/
	bool getBit(const unsigned char* b, int index) const;

	/**
	* @brief Gets the value of a single bit from a byte.
	* @param b The byte to read from.
	* @param index Index of the Bit in the byte in range 0-7.
	* @return Boolean value of the specified bit.
	*/
	bool getBit(unsigned char b, int index) const;

public:

#pragma region ConstructorsDestructor

	/**
	* @brief Creates an empty BitArray.
	*/
	BitArray();

	/**
	* @brief Creates a BitArray as a copy of another BitArray.
	* @param b The BitArray to copy from.
	* @throws std::bad_alloc if memory allocation fails.
	*/
	BitArray(const BitArray& b);

	/**
	* @brief Constructs a BitArray and initializes it with the binary representation of the provided value,
	* applying bit shifting to align the value bits according to the requested size.
	*
	* Conceptual behavior (value == 0bb₁b₂...bₙ, bᵢ∈{0,1}):
	*
	* - When num_bits == n: stores bits b₁b₂...bₙ directly
	*
	* - When num_bits == (n-k): stores bits bₖ₊₁bₖ₊₂...bₙ
	*
	* - When num_bits == (n+k): stores bits 0₁0₂...0ₖb₁b₂...bₙ
	*
	* @param num_bits Number of bits in the resulting BitArray (must be > 0)
	* @param value Unsigned long integer value whose bits will be used for initialization
	*
	* @throws std::invalid_argument if num_bits <= 0.
	* @throws std::bad_alloc if memory allocation fails.
	*/
	explicit BitArray(int num_bits, unsigned long value = 0);

	/**
	 * @brief Releases allocated memory.
	 */
	~BitArray();

#pragma endregion

#pragma region Interacting

	/**
	* @brief Sets a specific bit to the given value.
	* @param n 0-based Bit index to set.
	* @param val Value to set.
	* @return Reference to this BitArray.
	* @throws std::out_of_range if n >= BitArray size.
	*/
	BitArray& set(int n, bool val = true);

	/**
	* @brief Sets all bits to 1.
	* @return Reference to this BitArray.
	*/
	BitArray& set();

	/**
	* @brief Sets a specific bit to 0.
	* @param n 0-based Bit index to reset.
	* @return Reference to this BitArray.
	* @throws std::out_of_range if n >= BitArray size.
	*/
	BitArray& reset(int n);

	/**
	* @brief Sets all bits to 0.
	* @return Reference to this BitArray.
	*/
	BitArray& reset();

	/**
	* @brief Swaps the contents of this BitArray with another.
	* @param b The BitArray to swap with.
	*/
	void swap(BitArray& b);

	/**
	* @brief Resizes the BitArray to the specified number of bits.
	* @param num_bits New size in bits. Must be greater than zero.
	* @param value Value for any new bits added.
	* @throws std::invalid_argument if num_bits <= 0.
	* @throws std::bad_alloc if memory allocation fails.
	*/
	void resize(int num_bits, bool value = false);

	/**
	* @brief Clears the BitArray, releasing all memory and setting size to zero.
	*/
	void clear();

	/**
	* @brief Appends a bit to the end of the BitArray.
	* @param bit The bit value to append.
	* @throws std::bad_alloc if memory allocation fails during resize.
	*/
	void push_back(bool bit);

#pragma endregion

#pragma region Informational

	/**
	* @brief Checks if any bit is set to 1.
	* @return true if at least one bit is 1, false otherwise.
	*/
	bool any() const;

	/**
	* @brief Checks if all bits are set to 0.
	* @return true if all bits are 0, false otherwise.
	*/
	bool none() const;

	/**
	* @brief Counts the number of bits set to 1.
	* @return The number of bits with value 1.
	*/
	int count() const;

	/**
	* @brief Gets the number of bits in the array.
	* @return The size of the BitArray in bits (may differ from sizeof() due to alignment).
	*/
	int size() const;

	/**
	* @brief Checks if the BitArray is empty.
	* @return true if the BitArray is empty, false otherwise.
	*/
	bool empty() const;

	/**
	* @brief Converts the BitArray to a string representation.
	* @return String where each character is '1' or '0' representing the bits.
	*/
	std::string to_string() const;

#pragma endregion

#pragma region Operators

	/**
	* @brief Bitwise NOT operator. Returns a new BitArray with all bits inverted.
	* @return New BitArray with inverted bits.
	*/
	BitArray operator~() const;

	/**
	* @brief Assignment operator. Performs deep copy.
	* @param b The BitArray to copy from.
	* @return Reference to this BitArray.
	* @throws std::bad_alloc if memory allocation fails.
	*/
	BitArray& operator=(const BitArray& b);

	/**
	* @brief Bitwise AND assignment operator.
	* @param b The BitArray to AND with (must have the same size).
	* @return Reference to this BitArray.
	* @throws std::invalid_argument if BitArrays have different sizes.
	*/
	BitArray& operator&=(const BitArray& b);

	/**
	* @brief Bitwise OR assignment operator.
	* @param b The BitArray to OR with (must have the same size).
	* @return Reference to this BitArray.
	* @throws std::invalid_argument if BitArrays have different sizes.
	*/
	BitArray& operator|=(const BitArray& b);

	/**
	* @brief Bitwise XOR assignment operator.
	* @param b The BitArray to XOR with (must have the same size).
	* @return Reference to this BitArray.
	* @throws std::invalid_argument if BitArrays have different sizes.
	*/
	BitArray& operator^=(const BitArray& b);

	/**
	* @brief Left shift assignment operator. Shifts bits to the left.
	* @param n Number of positions to shift.
	* @return Reference to this BitArray.
	* @throws std::out_of_range if n >= BitArray size or n < 0.
	*/
	BitArray& operator<<=(int n);

	/**
	* @brief Right shift assignment operator. Shifts bits to the right.
	* @param n Number of positions to shift.
	* @return Reference to this BitArray.
	* @throws std::out_of_range if n >= BitArray size or n < 0.
	*/
	BitArray& operator>>=(int n);

	/**
	* @brief Left shift operator. Returns a new shifted BitArray.
	* @param n Number of positions to shift.
	* @return New BitArray containing the shifted bits.
	* @throws std::out_of_range if n >= BitArray size or n < 0.
	*/
	BitArray operator<<(int n) const;

	/**
	* @brief Right shift operator. Returns a new shifted BitArray.
	* @param n Number of positions to shift.
	* @return New BitArray containing the shifted bits.
	* @throws std::out_of_range if n >= BitArray size or n < 0.
	*/
	BitArray operator>>(int n) const;

	/**
	* @brief Array subscript operator for const access.
	* @param i 0-based Bit index to access.
	* @return Boolean value of the specified bit.
	* @throws std::out_of_range if i >= BitArray size or i < 0.
	*/
	BitArrayIndexOperator operator[](int i) const;
};

/**
* @brief Equality operator.
* @param a First BitArray to compare.
* @param b Second BitArray to compare.
* @return true if all bits are equal, false otherwise.
*/
bool operator==(const BitArray& a, const BitArray& b);

/**
* @brief Inequality operator.
* @param a First BitArray to compare.
* @param b Second BitArray to compare.
* @return true if any bits differ, false otherwise.
*/
bool operator!=(const BitArray& a, const BitArray& b);

/**
* @brief Bitwise AND operator.
* @param b1 First BitArray.
* @param b2 Second BitArray.
* @return New BitArray result of b1 AND b2.
* @throws std::invalid_argument if BitArrays have different sizes.
*/
BitArray operator&(const BitArray& b1, const BitArray& b2);

/**
* @brief Bitwise OR operator.
* @param b1 First BitArray.
* @param b2 Second BitArray.
* @return New BitArray result of b1 OR b2.
* @throws std::invalid_argument if BitArrays have different sizes.
*/
BitArray operator|(const BitArray& b1, const BitArray& b2);

/**
* @brief Bitwise XOR operator.
* @param b1 First BitArray.
* @param b2 Second BitArray.
* @return New BitArray result of b1 XOR b2.
* @throws std::invalid_argument if BitArrays have different sizes.
*/
BitArray operator^(const BitArray& b1, const BitArray& b2);

#pragma endregion