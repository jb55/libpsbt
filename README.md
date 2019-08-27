
# libpsbt

[![Build Status](https://travis-ci.org/jb55/libpsbt.svg)](https://travis-ci.org/jb55/libpsbt)

  C library for Partially Signed Bitcoin Transactions (bip174)

  This is a SAX-style parser/serializer. It allocates no memory during its
  operation.

## Progress

- [x] Serialization
- [x] Deserialization
- [ ] Validation
- [ ] Merging

## Installation

    $ make install PREFIX=out_dir

## Example

See [test.c](test.c)

