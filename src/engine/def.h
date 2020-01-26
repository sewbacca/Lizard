
#pragma once

#include <cstdint>
#include "misc/enum_operator.h"

#define startpos "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

typedef uint64_t U64;
typedef uint64_t bitboard;

constexpr int MAX_PIECES = 10;
constexpr int BOARD_SIZE = 8;
constexpr int PIECE_TYPES = 12;

enum Piece
{
	WP, WN, WB, WR, WQ, WK,
	BP, BN, BB, BR, BQ, BK,
	NO_PIECE
};

enum Color
{
	WHITE,
	BLACK,
};

enum CastlingSide
{
	KING_SIDE =	0b1010,
	QUEEN_SIDE =	0b0101,
	W_OO =		0b1000,
	W_OOO =		0b0100,
	B_OO =		0b0010,
	B_OOO =		0b0001,
	CS_WHITE =	0b1100,
	CS_BLACK =	0b0011,
	CS_ALL =	0b1111,
	NO_CASTLING =	0
};

struct Point {
	int x = 0, y = 0;
	Point(int x, int y) : x{x}, y{y} { };
	Point(int index) : Point(index % BOARD_SIZE, index / BOARD_SIZE) { }
	Point() { }
};

class Move {
private:
	
	/* Move representation
		0000 0000 0000 0000 0000 0011 1111 -> From
		0000 0000 0000 0000 1111 1100 0000 -> To
		0000 0000 0000 1111 0000 0000 0000 -> Prom
		0000 0000 1111 0000 0000 0000 0000 -> Captured
		0000 Flags:
		0000 0001 0000 0000 0000 0000 0000 -> EP
		0000 0010 0000 0000 0000 0000 0000 -> Double pawn push
		0011 1100 0000 0000 0000 0000 0000 -> Castle
	*/
	int move = 0;
public:
	Point from();
	Point to();
	Piece capture();
	Piece promotion();
	CastlingSide castling();

	bool isDoublePawnPush();
	bool isEnPassant();

	void setFrom(Point);
	void setTo(Point);
	void setCapture(Piece);
	void setPromotion(Piece);
	void setCastling(CastlingSide);

	void flagDoublePawnPush(bool);
	void flagEnPassant(bool);
};

bool is_inside(int x, int y);

// Enum operator

enum_bitwise_operator_rhs(CastlingSide, int, |)
enum_bitwise_operator_rhs(CastlingSide, int, &)
enum_bitwise_operator_rhs(CastlingSide, int, ^)
enum_bitwise_operator_invert(CastlingSide, int, CastlingSide::CS_ALL)

enum_bitwise_operator_lhs(CastlingSide, |=, |)
enum_bitwise_operator_lhs(CastlingSide, &=, &)
enum_bitwise_operator_lhs(CastlingSide, ^=, ^)
