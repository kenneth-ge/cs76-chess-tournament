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

int max_value(ChessRules &cr, int depth, int alpha, int beta, int &material, int &absmaterial, int max_depth);
int min_value(ChessRules &cr, int depth, int alpha, int beta, int &material, int &absmaterial, int max_depth);

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
int DEFAULT_MATERIAL = 0;

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

    DEFAULT_MATERIAL = 2 * (8 * value['P'] + 2 * value['R'] + 2 * value['N'] + 2 * value['B'] + value['Q']);

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
int squares[2];
int dir[][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
int dir2[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, +1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
int loc[8][2];

int evaluate(ChessRules &board, int depth, int &material, int &absmaterial){
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

    int eval = material;
    int material_value = absmaterial;

    /*for(int i = 0; i < 64; i++){
        eval += value[board.squares[i]];
        material_value += abs(value[board.squares[i]]);
    }*/

    //add things for check
    eval -= board.AttackedPiece(board.wking_square) * 500;
    eval += board.AttackedPiece(board.bking_square) * 500;

    squares[0] = board.wking_square; squares[1] = board.bking_square;

    if(abs(material_value) <= 2.5 * value['Q']){
    	//endgame, count number of free squares the king has to move
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

					if(board.squares[num] != ' ')
						continue;

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

    if(abs(material_value) >= 2 * value['Q']){
        //king protection
        //white king first
        for(int k = 0; k < 2; k++){
    		for(int i = 0; i < 8; i++){
    			loc[i][0] = squares[k] / 8;
    			loc[i][1] = squares[k] % 8;
    		}

    		//for each direction
    		for(int i = 0; i < 8; i++){
    			//move eight squares or until we hit a wall or get out of bounds
    			for(int j = 0; j < 8; j++){
    				loc[i][0] += dir2[i][0];
    				loc[i][1] += dir2[i][1];

    				if(out_of_bounds(loc[i][0], loc[i][1])){
    					eval += -player[k] * 4 * 5;
    					break;
    				}

    				int val = black_or_white(board.squares[convert_to_num(loc[i][0], loc[i][1])]);

    				if(val != 0){
    					if(player[k] == val){
    						eval += player[k] * (8 - j) * 5;
    					}else{
    						eval -= player[k] * j * 5;
    					}

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
pair<bool, int> cutoff_test(ChessRules &board, int depth, int max_depth, int &material, int &absmaterial){
	max_depth_reached = max(max_depth_reached, depth);

	//Past actual max depth
	if(MAX_DEPTH - 10 * depth <= 0){
		return {true, evaluate(board, depth, material, absmaterial)};
	}

    TERMINAL eval_final_position;
    board.Evaluate( eval_final_position );

    if(eval_final_position != NOT_TERMINAL){
    	return {true, evaluate(board, depth, material, absmaterial)};
    }

    int remaining = max_depth - 10 * depth;

    if(remaining <= 0){
        return {true, evaluate(board, depth, material, absmaterial)};
    }else if(remaining < 10){ //0 to 9 -- 5 for half
        bool randbool = rand() % 10; //0 to 9
        return {remaining > randbool, evaluate(board, depth, material, absmaterial)};
    }

    return {eval_final_position != NOT_TERMINAL, evaluate(board, depth, material, absmaterial)};
}

pair<int, int> priority(Move &m, ChessRules &board, bool check, bool mate, bool stalemate){
	//higher priority/depth for captures and check
	if(mate){
		return {-2000, 5};
	}

	if(check){
		return {-1000, 5}; //priority = -100 (very high priority), extra depth of one half
	}

	if(m.capture != ' '){
		//if capture
		return {-800 + (board.white ? 1 : -1) * value[m.capture] / 100, 15};
	}
	return {0, 0};
}

MOVELIST moves;
bool check[MAXMOVES];
bool mate[MAXMOVES];
bool stalemate[MAXMOVES];

void priority_sort(vector<priority_move> &sorted_moves, ChessRules &board){
	sorted_moves.resize(moves.count);
	for(int i = 0; i < moves.count; i++){
		//play and pop could be expensive -- see whether this is worth it, and if it is, maybe we should only run it in the endgame
		//board.PlayMove(moves[i]);
		auto [pr, depth] = priority(moves.moves[i], board, check[i], mate[i], stalemate[i]);
		sorted_moves[i] = {moves.moves[i], pr, depth};
		//board.PopMove(moves[i]);
	}
	sort(sorted_moves.begin(), sorted_moves.end());
}

Move blankmove;
Move lastconsidered;
//vector<bool> check, mate, stalemate;
//vector<Move> moves;

int max_value(ChessRules &cr, int depth, int alpha, int beta, int &material, int &absmaterial, int max_depth){
	auto [stop, eval] = cutoff_test(cr, depth, max_depth, material, absmaterial);

    if(stop){
        return eval;
    }

    //do not use eval after cutoff_test lol
    //moves.clear(); check.clear(); mate.clear(); stalemate.clear();
    cr.GenLegalMoveList(&moves, check, mate, stalemate);
    vector<priority_move> v2;
    priority_sort(v2, cr);

    auto best_eval_value = -INF;
    Move best_move;

    for(auto pm: v2){
    	Move x = pm.m;
    	cr.PlayMove(x);
    	material -= value[x.capture];
    	absmaterial -= abs(value[x.capture]);
        int curr_eval_value = min_value(cr, depth+1, alpha, beta, material, absmaterial, max_depth + pm.depth);
    	material += value[x.capture];
    	absmaterial += abs(value[x.capture]);
        cr.PopMove(x);
        if(curr_eval_value > best_eval_value) {
            best_eval_value = curr_eval_value;
            best_move = x;
        }
        if(best_eval_value >= beta){
        	lastconsidered = best_move;
            return best_eval_value;
        }
        alpha = max(alpha, curr_eval_value);
    }

    //best_move == null should not happen
    assert(best_eval_value > -INF);

    lastconsidered = best_move;
    return best_eval_value;
}

int min_value(ChessRules &cr, int depth, int alpha, int beta, int &material, int &absmaterial, int max_depth){
    auto [stop, eval] = cutoff_test(cr, depth, max_depth, material, absmaterial);

    if(stop){
        return eval;
    }

    vector<priority_move> v2;
    cr.GenLegalMoveList(&moves, check, mate, stalemate);
    priority_sort(v2, cr);

    auto best_eval_value = INF;
    Move best_move;

    int num_moves_considered = 0;

    for(auto pm: v2){
    	Move x = pm.m;
    	cr.PlayMove(x);
    	material -= value[x.capture];
    	absmaterial -= abs(value[x.capture]);
        int curr_eval_value = max_value(cr, depth+1, alpha, beta, material, absmaterial, max_depth + pm.depth);
    	material += value[x.capture];
    	absmaterial += abs(value[x.capture]);
        cr.PopMove(x);
        num_moves_considered++;
        if(curr_eval_value < best_eval_value) {
            best_eval_value = curr_eval_value;
            best_move = x;
        }
        if(best_eval_value <= alpha){
        	lastconsidered = best_move;
            return best_eval_value;
        }
        beta = min(beta, curr_eval_value);
    }

    assert(best_eval_value < INF);
    //assert(best_move.NaturalOut(&cr) != "--");

    lastconsidered = best_move;
    return best_eval_value;
}

thc::Move choose_move(ChessRules &board){
	int material = 0;
	int absmaterial = DEFAULT_MATERIAL;
    if(board.white){
        max_value(board, 0, -INF, INF, material, absmaterial, DEFAULT_MAX_DEPTH);
        return lastconsidered;
    }else{
    	min_value(board, 0, -INF, INF, material, absmaterial, DEFAULT_MAX_DEPTH);
        return lastconsidered;
    }
}

int main() {
    init_values();

    long total_time = 0;

	ChessPosition cb;
	std::string str;
	std::getline(std::cin, str);
	bool works = cb.Forsyth(str.c_str());
	assert(works);
	ChessRules cr(cb);
	Move m;

    while(true){

		//display_position(cr, "Starting position");

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
		cr.PlayMove(best_move);

		std::getline(std::cin, str);

		m.TerseIn(&cr, str.c_str());
		cr.PlayMove(m);
    }

	//cr.PlayMove(best_move);
	//display_position(cr, "Next Move");


	/*while(true){
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
