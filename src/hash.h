
#pragma once

#include "def.h"

size_t init_hash();

U64 get_hash(Color);
U64 get_hash(Square, Piece);
U64 get_hash(CastlingSide);
U64 get_hash(int enpassantx);