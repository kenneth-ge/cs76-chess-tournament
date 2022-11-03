//============================================================================
// Name        : Chess.cpp
// Author      :
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "chess/thc.cpp"
#include <climits>
#include <cassert>

using namespace std;
using namespace thc;

pair<Move, int> max_value(ChessRules &cr, int depth, int alpha, int beta, int max_depth);
pair<Move, int> min_value(ChessRules &cr, int depth, int alpha, int beta, int max_depth);

void display_position( thc::ChessRules &cr, const std::string &description )
{
    std::string fen = cr.ForsythPublish();
    std::string s = cr.ToDebugStr();
    printf( "%s\n", description.c_str() );
    printf( "FEN (Forsyth Edwards Notation) = %s\n", fen.c_str() );
    printf( "Position = %s\n", s.c_str() );
}

#define cout std::cout
#define endl std::endl

#define INF (INT_MAX / 5)
#define CHECKMATE (INF / 2)

// change this later on
const int MAX_DEPTH = 75;
const int DEFAULT_MAX_DEPTH = 55;

int value[128];

void init_values(){
    value['P'] = 100;
    value['p'] = -100;
    value['Q'] = 800;
    value['q'] = -800;
    value['R'] = 500;
    value['r'] = -500;
    value['B'] = 300;
    value['b'] = -300;
    value['N'] = 300;
    value['n'] = -300;
    value['K'] = 1000000;
    value['k'] = -1000000;
}

struct priority_move {
	Move m;
	int priority;
	int depth;
	bool operator <(const priority_move& another) const {
	   return priority < another.priority;
	}
};

int evaluate(ChessRules &board, int depth){
    TERMINAL eval_final_position;
    board.Evaluate( eval_final_position );
    if(eval_final_position != NOT_TERMINAL){
        switch(eval_final_position){
            case TERMINAL_WCHECKMATE:
                return -(CHECKMATE - depth);
            case TERMINAL_BCHECKMATE:
                return CHECKMATE - depth;
            default: //tie; count stalemate as 0 regardless of which player causes it
                return 0;
        }
    }

    int eval = 0;

    for(int i = 0; i < 64; i++){
        eval += value[board.squares[i]];
    }

    return eval;
}

int max_depth_reached = 0;

//depth is multiplied by 10
pair<bool, int> cutoff_test(ChessRules &board, int depth, int max_depth){
	max_depth_reached = max(max_depth_reached, depth);

	//Past actual max depth
	if(MAX_DEPTH - 10 * depth <= 0){
		return {true, evaluate(board, depth)};
	}

    TERMINAL eval_final_position;
    board.Evaluate( eval_final_position );

    if(eval_final_position != NOT_TERMINAL){
    	return {true, evaluate(board, depth)};
    }

    int remaining = max_depth - 10 * depth;

    if(remaining <= 0){
        return {true, evaluate(board, depth)};
    }else if(remaining < 10){ //0 to 9 -- 5 for half
        bool randbool = rand() % 10; //0 to 9
        return {remaining > randbool, evaluate(board, depth)};
    }

    return {eval_final_position != NOT_TERMINAL, evaluate(board, depth)};
}

pair<int, int> priority(Move &m, ChessRules &board, bool check, bool mate, bool stalemate){
	//higher priority/depth for captures and check
	if(mate){
		return {-200, 5};
	}

	if(check){
		return {-100, 5}; //priority = -100 (very high priority), extra depth of one half
	}
	if(m.capture != ' '){
		//if capture
		return {-80, 1};
	}
	return {0, 0};
}

void priority_sort(vector<Move> &moves, vector<priority_move> &sorted_moves, ChessRules &board, vector<bool> &check, vector<bool> &mate, vector<bool> &stalemate){
	sorted_moves.resize(moves.size());
	for(int i = 0; i < moves.size(); i++){
		//play and pop could be expensive -- see whether this is worth it, and if it is, maybe we should only run it in the endgame
		board.PlayMove(moves[i]);
		auto [pr, depth] = priority(moves[i], board, check[i], mate[i], stalemate[i]);
		sorted_moves[i] = {moves[i], pr, depth};
		board.PopMove(moves[i]);
	}
	sort(sorted_moves.begin(), sorted_moves.end());
}

pair<Move, int> max_value(ChessRules &cr, int depth, int alpha, int beta, int max_depth){
    auto [stop, eval] = cutoff_test(cr, depth, max_depth);

    if(stop){
        return {Move(), eval};
    }

    //do not use eval after cutoff_test lol

    vector<thc::Move> moves; std::vector<bool> check, mate, stalemate;
    cr.GenLegalMoveList(moves, check, mate, stalemate);
    vector<priority_move> v2;
    priority_sort(moves, v2, cr, check, mate, stalemate);

    auto best_eval_value = -INF;
    Move best_move;

    for(auto pm: v2){
    	Move x = pm.m;
    	cr.PlayMove(x);
        auto [move, curr_eval_value] = min_value(cr, depth+1, alpha, beta, max_depth + pm.depth);
        cr.PopMove(x);
        if(curr_eval_value > best_eval_value) {
            best_eval_value = curr_eval_value;
            best_move = x;
        }
        if(best_eval_value >= beta){
            return {best_move, best_eval_value};
        }
        alpha = max(alpha, curr_eval_value);
    }

    //best_move == null should not happen
    assert(best_eval_value > -INF);

    return {best_move, best_eval_value};
}

pair<Move, int> min_value(ChessRules &cr, int depth, int alpha, int beta, int max_depth){
    auto [stop, eval] = cutoff_test(cr, depth, max_depth);

    if(stop){
        return {Move(), eval};
    }

    vector<thc::Move> moves; std::vector<bool> check, mate, stalemate;
    cr.GenLegalMoveList(moves, check, mate, stalemate);
    vector<priority_move> v2;
    priority_sort(moves, v2, cr, check, mate, stalemate);

    auto best_eval_value = INF;
    Move best_move;

    int num_moves_considered = 0;

    for(auto pm: v2){
    	Move x = pm.m;
    	cr.PlayMove(x);
        pair<Move, int> p = max_value(cr, depth+1, alpha, beta, max_depth + pm.depth);
        Move move = p.first; int curr_eval_value = p.second;
        cr.PopMove(x);
        num_moves_considered++;
        if(curr_eval_value < best_eval_value) {
            best_eval_value = curr_eval_value;
            best_move = x;
        }
        if(best_eval_value <= alpha){
            return {best_move, best_eval_value};
        }
        beta = min(beta, curr_eval_value);
    }

    assert(best_eval_value < INF);
    //assert(best_move.NaturalOut(&cr) != "--");

    return {best_move, best_eval_value};

}

thc::Move choose_move(ChessRules &board){
    if(board.white){
        auto [move, eval] = max_value(board, 0, -INF, INF, DEFAULT_MAX_DEPTH);
        return move;
    }else{
        auto [move, eval] = min_value(board, 0, -INF, INF, DEFAULT_MAX_DEPTH);
        return move;
    }
}

int main() {
    init_values();

	ChessPosition cb;
	bool works = cb.Forsyth("3k4/6p1/3K1pPp/3PrP2/p7/P7/3Q3P/1q6 w - - 0 2");
	assert(works);
	ChessRules cr(cb);

	display_position(cr, "Starting position");

	while(true){
		Move best_move = choose_move(cr);

		if(best_move.NaturalOut(&cr) == "--"){
			break;
		}

		cout << "terse out " << best_move.TerseOut() << endl;
		cout << "natural out " << best_move.NaturalOut(&cr) << endl;

		cr.PlayMove(best_move);
		display_position(cr, "Next Move");
	}

	cout << "GAME OVER" << endl;

	cout << "max depth reached " << max_depth_reached << endl;

	return 0;
}
