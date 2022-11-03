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

pair<Move*, int> max_value(ChessRules &cr, int depth, int alpha, int beta);
pair<Move*, int> min_value(ChessRules &cr, int depth, int alpha, int beta);

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
const int MAX_DEPTH = 55;

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

int evaluate(ChessRules &board, int depth){
    TERMINAL eval_final_position;
    board.Evaluate( eval_final_position );
    if(eval_final_position != NOT_TERMINAL){
        switch(eval_final_position){
            case TERMINAL_WCHECKMATE:
                return CHECKMATE - depth;
            case TERMINAL_BCHECKMATE:
                return -(CHECKMATE - depth);
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

//depth is multiplied by 10
pair<bool, int> cutoff_test(ChessRules &board, int depth){
    int remaining = MAX_DEPTH - 10 * depth;

    if(remaining <= 0){
        return {true, evaluate(board, depth)};
    }else if(remaining < 10){ //0 to 9 -- 5 for half
        bool randbool = rand() % 10; //0 to 9
        return {remaining > randbool, evaluate(board, depth)};
    }

    TERMINAL eval_final_position;
    bool legal2 = board.Evaluate( eval_final_position );

    return {eval_final_position != NOT_TERMINAL, 0};
}

pair<Move*, int> max_value(ChessRules &cr, int depth, int alpha, int beta){
    auto [stop, eval] = cutoff_test(cr, depth);
    
    if(stop){
        return {nullptr, eval};
    }
    
    //do not use eval after cutoff_test lol

    vector<thc::Move> moves;
    cr.GenLegalMoveList(moves);

    auto best_eval_value = -INF;
    Move *best_move = nullptr;

    for(auto x: moves){
    	cr.PlayMove(x);
        auto [move, curr_eval_value] = min_value(cr, depth+1, alpha, beta);
        cr.PopMove(x);
        if(curr_eval_value > best_eval_value) {
            best_eval_value = curr_eval_value;
            best_move = &x;
        }
        if(best_eval_value >= beta){
            return {best_move, best_eval_value};
        }
        alpha = max(alpha, curr_eval_value);
    }

    //best_move == null should not happen
    assert(best_move != nullptr);
    if(best_move == nullptr){
        return {nullptr, evaluate(cr, depth)};
    }
}

pair<Move*, int> min_value(ChessRules &cr, int depth, int alpha, int beta){
    auto [stop, eval] = cutoff_test(cr, depth);

    if(stop){
        return {nullptr, eval};
    }

    vector<thc::Move> moves;
    cr.GenLegalMoveList(moves);

    auto best_eval_value = INF;
    Move *best_move = nullptr;

    int num_moves_considered = 0;

    for(auto x: moves){
    	cr.PlayMove(x);
        auto [move, curr_eval_value] = max_value(cr, depth+1, alpha, beta);
        cr.PopMove(x);
        num_moves_considered++;
        if(curr_eval_value < best_eval_value) {
            best_eval_value = curr_eval_value;
            best_move = &x;
        }
        if(best_eval_value <= alpha){
            return {best_move, best_eval_value};
        }
        beta = min(beta, curr_eval_value);
    }

    assert(best_move != nullptr);
    if(best_move == nullptr){
        return {nullptr, evaluate(cr, depth)};
    }
}

thc::Move choose_move(ChessRules &board){
    if(board.white){
        auto [move, eval] = max_value(board, 0, -INF, INF);
        return *move;
    }else{
        auto [move, eval] = min_value(board, 0, -INF, INF);
        return *move;
    }
}

int main() {
    init_values();
    
	ChessPosition cb;
	bool works = cb.Forsyth("1k2r2r/ppp2Q2/3p3b/2nP3p/2PN4/6PB/PP3R1P/1K6 b - - 0 1");
	assert(works);
	ChessRules cr(cb);

    Move best_move = choose_move(cr);

    cout << best_move.TerseOut() << endl;

    cr.PlayMove(best_move);
    display_position(cr, "Best Move");

	return 0;
}
