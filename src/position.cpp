
#include <cassert>
#include <algorithm>

#include "position.h"
#include "bitboard.h"
#include "hash.h"
#include "textutils.h"

Piece Position::get(Square sq) const
{
	assert(is_inside(sq));

	const bitboard* board = this->board();
	for (Piece p { WP }; p < PIECE_COUNT; ++p)
	{
		if (is_set(board[p], sq))
			return p;
	}

	return NO_PIECE;
}

void Position::set(Square sq, Piece p)
{
	assert(is_inside(sq));

	bitboard* board = this->board();
	Piece	  onboard { get(sq) };

	// Delete piece
	if (onboard != NO_PIECE)
	{
		board[onboard] -= cell(sq);

		// Find piece
		for (int i { 0 }; i < piececount[onboard]; i++)
		{
			Square* pos = &piecepos[onboard][i];
			if (*pos == sq)
			{
				std::swap(*pos, *(piecepos[onboard] + piececount[onboard] - 1));
				piececount[onboard]--;

				PieceType type { piece_type(onboard) };
				if(type == ROOK || type == QUEEN)
					bigpieces[piece_col(p)]--;

				break;
			}
		}

		m_hash ^= get_hash(sq, onboard);
	}

	// Add piece
	if (p != NO_PIECE)
	{
		board[p] |= cell(sq);
		piecepos[p][piececount[p]] = sq;

		piececount[p]++;
		
		// Check if big piece
		PieceType type { piece_type(p) };
		if(type == ROOK || type == QUEEN)
			bigpieces[piece_col(p)]++;

		m_hash ^= get_hash(sq, p);
	}
}

U64 Position::hash() const
{
	return m_hash ^ get_hash(lsb(enpassantsq) % BOARD_SIZE) ^ get_hash(side) ^ get_hash(rights);
}

constexpr Square POS_WR1 { idx(0, 0) };
constexpr Square POS_WR2 { idx(7, 0) };
constexpr Square POS_BR1 { idx(0, 7) };
constexpr Square POS_BR2 { idx(7, 7) };
constexpr Square POS_WK { idx(4, 0) };
constexpr Square POS_BK { idx(4, 7) };

void Position::make_move(Move move)
{
	// Store for restore
	history[hisply]	   = UndoMove(move);
	UndoMove& undoMove = history[hisply++];
	ply++;

	undoMove.hash = hash();

	undoMove.before_enpassantsq  = enpassantsq;
	undoMove.before_rights	     = rights;
	undoMove.before_fiftyply     = fiftyply;
	undoMove.before_done_castles = done_castles;

	Piece rook { side == WHITE ? WR : BR };
	Piece king { side == WHITE ? WK : BK };

	// May be resetted
	enpassantsq = 0;
	fiftyply++;

	if (move.castling() == NO_CASTLING)
	{
		int   p_dir { side == WHITE ? 1 : -1 };
		Piece pawn { side == WHITE ? WP : BP };

		Piece piece { move.piece() };

		if (piece == rook)
		{
			// Remove permission if rook was moved
			CastlingSide OO { side == WHITE ? W_OO : B_OO };
			CastlingSide OOO { side == WHITE ? W_OOO : B_OOO };

			if (file(move.from()) == 7)
			{
				rights &= ~OO;
			}
			else if (move.from() % BOARD_SIZE == 0)
			{
				rights &= ~OOO;
			}
		}
		else if (piece == pawn)
		{
			// Pawn extra movement
			if (move.isDoublePawnPush())
			{
				enpassantsq = cell(move.to() - p_dir * 8);
			}
			else if (move.isEnPassant())
			{
				set(move.to() - p_dir * 8, NO_PIECE);
			}
			else if (move.promotion() != NO_PIECE)
			{
				piece = move.promotion();
			}

			fiftyply = 0;
		}
		else if (piece == king)
		{
			rights &= ~(side == WHITE ? CS_WHITE : CS_BLACK);
		}

		if (move.capture() != NO_PIECE)
			fiftyply = 0;

		// Remove permission if rook was captured
		if (rank(move.to()) == (side == WHITE ? 7 : 0) && piece_type(move.capture()) == ROOK)
		{
			if (file(move.to()) == 7)
			{
				CastlingSide OO { side == WHITE ? B_OO : W_OO };
				rights &= ~OO;
			}
			else if (file(move.to()) == 0)
			{
				CastlingSide OOO { side == WHITE ? B_OOO : W_OOO };
				rights &= ~OOO;
			}
		}

		set(move.from(), NO_PIECE);
		set(move.to(), piece);
	}
	else
	{
		Square pos_k { side == WHITE ? POS_WK : POS_BK };
		Square pos_r { idx(move.castling() & QUEEN_SIDE ? 0 : 7, side == WHITE ? 0 : 7) };

		int cs_dir { move.castling() & KING_SIDE ? 1 : -1 };

		set(pos_r, NO_PIECE);
		set(pos_k, NO_PIECE);
		set(pos_k + cs_dir * 2, king);
		set(pos_k + cs_dir * 1, rook);

		done_castles |= move.castling();
	}

	side = swap(side);
}

void Position::undo_move()
{
	assert(hisply > 0);

	side = swap(side);
	UndoMove undomove { history[--hisply] };
	ply--;

	Piece rook { side == WHITE ? WR : BR };
	Piece king { side == WHITE ? WK : BK };

	if (undomove.castling() == NO_CASTLING)
	{
		int p_dir { side == WHITE ? 1 : -1 };

		set(undomove.from(), undomove.piece());

		if (undomove.isEnPassant())
		{
			set(undomove.to() - p_dir * 8, undomove.capture());
			set(undomove.to(), NO_PIECE);
		}
		else
			set(undomove.to(), undomove.capture());
	}
	else
	{
		Square pos_k { side == WHITE ? POS_WK : POS_BK };
		Square pos_r { idx(undomove.castling() & QUEEN_SIDE ? 0 : 7, side == WHITE ? 0 : 7) };

		int cs_dir { undomove.castling() & KING_SIDE ? 1 : -1 };

		set(pos_r, rook);
		set(pos_k, king);
		set(pos_k + cs_dir * 2, NO_PIECE);
		set(pos_k + cs_dir * 1, NO_PIECE);
	}

	fiftyply     = undomove.before_fiftyply;
	rights	     = undomove.before_rights;
	enpassantsq  = undomove.before_enpassantsq;
	done_castles = undomove.before_done_castles;

	// If this fails you certainly have forgotten the appropiate undo_move()
	assert(hash() == undomove.hash);
}

void Position::make_null()
{
	history[hisply] = UndoMove();
	UndoMove& undomove = history[hisply];
	undomove.hash = hash();

	ply++;
	hisply++;
	undomove.before_enpassantsq = enpassantsq;
	undomove.before_fiftyply = fiftyply;
	undomove.before_rights = rights;
	undomove.before_done_castles = done_castles;

	enpassantsq = 0ULL;
	side = swap(side);
}

void Position::undo_null()
{
	ply--;
	UndoMove& undomove = history[--hisply];

	enpassantsq = undomove.before_enpassantsq;
	fiftyply = undomove.before_fiftyply;
	rights = undomove.before_rights;
	done_castles = undomove.before_done_castles;
	
	side = swap(side);

	assert(hash() == undomove.hash);
}