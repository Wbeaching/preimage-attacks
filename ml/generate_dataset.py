# -*- coding: utf-8 -*-
import os
import sys
import random
import hashlib
import argparse
from binascii import unhexlify

import nsha256
from utils import constants
from utils.constants import BIT_PRED, HASH_INPUT_NBITS

"""
This generates a CSV dataset of random hash inputs of length 64 bits
and their SHA-256 hashes (256 bits).

Each line in the generated file is a comma-separated list of the
256 hash bits followed by the bits of the hash input, and finally
(and optionally) additional bits from computations which occurred
during the hash algorithm.
"""


def int2bytes(val):
  # one (1) hex digit per four (4) bits
  width = val.bit_length()

  # unhexlify wants an even multiple of eight (8) bits, but we don't
  # want more digits than we need (hence the ternary-ish 'or')
  width += 8 - ((width % 8) or 8)

  # format width specifier: four (4) bits per hex digit
  fmt = '%%0%dx' % (width // 4)
  
  # unhexlify requires an even number of hex digits
  s = fmt % val
  if len(s) % 2 != 0:
    s = '0' + s

  return unhexlify(s)


def randomBinaryString(nbits):
  x = random.randint(0, int(2**nbits) - 1)
  binstr = bin(x)[2:]
  num_zero_padding = nbits - len(binstr)
  return ('0' * num_zero_padding) + binstr


def hashFunc(binstr, hash_mode):
  """
  Parameters:
   - binstr: the input to the hash function (a string of bits, 0 or 1)
   - hash_mode: control which hash function is used (real SHA-256 or something for testing)
  Returns:
   - A tuple where the first element is a 256-bit binary string of 0's and 1's representing
     the hash of the input, and the second element is another string of 0's and 1's for
     additional, optional random variables from the internals of the hash algorithm.
  """

  if hash_mode == 'sha256':
    asint = int(binstr, 2)
    assert HASH_INPUT_NBITS % 8 == 0, 'HASH_INPUT_NBITS must be a multiple of 8'
    asbytes = (asint).to_bytes(HASH_INPUT_NBITS // 8, byteorder='big')
    return nsha256.SHA256(asbytes).get_data()

  elif hash_mode == 'md5':
    num = int2bytes(int(binstr, 2))
    hash_val = hashlib.md5(num).hexdigest()
    binhash = bin(int(hash_val, 16))[2:]
    return ('0' * (256 - len(binhash))) + binhash, ''

  elif hash_mode == 'map_from_input':
    binhash = ''
    for i in range(256):
      input_idx = int(i * HASH_INPUT_NBITS / 256)
      binhash += binstr[input_idx]
    return binhash, ''

  elif hash_mode == 'conditioned_on_input_and_hash':
    mapping = {'00': '1', '01': '0', '10': '1', '11': '0'}
    binhash = binstr[:2]
    for i in range(2, 256):
      input_idx = int(i * HASH_INPUT_NBITS / 256)
      binhash += mapping[binhash[i - 2] + binstr[input_idx]]
    return binhash[::-1], '' # reverse

  elif hash_mode == 'pseudo_hash':
    num = int(binstr, 2)
    A, B, C, D = 0xAC32, 0xFFE1, 0xBF09, 0xBEEF
    a = ((num >> 0 ) & 0xFFFF) ^ A
    b = ((num >> 16) & 0xFFFF) ^ B
    c = ((num >> 32) & 0xFFFF) ^ C
    d = ((num >> 48) & 0xFFFF) ^ D
    a = (a | b)
    b = (b + c) & 0xFFFF
    c = (c ^ d)
    s = bin(a | (b << 16) | (c << 32) | (d << 48))[2:]
    return ('0' * (256 - len(s))) + s, ''

  else:
    raise 'Hash algorithm "{}" is not supported'.format(hash_mode)


if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Hash reversal dataset generator')
  parser.add_argument('--num-samples', type=int, default=20000,
                      help='Number of samples to use in the dataset')
  parser.add_argument('--hash-algo', type=str, default='sha256',
                      choices=['sha256', 'md5', 'map_from_input', 'conditioned_on_input_and_hash', 'pseudo_hash'],
                      help='Choose the hashing algorithm to apply to the input data')
  args = parser.parse_args()
  
  data_file = os.path.join(constants.DATA_DIR, '{}-{}.csv'.format(args.hash_algo, args.num_samples))

  random.seed(0)
  constants.makeDataDirectoryIfNeeded()
  
  if os.path.exists(data_file):
      os.remove(data_file)

  with open(data_file, 'w') as f:
    for n in range(args.num_samples):
      binstr = randomBinaryString(HASH_INPUT_NBITS)
      ground_truth = binstr
      binhash, internals = hashFunc(binstr, args.hash_algo)
      dataset_entry = ','.join([c for c in binhash + ground_truth + internals])
      f.write(dataset_entry + '\n')
  
  print('Generated dataset with %d samples (hash=%s).' % (args.num_samples, args.hash_algo))
