#pragma once

#include <type_traits>
#include <vector>
#include <cassert>
#include <bitset>

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
          data_((element_capacity + Block_Size - 1) / Block_Size) {}

    void resize(uint64_t element_capacity) {
        element_capacity_ = element_capacity;
        data_.resize((element_capacity + Block_Size - 1) / Block_Size);
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
     * @brief Returns the index of the kth least significant bit that is set to 1 in the bitmap from the specified element index.
     * It will traverse the bits by blocks so it's faster than the naive implementation, which will traverse bit by bit.
     * 
     * @param element_index The index of the element to start the search from.
     * @param k The kth least significant bit to find.
     *
     * @return The index of the kth least significant bit that is set to 1, or -1 if not found.
     */
    uint64_t getKthLeqFromIndex(uint64_t element_index, int k) const
    {
        uint64_t array_index = element_index / Block_Size;
        uint64_t bit_index = element_index % Block_Size;

        // Traverse bits to the left within the current array element
        BlockType mask = ((BlockType(1) << (bit_index)) << 1) - 1; // Mask for bits to the left of bit_index
        BlockType current_value = data_.at(array_index) & mask;
        uint64_t ones_in_current_value = count_ones(current_value);

        // If there are not enough ones in the current block, skip the entire block
        while(ones_in_current_value < k && array_index > 0)
        {
            k -= ones_in_current_value;
            --array_index;
            ones_in_current_value = count_ones(data_.at(array_index));
            bit_index = Block_Size - 1;
        }

        // If there are enough ones in the current block, search bit by bit
        if(k > 0)
        {
            for(uint64_t i = bit_index; i >= 0; --i)
            {
                if((data_.at(array_index)) & (1 << i))
                {
                    --k;
                    if(k == 0)
                    {
                        return array_index * Block_Size + i;
                    }
                }
            }
        }

        return -1; // Return -1 if not found
    }

    /**
     * @brief Returns the index of the kth most significant bit that is set to 1 in the bitmap from the specified element index.
     * It will traverse the bits by blocks so it's faster than the naive implementation, which will traverse bit by bit.
     *
     * @param element_index The index of the element to start the search from.
     * @param k The kth most significant bit to find.
     *
     * @return The index of the kth most significant bit that is set to 1, or -1 if not found.
     */
    uint64_t getKthGeqFromIndex(int element_index, int k) const
    {
        uint64_t array_index = element_index / Block_Size;
        uint64_t bit_index = element_index % Block_Size;

        // Traverse bits to the right within the current array element
        BlockType mask = ~((BlockType(1) << (bit_index)) - 1); // Mask for bits to the right of bit_index
        BlockType current_value = data_.at(array_index) & mask;
        uint64_t ones_in_current_value = count_ones(current_value);

        // If there are not enough ones in the current block, skip the entire block
        while(ones_in_current_value < k && array_index < data_.size() - 1)
        {
            k -= ones_in_current_value;
            ++array_index;
            current_value = data_.at(array_index);
            ones_in_current_value = count_ones(current_value);
            bit_index = 0;  // Reset to the beginning of the next block
        }

        // If there are enough ones in the current block, search bit by bit
        if(k > 0)
        {
            for(uint64_t i = bit_index; i < Block_Size; ++i)
            {
                if((data_.at(array_index) & (BlockType(1) << i)))  
                {
                    --k;
                    if(k == 0)
                    {
                        return array_index * Block_Size + i;
                    }
                }
            }
        }

        return -1; // Return -1 if not found
    }

    
    /**
     * @brief Finds the index of the first valid bit (set to 1) which is less than the specified element index in the bitmap.
     *
     * @param element_index The index of the element to start the search from.
     *
     * @return The index of the first valid bit less than the element index, or -1 if not found.
     */
    uint64_t getFirstValidLessFromIndex(uint64_t element_index) const
    {
        uint64_t array_index = element_index / Block_Size;
        uint64_t bit_index = element_index % Block_Size;

        // Traverse bits to the left within the current block
        BlockType mask = ((BlockType(1) << bit_index) - 1); // Mask for bits to the left of bit_index
        BlockType current_value = data_[array_index] & mask;
        uint64_t ones_in_current_value = count_ones(current_value);

        // Check if there are valid bits in the current block
        if (ones_in_current_value > 0)
        {
            // Find the first valid bit within the current block
            for (uint64_t i = bit_index - 1; i >= 0; --i)
            {
                if (data_[array_index] & (BlockType(1) << i))
                {
                    return array_index * Block_Size + i;
                }
            }
        }

        // Traverse previous blocks
        for (uint64_t i = array_index - 1; i >= 0; --i)
        {
            if (data_[i] != 0)
            {
                uint64_t ones_in_block = count_ones(data_[i]);
                if (ones_in_block > 0)
                {
                    for (uint64_t j = Block_Size - 1; j >= 0; --j)
                    {
                        if (data_[i] & (BlockType(1) << j))
                        {
                            return i * Block_Size + j;
                        }
                    }
                }
            }
        }

        return -1; // Return -1 if not found
    }


    /**
     * Finds the index of the first valid bit (set to 1) which is greater than the specified element index in the bitmap.
     *
     * @param element_index The index of the element to start the search from.
     *
     * @return The index of the first valid bit greater than the element index, or -1 if not found.
     */
    uint64_t getFirstValidGreaterFromIndex(uint64_t element_index) const
    {
        uint64_t array_index = element_index / Block_Size;
        uint64_t bit_index = element_index % Block_Size;

        // Traverse bits to the right within the current block
        BlockType mask = ~((BlockType(1) << bit_index) - 1); // Mask for bits to the right of bit_index
        BlockType current_value = data_[array_index] & mask;
        uint64_t ones_in_current_value = count_ones(current_value);

        // Check if there are valid bits in the current block
        if (ones_in_current_value > 0)
        {
            // Find the first valid bit within the current block
            for (uint64_t i = bit_index + 1; i < Block_Size; ++i)
            {
                if (data_[array_index] & (BlockType(1) << i))
                {
                    return array_index * Block_Size + i;
                }
            }
        }

        // Traverse subsequent blocks
        for (uint64_t i = array_index + 1; i < data_.size(); ++i)
        {
            if (data_[i] != 0)
            {
                uint64_t ones_in_block = count_ones(data_[i]);
                if (ones_in_block > 0)
                {
                    for (uint64_t j = 0; j < Block_Size; ++j)
                    {
                        if (data_[i] & (BlockType(1) << j))
                        {
                            return i * Block_Size + j;
                        }
                    }
                }
            }
        }

        return -1; // Return -1 if not found
    }

private:
    std::vector<BlockType> data_; // Store the bitmap as a vector of BlockTypes
    uint64_t element_capacity_ = 0;

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
    static uint64_t count_ones(BlockType num)
    {
        static_assert(std::is_same_v<BlockType, decltype(num)>, "Implicit type conversion detected");

        if constexpr (std::is_same_v<BlockType, char> || std::is_same_v<BlockType, unsigned char>)
        {
            return __builtin_popcount(static_cast<uint64_t>(num));
        }
        else if constexpr (std::is_same_v<BlockType, short> || std::is_same_v<BlockType, unsigned short>)
        {
            return __builtin_popcount(static_cast<uint64_t>(num));
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

    // Find the kth unset bit to the left starting from `element_index`
    uint64_t getKthLeqFromIndex(uint64_t element_index, int32_t k) const
    {
        // Traverse bits to the left
        for(int32_t i = element_index; i >= 0; --i)
        {
            if(data_[i] == 1)
            {
                --k;
                if(k == 0)
                {
                    return i;
                }
            }
        }
        return -1; // Return -1 if not found
    }

    // Find the kth unset bit to the right starting from `element_index`
    uint64_t getKthGeqFromIndex(uint64_t element_index, int k) const
    {
        // Traverse bits to the right
        for(uint64_t i = element_index; i < data_.size(); ++i)
        {
            if(data_[i] == 1)
            {
                --k;
                if(k == 0)
                {
                    return i;
                }
            }
        }
        return -1; // Return -1 if not found
    }

    // Find the first valid bit (1) less than element_index
    uint64_t getFirstValidLessFromIndex(uint64_t element_index) const
    {
        for (int32_t i = element_index - 1; i >= 0; --i)
        {
            if (data_[i] == 1)
            {
                return i;
            }
        }
        return -1; // Return -1 if not found
    }

    // Find the first valid bit (1) greater than element_index
    uint64_t getFirstValidGreaterFromIndex(uint64_t element_index) const
    {
        for (uint64_t i = element_index + 1; i < data_.size(); ++i)
        {
            if (data_[i] == 1)
            {
                return i;
            }
        }
        return -1; // Return -1 if not found
    }

private:
    std::vector<int8_t> data_; // Use one char per bit to store the bitmap
};