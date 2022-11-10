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

#include <sys/time.h>
#include <iostream>

#include <queue>

struct timeval tp;
long int current_time_millis(){
	gettimeofday(&tp, NULL);
	return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

inline long int last_time = 0;

void start_timer(){
	last_time = current_time_millis();
}

long end_timer(){
	using namespace std;

	long int current_time = current_time_millis();

	return current_time - last_time;
}

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
int color[128];

void init_values(){
    value['P'] = 1000;
    value['p'] = -1000;
    value['Q'] = 8000;
    value['q'] = -8000;
    value['R'] = 5000;
    value['r'] = -5000;
    value['B'] = 3000;
    value['b'] = -3000;
    value['N'] = 3000;
    value['n'] = -3000;
    value['K'] = 10000000;
    value['k'] = -10000000;

    color['P'] = 1;
    color['p'] = -1;
    color['Q'] = 1;
    color['q'] = -1;
    color['R'] = 1;
    color['r'] = -1;
    color['B'] = 1;
    color['b'] = -1;
    color['N'] = 1;
    color['n'] = -1;
    color['K'] = 1;
    color['k'] = -1;
    color[' '] = 0;
}

struct priority_move {
	Move m;
	int priority;
	int depth;
	bool operator <(const priority_move& another) const {
	   return priority < another.priority;
	}
};

bool out_of_bounds(int x, int y){
	return x < 0 || y < 0 || x >= 8 || y >= 8;
}

int convert_to_num(int x, int y){
	return x * 8 + y;
}

//-1 if black, 1 if white, 0 if none
int black_or_white(char c){
	return color[c];
}

bool attacker[2] = {false, true};
int player[2] = {1, -1};

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
    int material_value = 0;

    for(int i = 0; i < 64; i++){
        eval += value[board.squares[i]];
        material_value += abs(value[board.squares[i]]);
    }

    //add things for check
    eval -= board.AttackedPiece(board.wking_square) * 500;
    eval += board.AttackedPiece(board.bking_square) * 500;

    int squares[2] = {board.wking_square, board.bking_square};

    if(abs(material_value) <= value['Q'] * 1.5){
    	//endgame, count number of free squares the king has to move
    	int dir[][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    	bool visited[64];

    	for(int i = 0; i < 2; i++){
    		fill(visited, visited + 64, false);
			int free_squares = 1;

			queue<pair<int, int>> q;

			q.push({squares[i] / 8, squares[i] % 8});
			visited[squares[i]] = true;

			while(!q.empty()){
				auto [r, c] = q.front();
				q.pop();
				for(int i = 0; i < 4; i++){
					int nextR = r + dir[i][0];
					int nextC = c + dir[i][1];

					if(out_of_bounds(nextR, nextC))
						continue;

					int num = convert_to_num(nextR, nextC);

					if(board.AttackedSquare(static_cast<Square>(num), attacker[i]))
						continue;

					if(visited[num])
						continue;

					visited[num] = true;
					free_squares += 1;
					q.push({nextR, nextC});
				}
			}

			eval += free_squares * 15 * player[i];
    	}
    }

    if(abs(material_value) >= value['Q']){
        //king protection
        //white king first
        for(int k = 0; k < 2; k++){
    		int loc[8][2];

    		for(int i = 0; i < 8; i++){
    			loc[i][0] = squares[k] / 8;
    			loc[i][1] = squares[k] % 8;
    		}

    		int dir[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, +1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

    		//for each direction
    		for(int i = 0; i < 8; i++){
    			//move eight squares or until we hit a wall or get out of bounds
    			for(int j = 0; j < 8; j++){
    				loc[i][0] += dir[i][0];
    				loc[i][1] += dir[i][1];

    				if(out_of_bounds(loc[i][0], loc[i][1])){
    					eval += -1 * black_or_white(squares[k]) * 4 * 5;
    					break;
    				}

    				int val = black_or_white(board.squares[convert_to_num(loc[i][0], loc[i][1])]);

    				if(val != 0){
    					eval += val * (8 - j) * 5;
    					break;
    				}
    			}
    		}
        }
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

    long total_time = 0;

    while(true){
		std::string str;
		std::getline(std::cin, str);

		ChessPosition cb;
		bool works = cb.Forsyth(str.c_str());
		assert(works);
		ChessRules cr(cb);

		display_position(cr, "Starting position");

		max_depth_reached = 0;

		start_timer();
		Move best_move = choose_move(cr);
		long time = end_timer();
		total_time += time;

		cout << "time " << time << endl;
		cout << "total time " << total_time << endl;

		cout << "max depth " << max_depth_reached << endl;

		cout << "terse out " << best_move.TerseOut() << endl;
		cout << "natural out " << best_move.NaturalOut(&cr) << endl;
    }

	//cr.PlayMove(best_move);
	//display_position(cr, "Next Move");

	/*
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

	cout << "max depth reached " << max_depth_reached << endl;*/

	return 0;
}
