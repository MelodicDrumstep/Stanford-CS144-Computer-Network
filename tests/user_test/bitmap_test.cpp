#include <iostream>
#include <cassert>
#include "../../util/bitmap.hpp"

void testGetPopCountAndUnsetBatch() {
    // Test Case 1: Range within a single block
    {
        Bitmap<32> bitmap(64);  // Create a 64-bit Bitmap
        bitmap.set(10);         // Set bit 10 to 1
        bitmap.set(15);         // Set bit 15 to 1
        bitmap.set(20);         // Set bit 20 to 1

        // Count and clear 1's in the range [10, 20]
        uint64_t pop_count = bitmap.getPopCountAndUnsetBatch(10, 20);

        // Verify results
        assert(pop_count == 3);  // There should be 3 set bits in the range
        assert(bitmap.get(10) == 0);  // Bit 10 should be cleared
        assert(bitmap.get(15) == 0);  // Bit 15 should be cleared
        assert(bitmap.get(20) == 0);  // Bit 20 should be cleared
    }

    // Test Case 2: Range spanning multiple blocks
    {
        Bitmap<32> bitmap(128);  // Create a 128-bit Bitmap
        bitmap.set(30);          // Set bit 30 to 1
        bitmap.set(35);          // Set bit 35 to 1
        bitmap.set(40);          // Set bit 40 to 1
        bitmap.set(60);          // Set bit 60 to 1

        // Count and clear 1's in the range [30, 60]
        uint64_t pop_count = bitmap.getPopCountAndUnsetBatch(30, 60);

        // Verify results
        assert(pop_count == 4);  // There should be 4 set bits in the range
        assert(bitmap.get(30) == 0);  // Bit 30 should be cleared
        assert(bitmap.get(35) == 0);  // Bit 35 should be cleared
        assert(bitmap.get(40) == 0);  // Bit 40 should be cleared
        assert(bitmap.get(60) == 0);  // Bit 60 should be cleared
    }

    // Test Case 3: Range exceeding the boundary
    {
        Bitmap<32> bitmap(64);  // Create a 64-bit Bitmap
        bitmap.set(50);         // Set bit 50 to 1
        bitmap.set(60);         // Set bit 60 to 1

        // Count and clear 1's in the range [50, 70] (70 is out of bounds)
        uint64_t pop_count = bitmap.getPopCountAndUnsetBatch(50, 70);

        // Verify results
        assert(pop_count == 2);  // There should be 2 set bits in the range
        assert(bitmap.get(50) == 0);  // Bit 50 should be cleared
        assert(bitmap.get(60) == 0);  // Bit 60 should be cleared
    }

    // Test Case 4: No set bits in the range
    {
        Bitmap<32> bitmap(64);  // Create a 64-bit Bitmap

        // Count and clear 1's in the range [10, 20]
        uint64_t pop_count = bitmap.getPopCountAndUnsetBatch(10, 20);

        // Verify results
        assert(pop_count == 0);  // There should be no set bits in the range
    }

    std::cout << "All test cases passed!" << std::endl;
}

int main() {
    testGetPopCountAndUnsetBatch();
    return 0;
}