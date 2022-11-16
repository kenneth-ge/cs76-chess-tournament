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

#include <unordered_map>

int count_moves(ChessRules &r){
	return r.CountMoves();
}

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

int max_value(ChessRules &cr, int depth, int alpha, int beta, int material, int absmaterial, int max_depth);
int min_value(ChessRules &cr, int depth, int alpha, int beta, int material, int absmaterial, int max_depth);

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
const int MAX_DEPTH = 75; // no longer const because of our mate in X guarantee
const int MAX_DEPTH_NUM = MAX_DEPTH / 10 + 1;
const int DEFAULT_MAX_DEPTH = 45;
int alt_max_depth = 75, alt_min_depth = 0;
int DEFAULT_MATERIAL = 0;

bool we_are_white;

struct checkmate_info {
	bool is_checkmate;
	int mate_depth;
};

checkmate_info checkmate_w(int eval){
	if(eval >= CHECKMATE - MAX_DEPTH_NUM){
		return {true, CHECKMATE - eval};
	}
	return {false, MAX_DEPTH_NUM};
}

checkmate_info checkmate_b(int eval){
	if(eval <= -CHECKMATE + MAX_DEPTH_NUM){
		return {true, CHECKMATE + eval};
	}
	return {false, MAX_DEPTH_NUM};
}

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

bool looked_up_successfully;
Move lookup_table(ChessRules &cr){
	switch(cr.full_move_count){
	case 0:
	case 1:
		looked_up_successfully = true;
		if(cr.white){
			return {e2, e4, SPECIAL_WPAWN_2SQUARES, ' '};
		}else{
			if(cr.squares[d4] != ' '){
				return {d7, d5, SPECIAL_BPAWN_2SQUARES, ' '};
			}
			return {e7, e5, SPECIAL_BPAWN_2SQUARES, ' '};
		}
		break;
	case 2:
		looked_up_successfully = true;
		if(cr.white){
			if(cr.squares[d5] != ' '){
				return {e4, d5, NOT_SPECIAL, 'p'};
			}else if(cr.squares[f5] != ' '){
				return {e4, f5, NOT_SPECIAL, 'p'};
			}
			return {b1, c3, NOT_SPECIAL, ' '};
		}else{
			if(cr.squares[d5] == 'p'){
				if(cr.squares[e4] == 'P'){
					return {d5, e4, NOT_SPECIAL, 'P'};
				}else if(cr.squares[c4] == 'P'){
					return {d5, c4, NOT_SPECIAL, 'P'};
				}
				return {b8, c6, NOT_SPECIAL, ' '};
			}
			//otherwise our pawn is at e5
			if(cr.squares[d4] == 'P'){
				return {e5, d4, NOT_SPECIAL, 'P'};
			}else if(cr.squares[f4] == 'P'){
				return {e5, f4, NOT_SPECIAL, 'P'};
			}
			return {b8, c6, NOT_SPECIAL, ' '};
		}
		break;
	case 3:
		ChessEvaluation eval(cr);

		vector<Move> moves;
		eval.GenLegalMoveListSorted(moves);

		looked_up_successfully = true;

		for(Move m: moves){
			if(m.capture != ' ' && (cr.squares[m.src] == 'p' || cr.squares[m.src] == 'P')){
				return m;
			}
		}

		if(eval.white){
			return {g1, f3, NOT_SPECIAL, ' '};
		}else{
			return {g8, f6, NOT_SPECIAL, ' '};
		}

		break;
	}
	looked_up_successfully = false;
	return Move();
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

MOVELIST countmobility;
bool eval_mid_terminal = false;
DRAWTYPE draw_type;

int evaluate_mid(ChessRules &board, int depth, int material, int absmaterial){
    TERMINAL eval_final_position;
    board.Evaluate( eval_final_position );
    if(eval_final_position != NOT_TERMINAL){
    	eval_mid_terminal = true;
        switch(eval_final_position){
            case TERMINAL_WCHECKMATE:
                return -(CHECKMATE - depth);
            case TERMINAL_BCHECKMATE:
                return CHECKMATE - depth;
            default: //tie; count stalemate as 0 regardless of which player causes it
                return 0;
        }
    }

    if(board.GetRepetitionCount() >= 3 || board.IsInsufficientDraw(true, draw_type) || board.IsInsufficientDraw(false, draw_type)){
    	return 0;
    }

    eval_mid_terminal = false;

    int eval = material;

    //add things for check
    eval -= board.AttackedPiece(board.wking_square) * 500;
    eval += board.AttackedPiece(board.bking_square) * 500;

    int current_player_moves = -1 * player[board.white] * count_moves(board);
    board.white = !board.white;
    int other_player_moves = -1 * player[board.white] * count_moves(board);
    board.white = !board.white;

    eval += (current_player_moves + other_player_moves) * 25;

    squares[0] = board.wking_square; squares[1] = board.bking_square;

    /*if(abs(material_value) <= 2.5 * value['Q']){
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
    }*/

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

    return eval;
}

Move move_history[MAXMOVES];

int evaluate_early(ChessRules &board, int depth, int material, int absmaterial){
	int starting = evaluate_mid(board, depth, material, absmaterial);

	if(eval_mid_terminal)
		return starting;

	int white_castled = 0, black_castled = 0;

	for(int i = depth - 1; i >= 0; i--){
		switch(move_history[i].special){
		case SPECIAL_WK_CASTLING:
		case SPECIAL_WQ_CASTLING:
			white_castled = 1;
			break;
		case SPECIAL_BK_CASTLING:
		case SPECIAL_BQ_CASTLING:
			black_castled = 1;
			break;
		}
	}
	if(board.wking_square != e1 && !white_castled){
		starting -= 0.65 * value['P'];
	}
	starting += white_castled * 450;
	if(board.bking_square != e8 && !black_castled){
		starting += 0.65 * value['P'];
	}
	starting -= black_castled * 450;

	return starting;
}

//TODO: benchmark because it's possible that compiler optimizations with using just a regular function could actually make it faster
//to just have an if-statement
int (*evaluate)(ChessRules &board, int depth, int material, int absmaterial) = evaluate_early;

int max_depth_reached = 0;

bool stop;
//depth is multiplied by 10
int cutoff_test(ChessRules &board, int depth, int max_depth, int material, int absmaterial){
	max_depth_reached = max(max_depth_reached, depth);

	//Past actual max depth
	if(MAX_DEPTH - 10 * depth <= 0){
		stop = true;
		return evaluate(board, depth, material, absmaterial);
	}

	if(alt_max_depth - 10 * depth <= 0){
		stop = true;
		return evaluate(board, depth, material, absmaterial);
	}

    TERMINAL eval_final_position;
    board.Evaluate( eval_final_position );

    if(eval_final_position != NOT_TERMINAL){
    	stop = true;
    	return evaluate(board, depth, material, absmaterial);
    }

    if(board.GetRepetitionCount() >= 3 || board.IsInsufficientDraw(true, draw_type) || board.IsInsufficientDraw(false, draw_type)){
    	stop = true;
    	return 0;
    }

    if(10 * depth < alt_min_depth){
    	stop = false;
    	return 0;
    }

    int remaining = max_depth - 10 * depth;

    if(remaining <= 0){
    	stop = true;
        return evaluate(board, depth, material, absmaterial);
    }else if(remaining < 10){ //0 to 9 -- 5 for half
        int randbool = rand() % 10; //0 to 9
        stop = remaining > randbool;

        if(!stop)
        	return 0;
        else{
        	return evaluate(board, depth, material, absmaterial);
        }
    }

    stop = false;
    return 0;
    //return evaluate(board, depth, material, absmaterial);
}

pair<int, int> priority(Move &m, ChessRules &board, bool check, bool mate, bool stalemate){
	//higher priority/depth for captures and check
	if(mate){
		return {-2000, 10};
		//m.special ==
	}

	if(check){
		return {-1000, 8}; //priority = -100 (very high priority), extra depth of one half
	}

	if(m.capture != ' '){
		//if capture
		return {-800 + (board.white ? 1 : -1) * value[m.capture] / 100, 5};
	}
	return {0, 0};
}

MOVELIST moves;
bool check[MAXMOVES];
bool mate[MAXMOVES];
bool stalemate[MAXMOVES];

priority_move v2[MAX_DEPTH / 10 + 2][MAXMOVES];

void priority_sort(int depth, ChessRules &board){
	for(int i = 0; i < moves.count; i++){
		//play and pop could be expensive -- see whether this is worth it, and if it is, maybe we should only run it in the endgame
		//board.PlayMove(moves[i]);

		auto [pr, depth2] = priority(moves.moves[i], board, check[i], mate[i], stalemate[i]);
		v2[depth][i].m = moves.moves[i];
		v2[depth][i].priority = pr;
		v2[depth][i].depth = depth2;
		//board.PopMove(moves[i]);

	}
	sort(v2[depth], v2[depth] + moves.count);

	/*if(depth <= 1){
		cout << "depth " << depth;
		for(int i = 0; i < moves.count; i++){
			cout << "move " << static_cast<char>(v2[depth][i].m.capture) << " " << v2[depth][i].m.NaturalOut(&board) << endl;
		}
	}*/
}

Move blankmove;
Move lastconsidered;
//vector<bool> check, mate, stalemate;
//vector<Move> moves;
int num_poss_checked;

struct transpos {
	int start_depth;
	int eval;
	Move best_move;
};

#define MAX_NUM_POS 1000000
#define USE_TRANSPOS

unordered_map<string, transpos> transpos_table;
string transpos_q[MAX_NUM_POS];
int q_min = 0, q_max = 0, q_size = 0;

void update_table(string pos, transpos t){
	transpos_table[pos] = t;

	if(q_size >= MAX_NUM_POS){
		transpos_table.erase(transpos_q[q_min]);
		q_min++;
		q_min %= MAX_NUM_POS;
		q_size--;
	}

	transpos_q[q_max] = pos;
	q_max++; q_max %= MAX_NUM_POS;
	q_size++;
}

char playerchar[] = {'b', 'w'};

int max_value(ChessRules &cr, int depth, int alpha, int beta, int material, int absmaterial, int max_depth){
	num_poss_checked += 1;

	int eval = cutoff_test(cr, depth, max_depth, material, absmaterial);

    if(stop){
        return eval;
    }

#ifdef USE_TRANSPOS
    string pos(65, '-');
    for(int i = 0; i < 64; i++){
    	pos[i] = cr.squares[i];
    }
    pos[64] = playerchar[cr.white];

    if(transpos_table.find(pos) != transpos_table.end()){
    	transpos t = transpos_table[pos];

    	if(t.start_depth <= depth){
    		lastconsidered = t.best_move;
    		return t.eval;
    	}
    }
#endif

    //do not use eval after cutoff_test lol
    //moves.clear(); check.clear(); mate.clear(); stalemate.clear();
    cr.GenLegalMoveList(&moves, check, mate, stalemate);
    int num_moves = moves.count;
    priority_sort(depth, cr);

    int alt_max_depth_before = alt_max_depth;
    auto best_eval_value = -INF;
    Move best_move;

    for(int i = 0; i < num_moves; i++){
    	priority_move pm = v2[depth][i];

    	Move x = pm.m;

        //queen promotion
        int piece_chng = value[x.capture];
        if(v2[depth][i].m.special == SPECIAL_PROMOTION_QUEEN){
        	piece_chng += value['q'];
        }

    	cr.PushMove(x);
    	move_history[depth] = x;

    	material -= piece_chng;
    	absmaterial -= abs(piece_chng);
        int curr_eval_value = min_value(cr, depth+1, alpha, beta, material, absmaterial, max_depth + pm.depth);

        auto [is_checkmate, mate_depth] = checkmate_w(curr_eval_value);

        if(is_checkmate){
        	alt_max_depth = mate_depth * 10;
        }

        cr.PopMove(x);

    	material += piece_chng;
    	absmaterial += abs(piece_chng);

        if(curr_eval_value > best_eval_value) {
            best_eval_value = curr_eval_value;
            best_move = x;
        }
        if(best_eval_value >= beta){
        	alt_max_depth = alt_max_depth_before;
        	lastconsidered = best_move;
            return best_eval_value;
        }
        alpha = max(alpha, curr_eval_value);
    }

    alt_max_depth = alt_max_depth_before;

    //best_move == null should not happen
    assert(best_eval_value > -INF);

#ifdef USE_TRANSPOS
    update_table(pos, {depth, best_eval_value, best_move});
#endif

    lastconsidered = best_move;
    return best_eval_value;
}

int min_value(ChessRules &cr, int depth, int alpha, int beta, int material, int absmaterial, int max_depth){
	/*num_poss_checked += 1;

	if(num_poss_checked % 100000 == 0){
		cout << num_poss_checked << endl;
	}*/

    int eval = cutoff_test(cr, depth, max_depth, material, absmaterial);

    if(stop){
        return eval;
    }

#ifdef USE_TRANSPOS
    string pos(65, '-');
	for(int i = 0; i < 64; i++){
		pos[i] = cr.squares[i];
	}
	pos[64] = playerchar[cr.white];

	if(transpos_table.find(pos) != transpos_table.end()){
		transpos t = transpos_table[pos];

		if(t.start_depth <= depth){
			lastconsidered = t.best_move;
			return t.eval;
		}
	}
#endif

    cr.GenLegalMoveList(&moves, check, mate, stalemate);
    int num_moves = moves.count;
    priority_sort(depth, cr);

    auto best_eval_value = INF;
    Move best_move;

    int num_moves_considered = 0;

    int alt_max_depth_before = alt_max_depth;

    for(int i = 0; i < num_moves; i++){
    	priority_move pm = v2[depth][i];
    	Move x = pm.m;

        //queen promotion
        int piece_chng = value[x.capture];
        if(v2[depth][i].m.special == SPECIAL_PROMOTION_QUEEN){
        	piece_chng += value['Q'];
        }

    	cr.PushMove(x);
    	move_history[depth] = x;

    	material -= piece_chng;
    	absmaterial -= abs(piece_chng);
        int curr_eval_value = max_value(cr, depth+1, alpha, beta, material, absmaterial, max_depth + pm.depth);

        auto [is_checkmate, mate_depth] = checkmate_b(curr_eval_value);

        if(is_checkmate){
        	alt_max_depth = mate_depth * 10;
        }

        cr.PopMove(x);

    	material += piece_chng;
    	absmaterial += abs(piece_chng);
        num_moves_considered++;
        if(curr_eval_value < best_eval_value) {
            best_eval_value = curr_eval_value;
            best_move = x;
        }
        if(best_eval_value <= alpha){
        	lastconsidered = best_move;
        	alt_max_depth = alt_max_depth_before;
            return best_eval_value;
        }
        beta = min(beta, curr_eval_value);
    }

    alt_max_depth = alt_max_depth_before;

    assert(best_eval_value < INF);
    //assert(best_move.NaturalOut(&cr) != "--");

#ifdef USE_TRANSPOS
    update_table(pos, {depth, best_eval_value, best_move});
#endif

    lastconsidered = best_move;
    return best_eval_value;
}

int mate_in_x = -1;

thc::Move choose_move(ChessRules &board, int material, int absmaterial, int starting_depth){
    if(board.white){
    	if(mate_in_x != -1){
    		mate_in_x--;
    		alt_max_depth = mate_in_x * 10;
    		alt_min_depth = alt_max_depth;
    	}
        int eval = max_value(board, starting_depth, -INF, INF, material, absmaterial, DEFAULT_MAX_DEPTH);
        if(eval >= CHECKMATE - 5){
        	mate_in_x = CHECKMATE - eval + 1;
        }
        return lastconsidered;
    }else{
    	if(mate_in_x != -1){
    		mate_in_x--;
    		alt_max_depth = mate_in_x * 10;
    		alt_min_depth = alt_max_depth;
    	}

    	int eval = min_value(board, starting_depth, -INF, INF, material, absmaterial, DEFAULT_MAX_DEPTH);
        if(eval <= -(CHECKMATE - 5)){
        	mate_in_x = CHECKMATE + eval + 1;
        }
        return lastconsidered;
    }
}

int main() {
    init_values();

    long total_time = 0;

	ChessPosition cb;

	std::string str;
	std::getline(std::cin, str);
	Move m;
	ChessRules cr;

	if(str == "w"){
		getline(std::cin, str);
		bool works = cb.Forsyth(str.c_str());
		assert(works);
		cr = ChessRules(cb);
	}else if(str == "p"){
		getline(std::cin, str);

		int n = stoi(str);

		for(int i = 0; i < n; i++){
			std::getline(std::cin, str);

			Move m;
			m.TerseIn(&cr, str.c_str());
			cr.PlayMove(m);
		}
	}else{
		std::getline(std::cin, str);

		m.TerseIn(&cr, str.c_str());
		cr.PlayMove(m);
	}

	we_are_white = cr.white;

	//display_position(cr, "Starting Position");

	int absmaterial = 0;
	int material = 0;

	for(int i = 0; i < 64; i++){
		absmaterial += abs(value[cr.squares[i]]);
		material += value[cr.squares[i]];
	}

    while(true){
    	Move best_move;

    	//cout << "full move count " << cr.full_move_count << endl;

    	if(cr.full_move_count <= 3){
    		best_move = lookup_table(cr);

    		//failsafe
    		if(best_move.NaturalOut(&cr) == "--"){
    			//cout << best_move.TerseOut() << endl;
    			//cout << "not valid " << best_move.NaturalOut(&cr) << endl;
        		looked_up_successfully = false;
    		}
    	}else{
    		looked_up_successfully = false;
    	}

    	if(cr.full_move_count > 11){
    		evaluate = evaluate_mid;
    	}

		// searching at smaller depth for the first 5 moves after the lookup table
		int starting_depth = 0;
		if(cr.full_move_count <= 8) {
			starting_depth = 2;
		}

		// int our_remaining_time = cr.white == 1 ? str_list[1] : str_list[2];

		
		/*
		// if remaining time is 1 min or so also we have to reduce the depth
		if(our_remaining_time <= 60) {
			starting_depth = 2;
		}
		*/

    	if(!looked_up_successfully){
			max_depth_reached = 0;

			start_timer();
			best_move = choose_move(cr, material, absmaterial, starting_depth);
			long time = end_timer();
			total_time += time;
    	}

		//cout << "time " << time << endl;
		//cout << "total time " << total_time << endl;

		//cout << "max depth " << max_depth_reached << endl;

		cout << best_move.TerseOut() << endl;
		cout << flush;
		//cout << "natural out " << best_move.NaturalOut(&cr) << endl;
		cr.PlayMove(best_move);

		bool okay = false;
		while(!okay){
			std::getline(std::cin, str);

			okay = m.TerseIn(&cr, str.c_str());

			if(!okay){
				cout << "not okay lol" << endl;
			}
		}
		cr.PlayMove(m);

		absmaterial = 0;
		material = 0;

		for(int i = 0; i < 64; i++){
			absmaterial += abs(value[cr.squares[i]]);
			material += value[cr.squares[i]];
		}
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
