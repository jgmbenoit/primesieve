///
/// @file   PrimeFinder.cpp
/// @brief  Callback, print and count primes and prime k-tuplets
///         (twin primes, prime triplets, ...).
///
/// Copyright (C) 2017 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <primesieve/config.hpp>
#include <primesieve/Callback.hpp>
#include <primesieve/littleendian_cast.hpp>
#include <primesieve/pmath.hpp>
#include <primesieve/PrimeFinder.hpp>
#include <primesieve/PrimeSieve.hpp>
#include <primesieve/SieveOfEratosthenes.hpp>

#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace primesieve {

uint64_t popcount(const uint64_t* array, uint64_t size);

const uint_t PrimeFinder::kBitmasks_[6][5] =
{
  { END },
  { 0x06, 0x18, 0xc0, END },       // Twin prime       bitmasks, i.e. b00000110, b00011000, b11000000
  { 0x07, 0x0e, 0x1c, 0x38, END }, // Prime triplet    bitmasks, i.e. b00000111, b00001110, ...
  { 0x1e, END },                   // Prime quadruplet bitmasks
  { 0x1f, 0x3e, END },             // Prime quintuplet bitmasks
  { 0x3f, END }                    // Prime sextuplet  bitmasks
};

PrimeFinder::PrimeFinder(PrimeSieve& ps, const PreSieve& preSieve) :
  SieveOfEratosthenes(std::max<uint64_t>(7, ps.getStart()),
                      ps.getStop(),
                      ps.getSieveSize(),
                      preSieve),
  ps_(ps),
  counts_(ps_.getCounts())
{
  if (ps_.isFlag(ps_.COUNT_TWINS, ps_.COUNT_SEXTUPLETS))
    init_kCounts();
}

/// Calculate the number of twins, triplets, ...
/// for each possible byte value 0 - 255.
///
void PrimeFinder::init_kCounts()
{
  for (uint_t i = 1; i < counts_.size(); i++)
  {
    if (ps_.isCount(i))
    {
      kCounts_[i].resize(256);
      for (uint_t j = 0; j < kCounts_[i].size(); j++)
      {
        uint_t bitmaskCount = 0;
        for (const uint_t* b = kBitmasks_[i]; *b <= j; b++)
        {
          if ((j & *b) == *b)
            bitmaskCount++;
        }
        kCounts_[i][j] = bitmaskCount;
      }
    }
  }
}

/// Executed after each sieved segment.
/// @see sieveSegment() in SieveOfEratosthenes.cpp
///
void PrimeFinder::generatePrimes(const byte_t* sieve, uint_t sieveSize)
{
  if (ps_.isCallback())
    callbackPrimes(ps_.getCallback(), sieve, sieveSize);
  if (ps_.isCount())
    count(sieve, sieveSize);
  if (ps_.isPrint())
    print(sieve, sieveSize);
  if (ps_.isStatus())
    ps_.updateStatus(sieveSize * NUMBERS_PER_BYTE);
}

void PrimeFinder::callbackPrimes(Callback& cb, const byte_t* sieve, uint_t sieveSize) const
{
  uint64_t low = getSegmentLow();
  for (uint_t i = 0; i < sieveSize; i += 8, low += NUMBERS_PER_BYTE * 8)
  {
    uint64_t bits = littleendian_cast<uint64_t>(&sieve[i]); 
    while (bits != 0)
    {
      uint64_t prime = getNextPrime(&bits, low);
      cb.callback(prime);
    }
  }
}

/// Count the primes and prime k-tuplets in
/// the current segment.
///
void PrimeFinder::count(const byte_t* sieve, uint_t sieveSize)
{
  if (ps_.isFlag(ps_.COUNT_PRIMES))
    counts_[0] += popcount((const uint64_t*) sieve, ceilDiv(sieveSize, 8));

  // count prime k-tuplets (i = 1 twins, i = 2 triplets, ...)
  for (uint_t i = 1; i < counts_.size(); i++)
  {
    if (ps_.isCount(i))
    {
      uint_t sum = 0;

      for (uint_t j = 0; j < sieveSize; j += 4)
      {
        sum += kCounts_[i][sieve[j+0]];
        sum += kCounts_[i][sieve[j+1]];
        sum += kCounts_[i][sieve[j+2]];
        sum += kCounts_[i][sieve[j+3]];
      }

      counts_[i] += sum;
    }
  }
}

/// Print primes and prime k-tuplets to cout.
/// primes <= 5 are handled in processSmallPrimes().
///
void PrimeFinder::print(const byte_t* sieve, uint_t sieveSize) const
{
  if (ps_.isFlag(ps_.PRINT_PRIMES))
  {
    uint64_t low = getSegmentLow();
    for (uint_t i = 0; i < sieveSize; i += 8, low += NUMBERS_PER_BYTE * 8)
    {
      uint64_t bits = littleendian_cast<uint64_t>(&sieve[i]); 
      while (bits != 0)
      {
        uint64_t prime = getNextPrime(&bits, low);
        std::cout << prime << '\n';
      }
    }
  }

  // print prime k-tuplets
  if (ps_.isFlag(ps_.PRINT_TWINS, ps_.PRINT_SEXTUPLETS))
  {
    uint_t i = 1; // i = 1 twins, i = 2 triplets, ...
    uint64_t low = getSegmentLow();

    for (; !ps_.isPrint(i); i++);
    for (uint_t j = 0; j < sieveSize; j++, low += NUMBERS_PER_BYTE)
    {
      for (const uint_t* bitmask = kBitmasks_[i]; *bitmask <= sieve[j]; bitmask++)
      {
        if ((sieve[j] & *bitmask) == *bitmask)
        {
          std::ostringstream kTuplet;
          kTuplet << "(";
          uint64_t bits = *bitmask;
          while (bits != 0)
          {
            kTuplet << getNextPrime(&bits, low);
            kTuplet << ((bits != 0) ? ", " : ")\n");
          }
          std::cout << kTuplet.str();
        }
      }
    }
  }
}

} // namespace
