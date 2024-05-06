// lib entry point
#define _HAS_CXX20 1
#include "lib_large_numbers.hpp"

// Fibonacci_rec(12) = 144
// Fibonacci_iter(12) = 144
// Expected: 144
// Result: 144
// Unit test duration: 0.0040736 s
// Final Index: 4782 - 1000 digits
// Duration: 426.793 s
const std::uint16_t MAX_DIGITS = 1000; // basically, 10 ^ 1000
// Brute force: Iterate through fibonacci numbers until we find the very first
// index that are 1000 digits long
int main() {
  hairev::libs::Large_Numbers::unit_test();

  auto start = std::chrono::high_resolution_clock::now();

  // start with 3 digits - fibonacci(12) = 144 (first index that are 3 digits)
  auto my_large_number = hairev::libs::Large_Numbers::New();
  auto index = 11; // it does pre-increment, so start at 11
  // while (fibonacci_large_rec(i64_to_digits(index)).size() < MAX_DIGITS) {
  auto digit_count = my_large_number.Fibonacci(index).Get().size();
  do {
    index++;
    auto result = hairev::libs::Large_Numbers::Fibonacci(index);
    digit_count = result.Get().size();
  } while (digit_count < MAX_DIGITS);

  std::cout << "Final Index: " << index << " - " << digit_count << " digits"
            << endl;

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;
  std::cout << "Duration: " << diff.count() << " s" << std::endl;

  return 0;
}
