// Copyright 2010 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "config.h"

#include <climits>
#include <locale>
#include <cmath>

#include <wtf/dtoa/double-conversion.h>

#include <wtf/dtoa/bignum-dtoa.h>
#include <wtf/dtoa/fast-dtoa.h>
#include <wtf/dtoa/fixed-dtoa.h>
#include <wtf/dtoa/ieee.h>
#include <wtf/dtoa/strtod.h>
#include <wtf/dtoa/utils.h>

#include <wtf/ASCIICType.h>

namespace WTF {
namespace double_conversion {

const DoubleToStringConverter& DoubleToStringConverter::EcmaScriptConverter() {
  int flags = UNIQUE_ZERO | EMIT_POSITIVE_EXPONENT_SIGN;
#if !PLATFORM(WKC)
  static DoubleToStringConverter converter(flags,
                                           "Infinity",
                                           "NaN",
                                           'e',
                                           -6, 21,
                                           6, 0);
  return converter;
#else
  WKC_DEFINE_STATIC_TYPE(DoubleToStringConverter*, converter, 
    new DoubleToStringConverter(flags,
                                "Infinity",
                                "NaN",
                                'e',
                                -6, 21,
                                 6, 0));
  return *converter;
#endif
}


bool DoubleToStringConverter::HandleSpecialValues(
    double value,
    StringBuilder* result_builder) const {
  Double double_inspect(value);
  if (double_inspect.IsInfinite()) {
    if (infinity_symbol_ == nullptr) return false;
    if (value < 0) {
      result_builder->AddCharacter('-');
    }
    result_builder->AddString(infinity_symbol_);
    return true;
  }
  if (double_inspect.IsNan()) {
    if (nan_symbol_ == nullptr) return false;
    result_builder->AddString(nan_symbol_);
    return true;
  }
  return false;
}


void DoubleToStringConverter::CreateExponentialRepresentation(
    const char* decimal_digits,
    int length,
    int exponent,
    StringBuilder* result_builder) const {
  ASSERT(length != 0);
  result_builder->AddCharacter(decimal_digits[0]);
  if (length != 1) {
    result_builder->AddCharacter('.');
    result_builder->AddSubstring(&decimal_digits[1], length-1);
  }
  result_builder->AddCharacter(exponent_character_);
  if (exponent < 0) {
    result_builder->AddCharacter('-');
    exponent = -exponent;
  } else {
    if ((flags_ & EMIT_POSITIVE_EXPONENT_SIGN) != 0) {
      result_builder->AddCharacter('+');
    }
  }
  if (exponent == 0) {
    result_builder->AddCharacter('0');
    return;
  }
  ASSERT(exponent < 1e4);
  const int kMaxExponentLength = 5;
  char buffer[kMaxExponentLength + 1];
  buffer[kMaxExponentLength] = '\0';
  int first_char_pos = kMaxExponentLength;
  while (exponent > 0) {
    buffer[--first_char_pos] = '0' + (exponent % 10);
    exponent /= 10;
  }
  result_builder->AddSubstring(&buffer[first_char_pos],
                               kMaxExponentLength - first_char_pos);
}


void DoubleToStringConverter::CreateDecimalRepresentation(
    const char* decimal_digits,
    int length,
    int decimal_point,
    int digits_after_point,
    StringBuilder* result_builder) const {
  // Create a representation that is padded with zeros if needed.
  if (decimal_point <= 0) {
      // "0.00000decimal_rep" or "0.000decimal_rep00".
    result_builder->AddCharacter('0');
    if (digits_after_point > 0) {
      result_builder->AddCharacter('.');
      result_builder->AddPadding('0', -decimal_point);
      ASSERT(length <= digits_after_point - (-decimal_point));
      result_builder->AddSubstring(decimal_digits, length);
      int remaining_digits = digits_after_point - (-decimal_point) - length;
      result_builder->AddPadding('0', remaining_digits);
    }
  } else if (decimal_point >= length) {
    // "decimal_rep0000.00000" or "decimal_rep.0000".
    result_builder->AddSubstring(decimal_digits, length);
    result_builder->AddPadding('0', decimal_point - length);
    if (digits_after_point > 0) {
      result_builder->AddCharacter('.');
      result_builder->AddPadding('0', digits_after_point);
    }
  } else {
    // "decima.l_rep000".
    ASSERT(digits_after_point > 0);
    result_builder->AddSubstring(decimal_digits, decimal_point);
    result_builder->AddCharacter('.');
    ASSERT(length - decimal_point <= digits_after_point);
    result_builder->AddSubstring(&decimal_digits[decimal_point],
                                 length - decimal_point);
    int remaining_digits = digits_after_point - (length - decimal_point);
    result_builder->AddPadding('0', remaining_digits);
  }
  if (digits_after_point == 0) {
    if ((flags_ & EMIT_TRAILING_DECIMAL_POINT) != 0) {
      result_builder->AddCharacter('.');
    }
    if ((flags_ & EMIT_TRAILING_ZERO_AFTER_POINT) != 0) {
      result_builder->AddCharacter('0');
    }
  }
}


bool DoubleToStringConverter::ToShortestIeeeNumber(
    double value,
    StringBuilder* result_builder,
    DoubleToStringConverter::DtoaMode mode) const {
  ASSERT(mode == SHORTEST || mode == SHORTEST_SINGLE);
  if (Double(value).IsSpecial()) {
    return HandleSpecialValues(value, result_builder);
  }

  int decimal_point;
  bool sign;
  const int kDecimalRepCapacity = kBase10MaximalLength + 1;
  char decimal_rep[kDecimalRepCapacity];
  int decimal_rep_length;

  DoubleToAscii(value, mode, 0, decimal_rep, kDecimalRepCapacity,
                &sign, &decimal_rep_length, &decimal_point);

  bool unique_zero = (flags_ & UNIQUE_ZERO) != 0;
  if (sign && (value != 0.0 || !unique_zero)) {
    result_builder->AddCharacter('-');
  }

  int exponent = decimal_point - 1;
  if ((decimal_in_shortest_low_ <= exponent) &&
      (exponent < decimal_in_shortest_high_)) {
    CreateDecimalRepresentation(decimal_rep, decimal_rep_length,
                                decimal_point,
                                Max(0, decimal_rep_length - decimal_point),
                                result_builder);
  } else {
    CreateExponentialRepresentation(decimal_rep, decimal_rep_length, exponent,
                                    result_builder);
  }
  return true;
}


bool DoubleToStringConverter::ToFixed(double value,
                                      int requested_digits,
                                      StringBuilder* result_builder) const {
  ASSERT(kMaxFixedDigitsBeforePoint == 21);
  const double kFirstNonFixed = 1e21;

  if (Double(value).IsSpecial()) {
    return HandleSpecialValues(value, result_builder);
  }

  if (requested_digits > kMaxFixedDigitsAfterPoint) return false;
  if (value >= kFirstNonFixed || value <= -kFirstNonFixed) return false;

  // Find a sufficiently precise decimal representation of n.
  int decimal_point;
  bool sign;
  // Add space for the '\0' byte.
  const int kDecimalRepCapacity =
      kMaxFixedDigitsBeforePoint + kMaxFixedDigitsAfterPoint + 1;
  char decimal_rep[kDecimalRepCapacity];
  int decimal_rep_length;
  DoubleToAscii(value, FIXED, requested_digits,
                decimal_rep, kDecimalRepCapacity,
                &sign, &decimal_rep_length, &decimal_point);

  bool unique_zero = ((flags_ & UNIQUE_ZERO) != 0);
  if (sign && (value != 0.0 || !unique_zero)) {
    result_builder->AddCharacter('-');
  }

  CreateDecimalRepresentation(decimal_rep, decimal_rep_length, decimal_point,
                              requested_digits, result_builder);
  return true;
}


bool DoubleToStringConverter::ToExponential(
    double value,
    int requested_digits,
    StringBuilder* result_builder) const {
  if (Double(value).IsSpecial()) {
    return HandleSpecialValues(value, result_builder);
  }

  if (requested_digits < -1) return false;
  if (requested_digits > kMaxExponentialDigits) return false;

  int decimal_point;
  bool sign;
  // Add space for digit before the decimal point and the '\0' character.
  const int kDecimalRepCapacity = kMaxExponentialDigits + 2;
  ASSERT(kDecimalRepCapacity > kBase10MaximalLength);
  char decimal_rep[kDecimalRepCapacity];
  int decimal_rep_length;

  if (requested_digits == -1) {
    DoubleToAscii(value, SHORTEST, 0,
                  decimal_rep, kDecimalRepCapacity,
                  &sign, &decimal_rep_length, &decimal_point);
  } else {
    DoubleToAscii(value, PRECISION, requested_digits + 1,
                  decimal_rep, kDecimalRepCapacity,
                  &sign, &decimal_rep_length, &decimal_point);
    ASSERT(decimal_rep_length <= requested_digits + 1);

    if (decimal_rep_length < requested_digits + 1) {
      for (int i = decimal_rep_length; i < requested_digits + 1; ++i)
        decimal_rep[i] = '0';
      decimal_rep_length = requested_digits + 1;
      decimal_rep[decimal_rep_length] = '\0';
    }
  }

  bool unique_zero = ((flags_ & UNIQUE_ZERO) != 0);
  if (sign && (value != 0.0 || !unique_zero)) {
    result_builder->AddCharacter('-');
  }

  int exponent = decimal_point - 1;
  CreateExponentialRepresentation(decimal_rep,
                                  decimal_rep_length,
                                  exponent,
                                  result_builder);
  return true;
}


bool DoubleToStringConverter::ToPrecision(double value,
                                          int precision,
                                          StringBuilder* result_builder) const {
  if (Double(value).IsSpecial()) {
    return HandleSpecialValues(value, result_builder);
  }

  if (precision < kMinPrecisionDigits || precision > kMaxPrecisionDigits) {
    return false;
  }

  // Find a sufficiently precise decimal representation of n.
  int decimal_point;
  bool sign;
  // Add one for the terminating null character.
  const int kDecimalRepCapacity = kMaxPrecisionDigits + 1;
  char decimal_rep[kDecimalRepCapacity];
  int decimal_rep_length;

  DoubleToAscii(value, PRECISION, precision,
                decimal_rep, kDecimalRepCapacity,
                &sign, &decimal_rep_length, &decimal_point);
  ASSERT(decimal_rep_length <= precision);

  bool unique_zero = ((flags_ & UNIQUE_ZERO) != 0);
  if (sign && (value != 0.0 || !unique_zero)) {
    result_builder->AddCharacter('-');
  }

  // The exponent if we print the number as x.xxeyyy. That is with the
  // decimal point after the first digit.
  int exponent = decimal_point - 1;

  int extra_zero = ((flags_ & EMIT_TRAILING_ZERO_AFTER_POINT) != 0) ? 1 : 0;
  if ((-decimal_point + 1 > max_leading_padding_zeroes_in_precision_mode_) ||
      (decimal_point - precision + extra_zero >
       max_trailing_padding_zeroes_in_precision_mode_)) {
    // Fill buffer to contain 'precision' digits.
    // Usually the buffer is already at the correct length, but 'DoubleToAscii'
    // is allowed to return less characters.
    for (int i = decimal_rep_length; i < precision; ++i) {
      decimal_rep[i] = '0';
    }

    CreateExponentialRepresentation(decimal_rep,
                                    precision,
                                    exponent,
                                    result_builder);
  } else {
    CreateDecimalRepresentation(decimal_rep, decimal_rep_length, decimal_point,
                                Max(0, precision - decimal_point),
                                result_builder);
  }
  return true;
}


static BignumDtoaMode DtoaToBignumDtoaMode(
    DoubleToStringConverter::DtoaMode dtoa_mode) {
  switch (dtoa_mode) {
    case DoubleToStringConverter::SHORTEST:  return BIGNUM_DTOA_SHORTEST;
    case DoubleToStringConverter::SHORTEST_SINGLE:
        return BIGNUM_DTOA_SHORTEST_SINGLE;
    case DoubleToStringConverter::FIXED:     return BIGNUM_DTOA_FIXED;
    case DoubleToStringConverter::PRECISION: return BIGNUM_DTOA_PRECISION;
    default:
      UNREACHABLE();
  }
}


void DoubleToStringConverter::DoubleToAscii(double v,
                                            DtoaMode mode,
                                            int requested_digits,
                                            char* buffer,
                                            int buffer_length,
                                            bool* sign,
                                            int* length,
                                            int* point) {
  BufferReference<char> bufferReference(buffer, buffer_length);
  ASSERT(!Double(v).IsSpecial());
  ASSERT(mode == SHORTEST || mode == SHORTEST_SINGLE || requested_digits >= 0);

  if (Double(v).Sign() < 0) {
    *sign = true;
    v = -v;
  } else {
    *sign = false;
  }

  if (mode == PRECISION && requested_digits == 0) {
    bufferReference[0] = '\0';
    *length = 0;
    return;
  }

  if (v == 0) {
    bufferReference[0] = '0';
    bufferReference[1] = '\0';
    *length = 1;
    *point = 1;
    return;
  }

  bool fast_worked;
  switch (mode) {
    case SHORTEST:
      fast_worked = FastDtoa(v, FAST_DTOA_SHORTEST, 0, bufferReference, length, point);
      break;
    case SHORTEST_SINGLE:
      fast_worked = FastDtoa(v, FAST_DTOA_SHORTEST_SINGLE, 0,
                             bufferReference, length, point);
      break;
    case FIXED:
      fast_worked = FastFixedDtoa(v, requested_digits, bufferReference, length, point);
      break;
    case PRECISION:
      fast_worked = FastDtoa(v, FAST_DTOA_PRECISION, requested_digits,
                             bufferReference, length, point);
      break;
    default:
      fast_worked = false;
      UNREACHABLE();
  }
  if (fast_worked) return;

  // If the fast dtoa didn't succeed use the slower bignum version.
  BignumDtoaMode bignum_mode = DtoaToBignumDtoaMode(mode);
  BignumDtoa(v, bignum_mode, requested_digits, bufferReference, length, point);
  bufferReference[*length] = '\0';
}

// Maximum number of significant digits in decimal representation.
// The longest possible double in decimal representation is
// (2^53 - 1) * 2 ^ -1074 that is (2 ^ 53 - 1) * 5 ^ 1074 / 10 ^ 1074
// (768 digits). If we parse a number whose first digits are equal to a
// mean of 2 adjacent doubles (that could have up to 769 digits) the result
// must be rounded to the bigger one unless the tail consists of zeros, so
// we don't need to preserve all the digits.
const int kMaxSignificantDigits = 772;


static double SignedZero(bool sign) {
  return sign ? -0.0 : 0.0;
}


// Returns true, when the iterator is equal to end.
template<class Iterator>
static inline bool Advance(Iterator* it, Iterator& end) {
  ++(*it);
  return *it == end;
}

template <typename FloatingPointType>
inline FloatingPointType StringToFloatingPointType(BufferReference<const char> buffer, int exponent);

template <>
inline double StringToFloatingPointType<double>(BufferReference<const char> buffer, int exponent) {
  return Strtod(buffer, exponent);
}

template <>
inline float StringToFloatingPointType<float>(BufferReference<const char> buffer, int exponent) {
  return Strtof(buffer, exponent);
}

template <typename FloatingPointType, class Iterator>
static FloatingPointType StringToIeee(
    Iterator input,
    size_t length,
    size_t* processed_characters_count) {
  static_assert(std::is_floating_point<FloatingPointType>::value, "Only floating point types are allowed.");

  Iterator current = input;
  Iterator end = input + length;

  *processed_characters_count = 0;

  // To make sure that iterator dereferencing is valid the following
  // convention is used:
  // 1. Each '++current' statement is followed by check for equality to 'end'.
  // 3. If 'current' becomes equal to 'end' the function returns or goes to
  // 'parsing_done'.
  // 4. 'current' is not dereferenced after the 'parsing_done' label.
  // 5. Code before 'parsing_done' may rely on 'current != end'.

  if (current == end) return 0.0;

  // The longest form of simplified number is: "-<significant digits>.1eXXX\0".
  const int kBufferSize = kMaxSignificantDigits + 10;
  char buffer[kBufferSize];  // NOLINT: size is known at compile time.
  int buffer_pos = 0;

  // Exponent will be adjusted if insignificant digits of the integer part
  // or insignificant leading zeros of the fractional part are dropped.
  int exponent = 0;
  int significant_digits = 0;
  int insignificant_digits = 0;
  bool nonzero_digit_dropped = false;

  bool sign = false;

  if (*current == '+' || *current == '-') {
    sign = (*current == '-');
    ++current;
    if (current == end) return 0.0;
  }

  bool leading_zero = false;
  if (*current == '0') {
    if (Advance(&current, end)) {
      *processed_characters_count = static_cast<size_t>(current - input);
      return SignedZero(sign);
    }

    leading_zero = true;

    // Ignore leading zeros in the integer part.
    while (*current == '0') {
      if (Advance(&current, end)) {
        *processed_characters_count = static_cast<size_t>(current - input);
        return SignedZero(sign);
      }
    }
  }

  // Copy significant digits of the integer part (if any) to the buffer.
  while (isASCIIDigit(*current)) {
    if (significant_digits < kMaxSignificantDigits) {
      ASSERT(buffer_pos < kBufferSize);
      buffer[buffer_pos++] = static_cast<char>(*current);
      significant_digits++;
    } else {
      insignificant_digits++;  // Move the digit into the exponential part.
      nonzero_digit_dropped = nonzero_digit_dropped || *current != '0';
    }
    if (Advance(&current, end)) goto parsing_done;
  }

  if (*current == '.') {
    if (Advance(&current, end)) {
      if (significant_digits == 0 && !leading_zero) {
        return 0.0;
      } else {
        goto parsing_done;
      }
    }

    if (significant_digits == 0) {
      // Integer part consists of 0 or is absent. Significant digits start after
      // leading zeros (if any).
      while (*current == '0') {
        if (Advance(&current, end)) {
          *processed_characters_count = static_cast<size_t>(current - input);
          return SignedZero(sign);
        }
        exponent--;  // Move this 0 into the exponent.
      }
    }

    // There is a fractional part.
    // We don't emit a '.', but adjust the exponent instead.
    while (isASCIIDigit(*current)) {
      if (significant_digits < kMaxSignificantDigits) {
        ASSERT(buffer_pos < kBufferSize);
        buffer[buffer_pos++] = static_cast<char>(*current);
        significant_digits++;
        exponent--;
      } else {
        // Ignore insignificant digits in the fractional part.
        nonzero_digit_dropped = nonzero_digit_dropped || *current != '0';
      }
      if (Advance(&current, end)) goto parsing_done;
    }
  }

  if (!leading_zero && exponent == 0 && significant_digits == 0) {
    // If leading_zeros is true then the string contains zeros.
    // If exponent < 0 then string was [+-]\.0*...
    // If significant_digits != 0 the string is not equal to 0.
    // Otherwise there are no digits in the string.
    return 0.0;
  }

  // Parse exponential part.
  if (*current == 'e' || *current == 'E') {
    ++current;
    if (current == end) {
      --current;
      goto parsing_done;
    }
    char exponen_sign = 0;
    if (*current == '+' || *current == '-') {
      exponen_sign = static_cast<char>(*current);
      ++current;
      if (current == end) {
        current -= 2;
        goto parsing_done;
      }
    }

    if (*current < '0' || *current > '9') {
      if (exponen_sign)
        --current;
      --current;
      goto parsing_done;
    }

    const int max_exponent = INT_MAX / 2;
    ASSERT(-max_exponent / 2 <= exponent && exponent <= max_exponent / 2);
    int num = 0;
    do {
      // Check overflow.
      int digit = *current - '0';
      if (num >= max_exponent / 10
          && !(num == max_exponent / 10 && digit <= max_exponent % 10)) {
        num = max_exponent;
      } else {
        num = num * 10 + digit;
      }
      ++current;
    } while (current != end && isASCIIDigit(*current));

    exponent += (exponen_sign == '-' ? -num : num);
  }

  parsing_done:
  exponent += insignificant_digits;

  if (nonzero_digit_dropped) {
    buffer[buffer_pos++] = '1';
    exponent--;
  }

  ASSERT(buffer_pos < kBufferSize);
  buffer[buffer_pos] = '\0';

  auto converted = StringToFloatingPointType<FloatingPointType>(BufferReference<const char>(buffer, buffer_pos), exponent);
  *processed_characters_count = static_cast<size_t>(current - input);
  return sign? -converted: converted;
}

double StringToDoubleConverter::StringToDouble(
    const char* buffer,
    size_t length,
    size_t* processed_characters_count) {
  return StringToIeee<double>(buffer, length, processed_characters_count);
}


double StringToDoubleConverter::StringToDouble(
    const uc16* buffer,
    size_t length,
    size_t* processed_characters_count) {
  return StringToIeee<double>(buffer, length, processed_characters_count);
}


float StringToDoubleConverter::StringToFloat(
    const char* buffer,
    size_t length,
    size_t* processed_characters_count) {
  return StringToIeee<float>(buffer, length, processed_characters_count);
}


float StringToDoubleConverter::StringToFloat(
    const uc16* buffer,
    size_t length,
    size_t* processed_characters_count) {
  return StringToIeee<float>(buffer, length, processed_characters_count);
}

}  // namespace double_conversion
}  // namespace WTF
