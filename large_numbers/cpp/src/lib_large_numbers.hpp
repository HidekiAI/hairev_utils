#ifndef LARGE_NUMBERS
#define LARGE_NUMBERS

// PLEASE NOTE: If you found this file somehow while looking for large-number
// handling for C++, there are better libraries (proven and tested) that you
// should use instead in particular "BigInteger" from Boost or "GMP" (GNU
// Multiple Precision Arithmetic Library) I have written this library for
// educational purposes to be used for Project Euler and other online puzzles
// and competitions in which I can grab parts of this code and paste it into the
// online editor...

// My essentials that I always include:
#if _HAS_CXX20 // NOTE: Though I just need C++17 so I can use std::optional and
               // lamdas, I believe Hackerrank allows UP TO uses C++20
#include <algorithm> // std::sort, std::transform, std::find (std::find - make sure to override operator==)
#include <array>
#include <cassert>    // assert()
#include <chrono>     // for start/end time measurement
#include <cstdint>    // std::uint16_t, etc - I'm too used to rust types...
#include <fstream>    // for reading in file
#include <functional> // lambdas!
#include <iostream>   // std::cout
#include <memory>     // std::shared_ptr, std::make_shared
#include <optional>   // a bit different from Rust Option<T> but still, useful!
#include <stack> // commonly used when I need to convert recursive to iterative
#include <string>
#include <tuple>
#include <unordered_map> // use map if need keys to be ordered, but generally, I just need key to be hashed...
#include <unordered_set>
#include <utility> // std::pair, etc
#include <vector>
#else
// fail compiler if C++ version is less than C++20
// but without using static_assert() because it's not available until C++17
#error This code requires at least C++17
#endif // !_HAS_CXX20 || !_HAS_CXX17

using namespace std;

namespace hairev {
namespace libs {
class Large_Numbers {

private:
  // NOTE: I'm using vector<uint32_t> instead of string and/or vector<uint8_t>
  // because uint8_t gives really unpredictable output routed to std::cout
  // because cout << thinks it's a char, and I don't want to bother wasting
  // time doing std::cout << static_cast<int>(some_uint8_t_value) every time
  // A careful caution must be made, on uint because there are some functions
  // which I'm updating in-place (i.e. lhs[i] -= rhs[i]) and if lhs[i] < rhs[i],
  // lhs[i] will turn this negative into uint underflow (i.e. 0 - 1 =
  // 4294967295); with that mentioned about updating in-place, because each
  // digits are in range of +0..+9, it's definitely safe to use uint16_t (not
  // uint8_t since it seems to cause issues and thinks it's char), but I'm using
  // uint32_t because of my suspicion that if it does unexplainable things with
  // char confusion on 8-bits, it'll probably do the same on wchar (16-bits),
  // and in today's CPU, using native 32-bits is much more faster/optimized than
  // downcasting to 8 or 16 bits (i.e. wasting 32-bit register to pack upper
  // 24-bits with 0's)
  typedef std::vector<std::uint32_t>
      int_revvec_t; // todo: write iterator for this defined type
  typedef bool sign_t;
  // NOTE: Internal representation is in reverse order (i.e. 1234 is stored as
  // {4, 3, 2, 1}) because it's easier to add and subtract that way
  // and only when rendering (i.e. dump, to_string) will it be reversed
  // hence these are private and should not be accessed directly
  int_revvec_t value;
  sign_t is_positive;

  const int_revvec_t vec_zero = int_revvec_t{0}; // it has no sign
  const int_revvec_t vec_one = int_revvec_t{1};  // it has no sign

public:
  Large_Numbers(const uint64_t &v) : is_positive(true) {
    auto tup = i64_to_digits(v);
    value = std::get<1>(tup);
    is_positive = std::get<0>(tup);
  }
  Large_Numbers(const int64_t &v) : is_positive(v >= 0) {
    auto tup = i64_to_digits(v);
    value = std::get<1>(tup);
    is_positive = std::get<0>(tup);
  }
  Large_Numbers(const std::string &str_v, bool is_positive)
      : is_positive(is_positive) {

    auto fs = from_string(str_v);
    this->value = fs.value;
    this->is_positive = fs.is_positive;
    if (this->is_positive != fs.is_positive) {
      std::clog << "Sign mismatch: '" << str_v
                << "' was passed as is_positive=" << this->is_positive
                << " but converter from_strin() thinks it is " << fs.is_positive
                << std::endl;
      throw std::invalid_argument("Sign mismatch");
    }
  }
  Large_Numbers(const std::string &v) : Large_Numbers(v, true) {}
  ~Large_Numbers() {}
  static Large_Numbers New() { return Zero(); }
  static Large_Numbers Zero() { return Large_Numbers(int_revvec_t{0}, true); }
  bool Is_Positive() {
    // if it's zero, it's positive even if is_positive is false
    return this->is_positive || this->Is_Zero();
  }
  std::string Get(int width = 0,
                  char padding_char = '0' /*use ' ' or '0'*/) const {
    // NOTE: we do not waste time trimming in case it was padded with zeros
    // intentionally especially when converting to string, ideally we'd like to
    // see it aligned Note that padding_char are ignored if width is 0
    return to_string_with_padding(width, padding_char);
  }
  size_t Size() const { return this->value.size(); }
  void Dump() const { dump_digits(*this); }

  // NOTE: both +0 and -0 should return true
  bool Is_Zero() const { return op_equal(this->value, vec_zero); }

  static Large_Numbers Fibonacci(const std::uint64_t &n) {
    // for performance, we'll just call the iterative version
    auto my_large_number = Large_Numbers(n);
    auto result = my_large_number.fibonacci_large_iter(n);
    return {result, true};
  };

private:
  // we do not allow NaN
  Large_Numbers() : is_positive(true) { value = vec_zero; }
  Large_Numbers(int_revvec_t v, bool is_positive)
      : value(v), is_positive(is_positive) {}
  Large_Numbers(int_revvec_t v) : is_positive(true), value(v) {}

public:
  // operator overloads are public facades to the private functions as well as
  // it's capabilites of what each function operations were designed to do
  Large_Numbers &operator=(const Large_Numbers &rhs) {
    this->is_positive = rhs.is_positive;
    this->value = rhs.value;
    return *this;
  }
  const Large_Numbers operator-(const Large_Numbers &rhs) const {
    auto tup_lhs = std::make_tuple(this->is_positive, this->value);
    auto tup_rhs = std::make_tuple(rhs.is_positive, rhs.value);
    auto result = op_subtract_tuple(tup_lhs, tup_rhs);
    return Large_Numbers(std::get<1>(result), std::get<0>(result));
  }
  Large_Numbers operator-=(Large_Numbers &rhs) { return *this - rhs; }
  const Large_Numbers operator+(const Large_Numbers &rhs) const {
    auto tup_lhs = std::make_tuple(this->is_positive, this->value);
    auto tup_rhs = std::make_tuple(rhs.is_positive, rhs.value);
    auto result = op_add_tuple(tup_lhs, tup_rhs);
    return Large_Numbers(std::get<1>(result), std::get<0>(result));
  }
  Large_Numbers operator+=(Large_Numbers &rhs) { return *this + rhs; }
  const Large_Numbers operator*(const Large_Numbers &rhs) {
    auto tup_lhs = std::make_tuple(this->is_positive, this->value);
    auto tup_rhs = std::make_tuple(rhs.is_positive, rhs.value);
    auto result = op_multiply_tuple(tup_lhs, tup_rhs);
    return Large_Numbers(std::get<1>(result), std::get<0>(result));
  }
  Large_Numbers operator*=(Large_Numbers &rhs) { return *this * rhs; }
  const Large_Numbers operator/(const Large_Numbers &rhs) {
    auto tup_lhs = std::make_tuple(this->is_positive, this->value);
    auto tup_rhs = std::make_tuple(rhs.is_positive, rhs.value);
    auto result = op_divide_tuple(tup_lhs, tup_rhs);
    return Large_Numbers(std::get<1>(result), std::get<0>(result));
  }
  Large_Numbers operator/=(Large_Numbers &rhs) { return *this / rhs; }
  // only reason why we have modulo is because we have division and it's too
  // useful if paired with division
  Large_Numbers operator%(Large_Numbers &rhs) {
    auto tup_lhs = std::make_tuple(this->is_positive, this->value);
    auto tup_rhs = std::make_tuple(rhs.is_positive, rhs.value);
    auto result = op_modulo_tuple(tup_lhs, tup_rhs);
    return Large_Numbers(std::get<1>(result), std::get<0>(result));
  }
  Large_Numbers operator%=(Large_Numbers &rhs) { return *this % rhs; }

  // unary operators
  Large_Numbers operator-() {
    this->is_positive = !this->is_positive;
    return *this;
  }

  // comparison operators
  bool operator==(const Large_Numbers &rhs) const {
    return (this->is_positive == rhs.is_positive) &&
           op_equal(this->value, rhs.value);
  }
  bool operator!=(const Large_Numbers &rhs) const { return !(*this == rhs); }
  bool operator<(const Large_Numbers &rhs) const {
    if (this->is_positive && !rhs.is_positive) {
      return false;
    }
    if (!this->is_positive && rhs.is_positive) {
      return true;
    }
    return op_less_than(this->value, rhs.value);
  }
  bool operator>(const Large_Numbers &rhs) const {
    return !(*this < rhs) && !(*this == rhs);
  }
  bool operator<=(const Large_Numbers &rhs) const {
    return (*this < rhs) || (*this == rhs);
  }
  bool operator>=(const Large_Numbers &rhs) const {
    return (*this > rhs) || (*this == rhs);
  }

  // conversion operators: to const string
  operator std::string() const { return to_string_with_padding(); }
  std::string to_string() const { return to_string_with_padding(); }
  std::string to_string_with_padding(int width = 0,
                                     char padding_char = '0') const {
    // NOTE: we do not waste time trimming in case it was padded with zeros
    // intentionally especially when converting to string, ideally we'd like to
    // see it aligned Note that padding_char are ignored if width is 0

    // we first do the usual/regular conversion to string
    std::string str = "";
    if (!this->is_positive) {
      str += "-";
    }
    for (auto digit : this->value) {
      str += std::to_string(digit);
    }

    // if no padding, just append the digits
    if ((width >= 2) || (this->value.size() < width - 1)) {
      // if here, we need to do the padding; first we want to remove the sign
      // (if any) and then pad the rest, then put the sign back
      if (!this->is_positive) {
        str = str.substr(1);
      }
      // now pad the rest
      while (str.size() < width) {
        str = padding_char + str;
      }
      // finally, put the sign back
      if (!this->is_positive) {
        str = "-" + str;
      }
    }

    return str;
  }

  Large_Numbers from_string(const std::string &str) {
    if (str.size() == 0) {
      throw std::invalid_argument("Empty string");
    }

    auto str_v_copied = str;
    auto ret_large_number = Large_Numbers();

    // NOTE: String indexing is that 0'th index is highest digit (or sign)
    // so we'll first check the highest digit if it's a sign, and if it
    // is, yank it out so we can deal with the rest as digits
    // if it's negative, override the is_positive bool and honor the sign
    if (str_v_copied[0] == '-') {
      ret_large_number.is_positive = false;
      str_v_copied = str_v_copied.substr(1);
    }

    // because number here will be in order of 9876543210, we'll have to reverse
    // it (actually, we'll just reverse the index, so we don't waste time doing
    // in std::reverse) and then push_back to the vector, so that it's in order
    // of 0123456789 will be pushed back in order of 0, 1, 2, 3, 4, 5, 6, 7, 8,
    // 9 which should generate a vector of {9, 8, 7, 6, 5, 4, 3, 2, 1, 0} we'll
    // just convert the string to vector<uint32_t>
    // now trim spaces and commas, and truncate any values trailing "." (i.e.
    // 1234.567 -> 1234 truncated/rounded-down rather than rounded up)
    // locate decimal point (if any)
    std::string::size_type pos = str_v_copied.find(".");
    if (pos != std::string::npos) {
      // just truncate FROM the first encounter of "."
      str_v_copied = str_v_copied.substr(0, pos);
    }
    // trim commas, underscores, and spaces (in rust, I think we need to trim
    // off "'" as well, so we'll do that here)
    str_v_copied.erase(
        std::remove(str_v_copied.begin(), str_v_copied.end(), ','),
        str_v_copied.end());
    str_v_copied.erase(
        std::remove(str_v_copied.begin(), str_v_copied.end(), '_'),
        str_v_copied.end());
    str_v_copied.erase(
        std::remove(str_v_copied.begin(), str_v_copied.end(), '\''),
        str_v_copied.end());
    str_v_copied.erase(
        std::remove(str_v_copied.begin(), str_v_copied.end(), ' '),
        str_v_copied.end());

    // pop off heading zeros (i.e. 000123 -> 321) before we reverse it
    while (str_v_copied[0] == '0') {
      str_v_copied = str_v_copied.substr(1);
    }

    // revert the string (in-place reverse) so that it's in order we deal with
    // internally
    std::reverse(str_v_copied.begin(), str_v_copied.end());

    // finally, convert the string to vector<uint32_t> (any non-numeric will
    // throw) - note: if you do not locate isdigit() in ctype.h, then...
    ret_large_number.value.clear(); // just in case it's not empty
    for (auto c : str_v_copied) {
      if (!isdigit(c)) {
        std::clog << "Invalid character " << c << " in string '" << str_v_copied
                  << "'." << std::endl;
        throw std::invalid_argument("Invalid character in string");
      }
      ret_large_number.value.push_back(c - '0');
    }

    return ret_large_number;
  }

  // all functions which does not deal with "this" object are private
  // but at the same time, they should be well modularized so that one can
  // just copy-and-paste each needed functions to other C++ projects (mainly
  // online puzzles and competitions)
private:
  // note: Only dumps digits, no sign
  static void dump_digits(const int_revvec_t &digits) {
    // internal representation order of a number "123" is stored as {3, 2, 1}
    // hence, iteration should be in reverse order
    auto digits_cloned = digits;
    std::reverse(digits_cloned.begin(), digits_cloned.end());
    for (auto digit : digits_cloned) {
      std::clog << digit;
    }
  }
  // dumps digits with sign
  static void dump_digits(const std::tuple<bool, int_revvec_t> &tup_digits) {
    std::clog << (get<0>(tup_digits) ? " " : "-");
    dump_digits(get<1>(tup_digits));
  }
  static void dump_digits(const Large_Numbers &ln) {
    std::clog << (ln.is_positive ? " " : "-");
    dump_digits(ln.value);
  }

  std::tuple<bool, int_revvec_t> i64_to_digits(const std::int64_t &num64) {
    auto digits = int_revvec_t();
    // push_back appends to the end of the vector, hence if the sequence of the
    // i64 is "1234" in which we'd mod% it by 10, we'd get 4, then we would
    // push_back 4, then we'd divide by 10, and get 123, and so on and push_back
    // to the vector the final order on the vector will be {4, 3, 2, 1} which
    // conviniently is the reverse order we'd expect!
    auto is_positive = num64 >= 0;
    auto current_digit = is_positive ? num64 : -num64;
    while (current_digit > 0) {
      digits.push_back(current_digit % 10);
      current_digit /= 10;
    }

    return std::tuple(is_positive, digits);
  }

  // if the digits is "0000123" (maybe for alignment purposes), we'll trim the
  // front zeros so that it's "123".  Because internally, digits are stored
  // in reverse order ("000123" is stored as {3, 2, 1, 0, 0, 0}), we'll just
  // pop off the back zeroes
  int_revvec_t trim(const int_revvec_t &digits) {
    // while the first digit is 0, pop it off (i.e. 000123 -> 123)
    auto digits_cloned = digits;
    while (digits_cloned.back() == 0) {
      digits_cloned.pop_back();
    }
    // now reverse it back to original order
    return digits_cloned;
  }
  void trim() { value = trim(value); }

  // We need comparitors for the vector<uint32_t>
  bool op_equal(const int_revvec_t &lhs, const int_revvec_t &rhs) const {
    if (lhs != rhs) {
      // usually, the built-in version is optimized and faster than manual
      return false;
    }
    if (lhs.size() != rhs.size()) {
      return false;
    }
    for (int i = 0; i < lhs.size(); i++) {
      if (lhs[i] != rhs[i]) {
        return false;
      }
    }
    return true;
  }
  bool op_equal_tup(
      const std::tuple<bool /*sign*/, int_revvec_t /*digits*/> &lhs,
      const std::tuple<bool /*sign*/, int_revvec_t /*digits*/> &rhs) const {
    if (std::get<1>(lhs).size() != std::get<1>(rhs).size()) {
      return false;
    }
    return op_equal(std::get<1>(lhs), std::get<1>(rhs));
  }

  bool op_less_than(const int_revvec_t lhs, const int_revvec_t rhs) const {
    if (lhs < rhs) {
      // usually, the built-in version is optimized and faster than manual
      return true;
    }
    if (lhs.size() != rhs.size()) {
      return lhs.size() < rhs.size();
    }
    for (int i = 0; i < lhs.size(); i++) {
      if (lhs[i] != rhs[i]) {
        return lhs[i] < rhs[i];
      }
    }
    return false;
  }
  bool op_less_than_tup(
      const std::tuple<bool /*sign*/, int_revvec_t /*digits*/> &lhs,
      const std::tuple<bool /*sign*/, int_revvec_t /*digits*/> &rhs) const {
    return op_less_than(std::get<1>(lhs), std::get<1>(rhs));
  }

  // subtaction based assumptions that left is >= right AND both
  // left AND right are POSITIVE numbers
  // See op_subtract_tuple for signed subtraction
  int_revvec_t op_subtract(const int_revvec_t &lhs,
                           const int_revvec_t &rhs) const {
    // std::cout << "op_subtract:" << std::endl;
    // std::cout << "\tleft:  ";
    // dump_digits(lhs);
    // std::cout << std::endl;
    // std::cout << "\tright: ";
    // dump_digits(rhs);
    // std::cout << std::endl;

    // for subtraction, the most obvious is to check if left == right
    if (op_equal(lhs, rhs)) {
      //std::cout << "\top_subtract result (zero): ";
      //dump_digits(vec_zero);
      //std::cout << std::endl;

      return vec_zero;
    }
    // another obvious is if either is 0 (but because we assume lhs > rhs, we
    // only need to check rhs)
    if ((rhs.size() == 0) || (rhs.size() == 1 && rhs[0] == 0)) {
      //std::cout << "\top_subtract result (rhs==0, returning lhs-as-is): ";
      //dump_digits(lhs);
      //std::cout << std::endl;

      return lhs;
    }

    // first, clone so that we can reverse it
    auto left_clone = lhs;
    auto right_clone = rhs;
    // we won't eve throw, we will ASSUME left > right (we already checked if
    // equal)
    // 87654
    // - 780
    // =====
    // ((4 - 0) +  0) - 0 = 4 (no borrow)
    // ((5 - 0) + 10) - 8 = 7 (borrow from next digit)
    // ((6 - 1) + 10) - 7 = 8 (borrow from next digit)
    // ((7 - 1) +  0) - 0 = 6 (no borrow)
    // ((8 - 0) +  0) - 0 = 8 (no borrow)
    // 87654 - 780 = 86874
    // we'll update left-clone in-place
    int32_t borrow = 0; // 0 if previous did not borrow, 1 if it was borrowed
    for (auto left_index = 0; left_index < left_clone.size(); left_index++) {
      // if rhs.size() < left_index, just use 0 since (lhs - 0 = lhs)
      int32_t right_digit = (left_index < right_clone.size())
                                ? (int32_t)right_clone[left_index]
                                : 0;
      int32_t left_digit = (int32_t)left_clone[left_index] - borrow;
      if ((left_digit < 0) || (left_digit < right_digit)) {
        // we'll need to borrow again
        left_digit += 10; // borrow
        borrow = 1;       // we borrowed
      } else {
        borrow = 0; // we did not need to borrow
      }
      left_clone[left_index] = left_digit - right_digit;
    }
    // finally, if borrow is still 1, then something went wrong, because we have
    // already checked that lhs > rhs
    if (borrow == 1) {
      std::clog << "Borrow is still 1 after subtraction" << std::endl;
      throw std::invalid_argument("Borrow is still 1 after subtraction");
    } else {
      // trim: remove the last digit if it's 0 (43210 , strip it to 4321)
      while (left_clone.back() == 0) {
        left_clone.pop_back();
      }
    }

    return left_clone;
  }

  // Note: unlike the other excercise, we'll add IN-PLACE so we do
  // not need to mess with any carry-over values
  int_revvec_t op_add(const int_revvec_t &lhs, const int_revvec_t &rhs) const {
    // std::cout << "op_add:" << std::endl;
    // std::cout << "\tleft:  ";
    // dump_digits(lhs);
    // std::cout << std::endl;
    // std::cout << "\tright: ";
    // dump_digits(rhs);
    // std::cout << std::endl;

    // first, clone so that we can reverse it
    auto left_clone = lhs.size() > rhs.size() ? lhs : rhs;
    auto right_clone = lhs.size() > rhs.size() ? rhs : lhs;

    // as tempting it is to check if rhs is just single digit, it can become a
    // carry-over chained reaction if lhs is 999...9999 and rhs > 0...  lhs==0
    // is the only special case we'll handle if either is val[0] == 0 (or
    // empty), just return the other since we know that lhs is always wider than
    // rhs, we can just check rhs for width
    if ((right_clone.size() == 0) ||
        (right_clone.size() == 1 && right_clone[0] == 0)) {
      //std::cout << "\top_add result (rhs==0, returning lhs-as-is): ";
      //dump_digits(left_clone);
      //std::cout << std::endl;

      return left_clone;
    }

    // 87654
    // 91239
    // =====
    // (4 + 9) = 13 = 3 (carry-over 1)
    // (5 + 3) + 1 = 9 (carry-over 0)
    // (6 + 2) = 8 (carry-over 0)
    // (7 + 1) = 8 (carry-over 0)
    // 8 + 9 = 17 = 7 (carry-over 1)
    // 91239 (5 digits) + 87654 (5 digits) = 178893 (6 digits)
    for (auto left_index = 0; left_index < left_clone.size(); left_index++) {
      // add and carry-over if needed; Note that because we use u8 type instead
      // of char '0'..'9' we can actually add in place (i.e. 9 + 4 = 13) and
      // then just subtract 10 if >= 10
      if (left_index < right_clone.size()) {
        // make sure right[i] exists...
        left_clone[left_index] += right_clone[left_index];
      }

      // TODO: some reason, when I do 'if (left_clone[left_index] >= 10)' it
      //       doesn't work and I have to preset my carry over - figure out why
      auto carry_over = left_clone[left_index] / 10; // 23 / 10 = 2
      if (carry_over > 0) {
        left_clone[left_index] = left_clone[left_index] % 10; // 23 % 10 = 3
        // if it's the last digit, we have to add a new digit, else +1 to next
        // digit.  We shouldn't have to do the push_back since we resized it
        // (left_clone.size()) earlier, but just in case, we'll leave this extra
        // check so that when I port this to rust, it'l be a reminder...
        if (left_index + 1 < left_clone.size()) {
          left_clone[left_index + 1] += carry_over;
        } else {
          try {
            // if we're at the end, we'll just push_back the carry_over
            left_clone.push_back(carry_over);
          } catch (const std::exception &e) {
            std::clog << "Exception: " << e.what() << std::endl;
            std::clog << "left_index: " << left_index
                      << ", left_clone.size(): " << left_clone.size()
                      << ", carry_over: " << carry_over << std::endl;
          }
        }
      }
    }

    // trim: remove the last digit if it's 0 (43210 , strip it to 4321)
    while (left_clone.back() == 0) {
      left_clone.pop_back();
    }

    return left_clone;
  }

  std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> op_add_tuple(
      std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> left,
      std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> right) const {
    //std::cout << "op_add_tuple:" << std::endl;
    //std::cout << "\tleft (is_positive=" << std::get<0>(left) << ")  :";
    //dump_digits(left);
    //std::cout << std::endl;
    //std::cout << "\tright (is_positive=" << std::get<0>(right) << ") :";
    //dump_digits(right);
    //std::cout << std::endl;

    // if lhs > 0, rhs > 0, just add both
    if (std::get<0>(left) && std::get<0>(right)) {
      //std::cout << "\tboth positive..." << std::endl;
      // NOTE: op_add() pretests for lesser-of-the-two == 0
      auto result = op_add(std::get<1>(left), std::get<1>(right));
      // both are positive, so result is positive
      return {true, result};
    }

    // if lhs < 0, rhs > 0, swap and subtract
    else if (!std::get<0>(left) && std::get<0>(right)) {
      //std::cout << "\tleft negative, right positive..." << std::endl;
      auto result = op_subtract(std::get<1>(right), std::get<1>(left));
      // if rhs >= lhs, then result is positive, else negative
      return {op_less_than(std::get<1>(right), std::get<1>(left)), result};
    }

    // if lhs > 0, rhs < 0, just subtract
    else if (std::get<0>(left) && !std::get<0>(right)) {
      //std::cout << "\tleft positive, right negative..." << std::endl;
      auto result = op_subtract(std::get<1>(left), std::get<1>(right));
      // if lhs >= rhs, then result is positive, else negative
      return {op_less_than(std::get<1>(left), std::get<1>(right)), result};
    }

    // if lhs < 0, rhs < 0, make rhs positive, swap, and subtract
    else if (!std::get<0>(left) && !std::get<0>(right)) {
      //std::cout << "\tboth negative..." << std::endl;
      auto result = op_subtract(std::get<1>(right), std::get<1>(left));
      // if rhs >= lhs, then result is positive, else negative
      return {op_less_than(std::get<1>(right), std::get<1>(left)), result};
    }

    // else some unknown state?
    std::clog << "Unknown state: " << std::get<0>(left) << " and "
              << std::get<0>(right) << std::endl;
    throw std::invalid_argument("Unknown state");
  }

  // operator subtract (to calculate n-1 and n-2)
  std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> op_subtract_tuple(
      std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> left,
      std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> right) const {
    //std::cout << "op_subtract_tuple:" << std::endl;
    //std::cout << "\tleft (is_positive=" << std::get<0>(left) << ")  :";
    //dump_digits(left);
    //std::cout << std::endl;
    //std::cout << "\tright (is_positive=" << std::get<0>(right) << ") :";
    //dump_digits(right);
    //std::cout << std::endl;

    // first, clone so that we can reverse it
    auto left_clone = std::get<1>(left);
    auto right_clone = std::get<1>(right);
    auto left_is_positive = std::get<0>(left);
    auto right_is_positive = std::get<0>(right);
    auto result_is_positive = op_less_than_tup(left, right) ? false : true;

    // now we can subtract (paying attention to assumptions that
    // width/size/length of n.size() != m.size()) but also realizing that n - m
    // != m - n
    auto max_width = std::max(left_clone.size(), right_clone.size());
    auto result = int_revvec_t();
    // n - m = -m + n
    // 123 - 87654 = -87654 + 123 = -87531
    // 87654
    // - 123
    // =====
    //  4 - 3 = 1
    //  5 - 2 = 3
    //  6 - 1 = 5
    //  7 - 0 = 7
    //  8 - 0 = 8
    // sign it => -87531
    // very trivial elementary school math, but doing this in code makes it seem
    // complex :P
    // * (+l) - (+r) = subtract (l-r) or (r-l) and if r > l, negate;
    //   i.e. 10 - 5 = 5; 5 - 10 = -5
    // * (+l) - (-r) = (+l) + (+r) = just addition;
    //   i.e. 10 - -5 = 10 + 5 = 15; 5 - -10 = 5 + 10 = 15
    // * (-l) - (+r) = just add and negate;
    //   i.e. -10 - 5 = -10 + -5 = -15; -5 - 10 = -5 + -10 = -15
    // * (-l) - (-r) = (-l) + (+r) = subtract (r-l) or (l-r) and if r > l,
    // negate
    //  i.e. -10 - -5 = -10 + 5 = -5; -5 - -10 = -5 + 10 = 5

    // * (+l) - (+r) = subtract (l-r) or (r-l) and if r > l, negate;
    //   i.e. 10 - 5 = 5; 5 - 10 = -5
    if (left_is_positive && right_is_positive) {
      result_is_positive = true;
      // if both are postive, first check if left < right, if so, swap and set
      // (and later negate)
      if (op_less_than(left_clone, right_clone)) {
        // std::swap(left_clone, right_clone); // alternatively, just r - l
        result_is_positive = false;
        result = op_subtract(right_clone, left_clone); // swap left and right
      } else {
        result = op_subtract(left_clone, right_clone);
      }
    }
    // * (+l) - (-r) = (+l) + (+r) = just addition;
    //   i.e. 10 - -5 = 10 + 5 = 15; 5 - -10 = 5 + 10 = 15
    else if (left_is_positive != right_is_positive) {
      // if right is negative, then it's just addition (n - -m = n + m)
      // if left is negative, swap, then add, and negate
      result_is_positive = left_is_positive;    // negate if left is negative
      result = op_add(left_clone, right_clone); // either way, just add...
    }
    // * (-l) - (+r) = just add and negate;
    //   i.e. -10 - 5 = -10 + -5 = -15; -5 - 10 = -5 + -10 = -15
    else if (!left_is_positive && right_is_positive) {
      result_is_positive = false;
      // if left is negative, then it's just addition (n - -m = n + m)
      // if right is negative, swap, then add, and negate
      result = op_add(left_clone, right_clone); // either way, just add...
    }
    // * (-l) - (-r) = (-l) + (+r) = subtract (r-l) or (l-r) and if r > l,
    // negate
    //  i.e. -10 - -5 = -10 + 5 = -5; -5 - -10 = -5 + 10 = 5
    else if (!left_is_positive && !right_is_positive) {
      result_is_positive = true;
      // if both are negative, make right side positive and then
      // test to see if m > n, and if so, just subtract (-n - -m = m - n)
      // else, swap again, and then subtract and then negate (n - m)

      // first, make right side positive (i.e. -left + right)
      if (op_less_than(left_clone, right_clone)) {
        // now that right side is positive, we can subtract right - left
        // (basically, swapped) IF right > left
        result = op_subtract(right_clone, left_clone);
      } else {
        // because right < left, doing (right - left) will need swapped
        // to (left - right) and negate
        result = op_subtract(left_clone, right_clone); // left > right
        result_is_positive = false;
      }
    }

    // finally, reverse the result
    std::tuple<bool, int_revvec_t> result_tuple = {result_is_positive, result};
    return result_tuple;
  }

  std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> op_multiply_tuple(
      std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> left,
      std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> right) const {
    std::cout << "op_multiply_tuple:" << std::endl;
    std::cout << "\tleft (is_positive=" << std::get<0>(left) << ")  :";
    dump_digits(left);
    std::cout << std::endl;
    std::cout << "\tright (is_positive=" << std::get<0>(right) << ") :";
    dump_digits(right);
    std::cout << std::endl;

    // not implemented yet
    throw std::invalid_argument("Not implemented yet");
  }
  std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> op_divide_tuple(
      std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> left,
      std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> right) const {
    std::cout << "op_divide_tuple:" << std::endl;
    std::cout << "\tleft (is_positive=" << std::get<0>(left) << ")  :";
    dump_digits(left);
    std::cout << std::endl;
    std::cout << "\tright (is_positive=" << std::get<0>(right) << ") :";
    dump_digits(right);
    std::cout << std::endl;

    // not implemented yet
    throw std::invalid_argument("Not implemented yet");
  }
  std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> op_modulo_tuple(
      std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> left,
      std::tuple<bool /*is_positive*/, int_revvec_t /*digits*/> right) const {
    std::cout << "op_modulo_tuple:" << std::endl;
    std::cout << "\tleft (is_positive=" << std::get<0>(left) << ")  :";
    dump_digits(left);
    std::cout << std::endl;
    std::cout << "\tright (is_positive=" << std::get<0>(right) << ") :";
    dump_digits(right);
    std::cout << std::endl;

    // not implemented yet
    throw std::invalid_argument("Not implemented yet");
  }

  // ideally, I'd probably want to have functions jut for finding fibonacci
  // that are within the range to fit in uint64_t, but it'll just cause
  // code-pollutions, so we'll just handle all cases in one function...
  // constraints: for fibonacci, n must be > 0
  int_revvec_t fibonacci_large_rec(int_revvec_t n) {
    if (n.size() == 0) {
      throw std::invalid_argument("n must be > 0");
    }
    auto result = int_revvec_t();
    if ((n.size() == 1) && (n[0] == 0)) {
      result.push_back(0);
      return result;
    }
    if ((n.size() == 1) && (n[0] == 1)) {
      result.push_back(1);
      return result;
    }
    auto n_minus_1 = op_subtract(n, {1});
    auto n_minus_2 = op_subtract(n, {2});
    auto fib_n_minus_1 = fibonacci_large_rec(n_minus_1);
    auto fib_n_minus_2 = fibonacci_large_rec(n_minus_2);
    auto added = op_add(fib_n_minus_1, fib_n_minus_2);
    return added;
  }

  std::int64_t to_i64() {
    // first, let's make sure digits are trimmed, and that it fits in i64 max
    // value
    auto v = this->value;
    auto trimmed = trim(v);
    if (trimmed.size() > 19) {
      throw std::invalid_argument("Number too large to fit in i64");
    }

    std::int64_t result = 0;

    return result;
  }

  int_revvec_t fibonacci_large_iter() {
    auto n = this->to_i64();
    return fibonacci_large_iter(n);
  }
  // iterative version of fibonacci because trying to achieve 1000 digits
  // in recursive version is not only slow, but also stack-overflow prone
  int_revvec_t fibonacci_large_iter(uint64_t n) {
    if (n == 0) {
      return {0};
    }
    if (n == 1) {
      return {1};
    }
    auto n_minus_1 = int_revvec_t{1};
    auto n_minus_2 = int_revvec_t{0};
    auto fib_n = int_revvec_t{};
    // begin iteration from 2..=n
    for (auto i = 2; i <= n; i++) {
      // fib(n) = fib(n-1) + fib(n-2); 34 = 21 + 13
      // fib(n-1) = fib(n-2) + fib(n-3) ; 21 = 13 + 8
      fib_n = op_add(n_minus_1, n_minus_2);
      n_minus_2 = n_minus_1;
      n_minus_1 = fib_n;
    }

    this->value = fib_n;
    return fib_n;
  }

public:
  static void unit_test() {
    auto start = std::chrono::high_resolution_clock::now();
    // if lambda is recursive, it must be defined as a std::function separately
    // again, because of these "feels like a hack added to C++", I prefer
    // rust...
    auto test_from_string = Large_Numbers("1234567890");
    auto test_from_string_negative = Large_Numbers("-1234567890");
    auto test_from_vec =
        Large_Numbers(int_revvec_t{1, 2, 3, 4, 5, 6, 7, 8, 9, 0});
    auto test_copy_ctor = Large_Numbers(test_from_vec);
    auto test_trim = Large_Numbers("00001234567890");

    // test operator overloads
    const auto lhs = Large_Numbers("9876543210");
    const auto rhs = Large_Numbers(-9876543210);
    const auto zero = Large_Numbers::Zero();
    std::clog << "\nlhs: ";
    dump_digits(lhs);
    std::clog << "\nrhs: ";
    dump_digits(rhs);
    std::clog << std::endl;

    std::clog << "Testing addition:" << std::endl;
    auto result_add = lhs + rhs; // should subtract and result to 0
    dump_digits(lhs);
    std::clog << " + ";
    dump_digits(rhs);
    std::clog << " = ";
    dump_digits(result_add);
    std::clog << std::endl;

    std::clog << "Testing subtraction (1):" << std::endl;
    auto result_subtract = lhs - rhs; // should add and result to 2x
    dump_digits(lhs);
    std::clog << " - ";
    dump_digits(rhs);
    std::clog << " = ";
    dump_digits(result_subtract);
    std::clog << std::endl;

    std::clog << "Testing subtraction (2):" << std::endl;
    auto result_subtract2 = lhs - zero;
    dump_digits(lhs);
    std::clog << " - ";
    dump_digits(zero);
    std::clog << " = ";
    dump_digits(result_subtract2);
    std::clog << std::endl;

    std::clog << "Testing subtraction (3):" << std::endl;
    auto result_subtract3 = zero - lhs;
    dump_digits(zero);
    std::clog << " - ";
    dump_digits(lhs);
    std::clog << " = ";
    dump_digits(result_subtract3);
    std::clog << std::endl;

    assert(result_add.Is_Positive() == true);
    assert(result_subtract.is_positive == true);
    assert(result_add.Is_Zero());
    assert(result_subtract == Large_Numbers(9876543210 * 2));

    // now unit-test PRIVATE methods internal to THIS class (do NOT try to use
    // this method on public)
    auto my_large_number = Large_Numbers("0");
    std::function<std::uint64_t(std::uint64_t)> fn_fib_rec;
    fn_fib_rec = [&fn_fib_rec](std::uint64_t n) -> std::uint64_t {
      if (n == 0)
        return 0;
      if (n == 1)
        return 1;
      return fn_fib_rec(n - 1) + fn_fib_rec(n - 2);
    };

    auto fn_fib_iterator = [&fn_fib_rec](std::uint64_t n) -> std::uint64_t {
      if (n == 0)
        return 0;
      if (n == 1)
        return 1;
      auto n_minus_1 = 1;
      auto n_minus_2 = 0;
      auto result = 0;
      for (auto i = 2; i <= n; i++) {
        result = n_minus_1 + n_minus_2;
        n_minus_2 = n_minus_1;
        n_minus_1 = result;
      }
      return result;
    };

    auto index = 12;
    std::clog << "Fibonacci_rec(" << index << ") = " << fn_fib_rec(index)
              << std::endl;
    std::clog << "Fibonacci_iter(" << index << ") = " << fn_fib_iterator(index)
              << std::endl;
    // NOTE: all non-public INTERNAL functions deals with digits in reversed
    // order (i.e. "1234" is stored as {4, 3, 2, 1} )
    auto expected_reversed_tup =
        my_large_number.i64_to_digits(fn_fib_rec(index));
    std::clog << "Expected: ";
    // dump_digits will take input in reverse order, so we'll just pass the
    my_large_number.dump_digits(expected_reversed_tup);
    std::clog << std::endl;
    auto result_reversed = my_large_number.fibonacci_large_iter(index);
    std::clog << "Result: ";
    my_large_number.dump_digits(result_reversed);
    std::clog << std::endl;
    // assert(op_equal(expected, result));

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::clog << "Unit test duration: " << diff.count() << " s" << std::endl;
  }
};
} // namespace libs
} // namespace hairev

#endif // LARGE_NUMBERS
