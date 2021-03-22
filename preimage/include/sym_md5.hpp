/*
 * Hash reversal
 *
 * Copyright (c) 2020 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#pragma once

#include <utility>
#include <vector>
#include <assert.h>

#include "sym_bit_vec.hpp"
#include "sym_hash.hpp"

#define MD5_BLOCK_SIZE 64
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

namespace preimage {

class MD5 : public SymHash {
 public:
  MD5() {
    finalized = false;

    count[0] = 0;
    count[1] = 0;

    state[0] = SymBitVec(0x67452301, 32);
    state[1] = SymBitVec(0xefcdab89, 32);
    state[2] = SymBitVec(0x98badcfe, 32);
    state[3] = SymBitVec(0x10325476, 32);

    const std::vector<uint32_t> raw_constants = {
      0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
      0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
      0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
      0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x2441453, 0xd8a1e681, 0xe7d3fbc8,
      0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
      0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
      0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
      0xd4ef3085, 0x4881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
      0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
      0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
      0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
    };
    constants_ = {};
    for (uint32_t c : raw_constants) constants_.push_back(SymBitVec(c, 32));
  }

  SymBitVec hash(const SymBitVec &hash_input, int difficulty) override {
    // Input size must be byte-aligned
    assert(hash_input.size() % 8 == 0);
    const size_t n_bytes = hash_input.size() / 8;
    SymBitVec input[n_bytes];
    for (size_t i = 0; i < n_bytes; i++) {
      input[i] = hash_input.extract(i * 8, (i + 1) * 8);
    }

    update(input, n_bytes);
    finalize();

    SymBitVec combined_digest;
    for (size_t i = 0; i < 16; i++) {
      combined_digest = digest[i].concat(combined_digest);
    }
    return combined_digest;
  }

 private:
  void decode(SymBitVec output[], const SymBitVec input[], size_t len) {
    // Decode input (8 bits per SymBitVec) into output (32 bits per SymBitVec)
    assert(len % 4 == 0);
    for (unsigned int i = 0, j = 0; j < len; i++, j += 4) {
      output[i] = (input[j].resize(32)) |
                  (input[j + 1].resize(32) << 8) |
                  (input[j + 2].resize(32) << 16) |
                  (input[j + 3].resize(32) << 24);
    }
  }

  void encode(SymBitVec output[], const SymBitVec input[], size_t len) {
    // Encode input (32 bits per SymBitVec) into output (8 bits per SymBitVec)
    assert(len % 4 == 0);
    for (size_t i = 0, j = 0; j < len; i++, j += 4) {
      output[j] = input[i].extract(0, 8);
      output[j + 1] = (input[i] >> 8).extract(0, 8);
      output[j + 2] = (input[i] >> 16).extract(0, 8);
      output[j + 3] = (input[i] >> 24).extract(0, 8);
    }
  }

  void update(const SymBitVec input[], size_t len) {
    // Each SymBitVec in `input` has 8 bits
    // Compute number of bytes mod 64
    size_t index = count[0] / 8 % MD5_BLOCK_SIZE;

    // Update number of bits
    if ((count[0] += (len << 3)) < (len << 3)) count[1]++;
    count[1] += (len >> 29);

    // Number of bytes we need to fill in `buffer`
    size_t firstpart = 64 - index;

    size_t i;

    // Transform as many times as possible.
    if (len >= firstpart) {
      // Fill buffer first, then transform
      // memcpy(&buffer[index], input, firstpart);
      for (size_t j = index; j < index + firstpart; j++) {
        buffer[j] = input[j - index];
      }

      transform(buffer);

      // transform chunks of blocksize (64 bytes)
      for (i = firstpart; i + MD5_BLOCK_SIZE <= len; i += MD5_BLOCK_SIZE) {
        transform(&input[i]);
      }

      index = 0;
    } else {
      i = 0;
    }

    // Buffer remaining input
    // memcpy(&buffer[index], &input[i], length - i);
    for (size_t j = index; j < index + len - i; j++) {
      buffer[j] = input[i + j - index];
    }
  }

  void transform(const SymBitVec block[MD5_BLOCK_SIZE]) {
    // Each SymBitVec in `block` is 8 bits
    SymBitVec a = state[0], b = state[1], c = state[2], d = state[3];
    SymBitVec x[16];
    decode(x, block, MD5_BLOCK_SIZE);

    size_t i = 0;

    /* Round 1 */
    FF(a, b, c, d, x[ 0], S11, constants_.at(i)); i++; /* 1 */
    FF(d, a, b, c, x[ 1], S12, constants_.at(i)); i++; /* 2 */
    FF(c, d, a, b, x[ 2], S13, constants_.at(i)); i++; /* 3 */
    FF(b, c, d, a, x[ 3], S14, constants_.at(i)); i++; /* 4 */
    FF(a, b, c, d, x[ 4], S11, constants_.at(i)); i++; /* 5 */
    FF(d, a, b, c, x[ 5], S12, constants_.at(i)); i++; /* 6 */
    FF(c, d, a, b, x[ 6], S13, constants_.at(i)); i++; /* 7 */
    FF(b, c, d, a, x[ 7], S14, constants_.at(i)); i++; /* 8 */
    FF(a, b, c, d, x[ 8], S11, constants_.at(i)); i++; /* 9 */
    FF(d, a, b, c, x[ 9], S12, constants_.at(i)); i++; /* 10 */
    FF(c, d, a, b, x[10], S13, constants_.at(i)); i++; /* 11 */
    FF(b, c, d, a, x[11], S14, constants_.at(i)); i++; /* 12 */
    FF(a, b, c, d, x[12], S11, constants_.at(i)); i++; /* 13 */
    FF(d, a, b, c, x[13], S12, constants_.at(i)); i++; /* 14 */
    FF(c, d, a, b, x[14], S13, constants_.at(i)); i++; /* 15 */
    FF(b, c, d, a, x[15], S14, constants_.at(i)); i++; /* 16 */

    /* Round 2 */
    GG(a, b, c, d, x[ 1], S21, constants_.at(i)); i++; /* 17 */
    GG(d, a, b, c, x[ 6], S22, constants_.at(i)); i++; /* 18 */
    GG(c, d, a, b, x[11], S23, constants_.at(i)); i++; /* 19 */
    GG(b, c, d, a, x[ 0], S24, constants_.at(i)); i++; /* 20 */
    GG(a, b, c, d, x[ 5], S21, constants_.at(i)); i++; /* 21 */
    GG(d, a, b, c, x[10], S22, constants_.at(i)); i++; /* 22 */
    GG(c, d, a, b, x[15], S23, constants_.at(i)); i++; /* 23 */
    GG(b, c, d, a, x[ 4], S24, constants_.at(i)); i++; /* 24 */
    GG(a, b, c, d, x[ 9], S21, constants_.at(i)); i++; /* 25 */
    GG(d, a, b, c, x[14], S22, constants_.at(i)); i++; /* 26 */
    GG(c, d, a, b, x[ 3], S23, constants_.at(i)); i++; /* 27 */
    GG(b, c, d, a, x[ 8], S24, constants_.at(i)); i++; /* 28 */
    GG(a, b, c, d, x[13], S21, constants_.at(i)); i++; /* 29 */
    GG(d, a, b, c, x[ 2], S22, constants_.at(i)); i++; /* 30 */
    GG(c, d, a, b, x[ 7], S23, constants_.at(i)); i++; /* 31 */
    GG(b, c, d, a, x[12], S24, constants_.at(i)); i++; /* 32 */

    /* Round 3 */
    HH(a, b, c, d, x[ 5], S31, constants_.at(i)); i++; /* 33 */
    HH(d, a, b, c, x[ 8], S32, constants_.at(i)); i++; /* 34 */
    HH(c, d, a, b, x[11], S33, constants_.at(i)); i++; /* 35 */
    HH(b, c, d, a, x[14], S34, constants_.at(i)); i++; /* 36 */
    HH(a, b, c, d, x[ 1], S31, constants_.at(i)); i++; /* 37 */
    HH(d, a, b, c, x[ 4], S32, constants_.at(i)); i++; /* 38 */
    HH(c, d, a, b, x[ 7], S33, constants_.at(i)); i++; /* 39 */
    HH(b, c, d, a, x[10], S34, constants_.at(i)); i++; /* 40 */
    HH(a, b, c, d, x[13], S31, constants_.at(i)); i++; /* 41 */
    HH(d, a, b, c, x[ 0], S32, constants_.at(i)); i++; /* 42 */
    HH(c, d, a, b, x[ 3], S33, constants_.at(i)); i++; /* 43 */
    HH(b, c, d, a, x[ 6], S34, constants_.at(i)); i++; /* 44 */
    HH(a, b, c, d, x[ 9], S31, constants_.at(i)); i++; /* 45 */
    HH(d, a, b, c, x[12], S32, constants_.at(i)); i++; /* 46 */
    HH(c, d, a, b, x[15], S33, constants_.at(i)); i++; /* 47 */
    HH(b, c, d, a, x[ 2], S34, constants_.at(i)); i++; /* 48 */

    /* Round 4 */
    II(a, b, c, d, x[ 0], S41, constants_.at(i)); i++; /* 49 */
    II(d, a, b, c, x[ 7], S42, constants_.at(i)); i++; /* 50 */
    II(c, d, a, b, x[14], S43, constants_.at(i)); i++; /* 51 */
    II(b, c, d, a, x[ 5], S44, constants_.at(i)); i++; /* 52 */
    II(a, b, c, d, x[12], S41, constants_.at(i)); i++; /* 53 */
    II(d, a, b, c, x[ 3], S42, constants_.at(i)); i++; /* 54 */
    II(c, d, a, b, x[10], S43, constants_.at(i)); i++; /* 55 */
    II(b, c, d, a, x[ 1], S44, constants_.at(i)); i++; /* 56 */
    II(a, b, c, d, x[ 8], S41, constants_.at(i)); i++; /* 57 */
    II(d, a, b, c, x[15], S42, constants_.at(i)); i++; /* 58 */
    II(c, d, a, b, x[ 6], S43, constants_.at(i)); i++; /* 59 */
    II(b, c, d, a, x[13], S44, constants_.at(i)); i++; /* 60 */
    II(a, b, c, d, x[ 4], S41, constants_.at(i)); i++; /* 61 */
    II(d, a, b, c, x[11], S42, constants_.at(i)); i++; /* 62 */
    II(c, d, a, b, x[ 2], S43, constants_.at(i)); i++; /* 63 */
    II(b, c, d, a, x[ 9], S44, constants_.at(i)); i++; /* 64 */

    state[0] = state[0] + a;
    state[1] = state[1] + b;
    state[2] = state[2] + c;
    state[3] = state[3] + d;
  }

  void finalize() {
    if (finalized) return;

    static uint8_t padding_raw[64] = {
      0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    SymBitVec padding[64];
    for (size_t i = 0; i < 64; i++) padding[i] = SymBitVec(padding_raw[i], 8);

    // Save number of bits
    SymBitVec bits[8];  // 8 bits per SymBitVec
    SymBitVec count_bv[2];
    count_bv[0] = SymBitVec(count[0], 32);
    count_bv[1] = SymBitVec(count[1], 32);
    encode(bits, count_bv, 8);

    // pad out to 56 mod 64.
    size_t index = count[0] / 8 % 64;
    size_t pad_len = (index < 56) ? (56 - index) : (120 - index);
    update(padding, pad_len);

    // Append length (before padding)
    update(bits, 8);

    // Store state in digest
    encode(digest, state, 16);

    finalized = true;
  }

  static inline SymBitVec F(const SymBitVec &x, const SymBitVec &y,
                            const SymBitVec &z) {
    return (x & y) | (~x & z);
  }

  static inline SymBitVec G(const SymBitVec &x, const SymBitVec &y,
                            const SymBitVec &z) {
    return (x & z) | (y & ~z);
  }

  static inline SymBitVec H(const SymBitVec &x, const SymBitVec &y,
                            const SymBitVec &z) {
    return x ^ y ^ z;
  }

  static inline SymBitVec I(const SymBitVec &x, const SymBitVec &y,
                            const SymBitVec &z) {
    return y ^ (x | ~z);
  }

  static inline SymBitVec rotateLeft(const SymBitVec &x, int n) {
    return (x << n) | (x >> (32 - n));
  }

  static inline void FF(SymBitVec &a, const SymBitVec &b, const SymBitVec &c,
                        const SymBitVec &d, const SymBitVec &x, uint32_t s,
                        const SymBitVec &ac) {
    a = rotateLeft(a + F(b, c, d) + x + ac, s) + b;
  }

  static inline void GG(SymBitVec &a, const SymBitVec &b, const SymBitVec &c,
                        const SymBitVec &d, const SymBitVec &x, uint32_t s,
                        const SymBitVec &ac) {
    a = rotateLeft(a + G(b, c, d) + x + ac, s) + b;
  }

  static inline void HH(SymBitVec &a, const SymBitVec &b, const SymBitVec &c,
                        const SymBitVec &d, const SymBitVec &x, uint32_t s,
                        const SymBitVec &ac) {
    a = rotateLeft(a + H(b, c, d) + x + ac, s) + b;
  }

  static inline void II(SymBitVec &a, const SymBitVec &b, const SymBitVec &c,
                        const SymBitVec &d, const SymBitVec &x, uint32_t s,
                        const SymBitVec &ac) {
    a = rotateLeft(a + I(b, c, d) + x + ac, s) + b;
  }

  bool finalized;

  // Bytes that didn't fit into last 64-byte chunk.
  // Each SymBitVec is 8 bits.
  SymBitVec buffer[MD5_BLOCK_SIZE];

  // 64-bit counter for number of bits (lo, hi).
  uint32_t count[2];

  // Digest so far. Each SymBitVec is 32 bits.
  SymBitVec state[4];

  // Final digest result. Each SymBitVec is 8 bits.
  SymBitVec digest[16];

  // Known constants
  std::vector<SymBitVec> constants_;
};

}  // end namespace preimage

#undef S11
#undef S12
#undef S13
#undef S14
#undef S21
#undef S22
#undef S23
#undef S24
#undef S31
#undef S32
#undef S33
#undef S34
#undef S41
#undef S42
#undef S43
#undef S44