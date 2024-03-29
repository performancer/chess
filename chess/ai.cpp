#include "ai.h"
#include "generator.h"
#include "movement.h"
#include "clock.h"
#include "transposition.h"
#include <limits>
#include <vector>
#include <algorithm>
#include <map>

#define LIMIT 10

static Clock clock;
static int table_uses = 0;

int minimax(struct State* state, int depth, int alpha, int beta, bool color, bool use_table = false)
{
	transposition::UINT64 hash_key = transposition::hash(state);

	if (use_table && transposition::exists(hash_key))
	{
		Transposition value = transposition::get(hash_key);

		if (value.depth >= depth && value.color == color)
		{
			table_uses++;
			return value.eval;
		}
	}

	if (depth == 0)
	{
		int eval = state->evaluate();
		transposition::add(hash_key, { color, depth, eval });
		return eval;
	}

	std::vector<struct Move> moves = getMoves(state, color);

	int best_eval = color ? INT_MAX : INT_MIN;

	for (Move move : moves)
	{
		if (clock.getSeconds() >= LIMIT)
			return best_eval;

		struct State temp = state->simulate(move);

		int eval = minimax(&temp, depth - 1, alpha, beta, !color, use_table);

		if (color)
		{
			if (eval < best_eval)
				best_eval = eval;

			beta = std::min(beta, eval);
		}
		else
		{
			if (eval > best_eval)
				best_eval = eval;

			alpha = std::max(alpha, eval);
		}

		if (beta <= alpha)
			break;
	}

	if (!transposition::exists(hash_key))
		transposition::add(hash_key, { color, depth, best_eval });

	return best_eval;
}


EvaluatedMove minimaxWithMove(struct State* state, int depth, int alpha, int beta, bool color, bool use_table = false)
{
	std::vector<struct Move> moves = getMoves(state, color);

	EvaluatedMove best_move = { color ? INT_MAX : INT_MIN, -1, -1, -1, -1 };

	for (Move move : moves)
	{
		if (clock.getSeconds() >= LIMIT)
			return { -1, -1, -1, -1, -1 };

		struct State temp = state->simulate(move);

		int eval = minimax(&temp, depth - 1, alpha, beta, !color, use_table);

		if (color)
		{
			if (eval < best_move.eval)
				best_move = { eval, move };

			beta = std::min(beta, eval);
		}
		else
		{
			if(eval > best_move.eval)
				best_move = { eval, move };

			alpha = std::max(alpha, eval);
		}

		if (beta <= alpha)
			break;
	}

	return best_move;
}

EvaluatedMove getNextMove(struct State* state, bool color)
{
	clock.start();
	table_uses = 0;

	transposition::UINT64 hash_key = transposition::hash(state);
	wprintf(L"Hash: %llu\n", hash_key);

	int depth = 4;
	EvaluatedMove move = minimaxWithMove(state, depth, INT_MIN, INT_MAX, color, true);
	
	while (clock.getSeconds() < LIMIT)
	{
		wprintf(L"Depth: %d\n", depth);

		int new_depth = depth + 2;
		wprintf(L"Processing depth %d...", new_depth);
		EvaluatedMove deeper = minimaxWithMove(state, new_depth, INT_MIN, INT_MAX, color, true);

		if (clock.getSeconds() < LIMIT)
		{
			move = deeper;
			depth = new_depth;
			wprintf(L"done.\n");
		}
		else
		{
			wprintf(L"falling back to depth %d.\n", depth);
		}
	}
	
	wprintf(L"Transposition table has been used %d times this turn.\n", table_uses);
	return move;
}
