#pragma once

#include <type_traits>
#include <vector>
#include <cassert>
#include <bitset>
#include <cstdint>

/** 
* @brief A customized bitmap implementation
* It uses a bit array to store the allocation information of a certain element array.
* Support finding the kth set bit from a given element index
*
* @tparam Block_Size The size of each block in the bitmap.
*/
template <int32_t Block_Size = 32>
class Bitmap
{
    // Determine the Block_Type based on Block_Size,
    // support unsigned char, unsigned short, unsigned int, unsigned long long as the Block_Type
    using BlockType = std::conditional_t<Block_Size <= 8, 
        unsigned char,
        std::conditional_t<Block_Size <= 16, 
            unsigned short,
            std::conditional_t<Block_Size <= 32, 
                unsigned int,
                unsigned long long
            >
        >
    >;

public:
    Bitmap() : data_() {}

    Bitmap(uint64_t element_capacity) 
        : element_capacity_(element_capacity), 
          array_size_((element_capacity + Block_Size - 1) / Block_Size),
          data_((element_capacity + Block_Size - 1) / Block_Size) {}

    void resize(uint64_t element_capacity) {
        element_capacity_ = element_capacity;
        array_size_ = (element_capacity + Block_Size - 1) / Block_Size;
        data_.resize(array_size_);
    }

    /**
     * @brief Sets the bit at the specified element index to 1.
     *
     * @param element_index The index of the element to set.
     *
     */
    void set(uint64_t element_index)
    {
        assert(element_index < element_capacity_);
        uint64_t array_index = element_index / Block_Size;
        uint64_t bit_index = element_index % Block_Size;
        data_[array_index] |= (BlockType(1) << bit_index); // Set the specific bit to 1
    }

    /**
     * @brief Unsets the bit at the specified element index to 0.
     *
     * @param element_index The index of the element to unset.
     *
     */
    void unset(uint64_t element_index)
    {
        assert(element_index < element_capacity_);
        uint64_t array_index = element_index / Block_Size;
        uint64_t bit_index = element_index % Block_Size;
        data_[array_index] &= ~(BlockType(1) << bit_index); // Set the specific bit to 0
    }


    /**
     * Retrieves the value of the bit at the specified element index.
     *
     * @param element_index The index of the element to retrieve the bit value from.
     *
     * @return The value of the bit at the specified element index (0 or 1).
     *
     */
    bool get(uint64_t element_index) const
    {
        assert(element_index < element_capacity_);
        uint64_t array_index = element_index / Block_Size;
        uint64_t bit_index = element_index % Block_Size;
        return (data_[array_index] >> bit_index) & 1; // Return the bit value (0 or 1)
    }

    /**
     * @brief
     Count the 1's in [start_element_index, end_element_index]
     Set the elements to 0
     */
    uint64_t getPopCountAndUnsetBatch(uint64_t start_element_index, uint64_t end_element_index) {
        uint64_t start_array_index = start_element_index / Block_Size;
        uint64_t start_bit_index = start_element_index % Block_Size;
        uint64_t end_array_index = end_element_index / Block_Size;
        uint64_t end_bit_index = end_element_index % Block_Size;
        uint64_t pop_count = 0;

        BlockType start_mask = BlockType(-1) << (start_bit_index);
        BlockType end_mask = BlockType(-1) >> (Block_Size - 1 - end_bit_index);

        // count start bits in the first block using mask and set the bits to 0
        if(start_array_index == end_array_index) {
            // If start and end are in the same block, create a mask for the range
            BlockType mask = start_mask & end_mask;
            pop_count += countOnes(data_[start_array_index] & mask);
            data_[start_array_index] &= ~mask;
            return pop_count;
        }

        pop_count += countOnes(data_[start_array_index] & start_mask);
        data_[start_array_index] &= ~(start_mask);
        // count blocks in batch using pop_count instructions and set the blocks to 0
        for(uint64_t i = start_array_index + 1; i != end_array_index; i = (i + 1) % array_size_) {
            pop_count += countOnes(data_[i]);
            data_[i] = 0;
        }
        // count end bits in the last block using mask and set the bits to 0
        pop_count += countOnes(data_[end_array_index] & end_mask);
        data_[end_array_index] &= ~(end_mask);
        return pop_count;
    }

private:
    std::vector<BlockType> data_; // Store the bitmap as a vector of BlockTypes
    uint64_t element_capacity_ = 0;
    uint64_t array_size_ = 0;

    /**
     * @brief Counts the number of set bits in the given integer type.
     * It uses __builtin_popcount or its equivalents for different integer types.
     * These functions are supported by the compiler as hardware accelerated instructions.
     * 
     * @param num The integer for which the number of set bits is to be counted.
     *
     * @return The number of set bits in the given integer.
     *
     */
    static uint64_t countOnes(BlockType num)
    {
        static_assert(std::is_same_v<BlockType, decltype(num)>, "Implicit type conversion detected");

        if constexpr (std::is_same_v<BlockType, char> || std::is_same_v<BlockType, unsigned char>)
        {
            return __builtin_popcount(static_cast<int>(num));
        }
        else if constexpr (std::is_same_v<BlockType, short> || std::is_same_v<BlockType, unsigned short>)
        {
            return __builtin_popcount(static_cast<int>(num));
        }
        else if constexpr (std::is_same_v<BlockType, int> || std::is_same_v<BlockType, unsigned int>)
        {
            return __builtin_popcount(num);
        }
        else if constexpr (std::is_same_v<BlockType, long> || std::is_same_v<BlockType, unsigned long>)
        {
            return __builtin_popcountl(num);
        }
        else if constexpr (std::is_same_v<BlockType, long long> || std::is_same_v<BlockType, unsigned long long>)
        {
            return __builtin_popcountll(num);
        }
        else
        {
            return -1; // Return -1 if the type is not supported
        }
    }
};

/**
 * @brief This is a naive implementation for the bitmap, for testing purposes.
 */
class BitmapNaive
{
public:
    // Initialize a naive bitmap with `element_capacity_` bits.
    BitmapNaive(uint64_t element_capacity)
        : data_(element_capacity, 0) {} // Initialize with all bits unset (0)

    // Set the bit at `element_index` to 1
    void set(uint64_t element_index)
    {
        assert(element_index < data_.size());
        data_[element_index] = 1; // Set the specific bit to 1
    }

    void unset(uint64_t element_index)
    {
        assert(element_index < data_.size());
        data_[element_index] = 0; // Set the specific bit to 0
    }

    // Get the bit at `element_index`
    bool get(uint64_t element_index) const
    {
        assert(element_index < data_.size());
        return data_[element_index]; // Return the bit value (0 or 1)
    }

private:
    std::vector<int8_t> data_; // Use one char per bit to store the bitmap
};