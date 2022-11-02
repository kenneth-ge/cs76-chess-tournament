//============================================================================
// Name        : Chess.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "chess/thc.cpp"

using namespace std;
using namespace thc;

#define cout std::cout
#define endl std::endl

void display_position( thc::ChessRules &cr, const std::string &description )
{
    std::string fen = cr.ForsythPublish();
    std::string s = cr.ToDebugStr();
    printf( "%s\n", description.c_str() );
    printf( "FEN (Forsyth Edwards Notation) = %s\n", fen.c_str() );
    printf( "Position = %s\n", s.c_str() );
}

int main() {
	cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
	ChessPosition cb;
	bool works = cb.Forsyth("1k2r2r/ppp2Q2/3p3b/2nP3p/2PN4/6PB/PP3R1P/1K6 b - - 0 1");
	assert(works);
	ChessRules cr(cb);
    display_position( cr, "Initial position" );
    cout << cr.Evaluate() << endl;

    vector<thc::Move> moves;
    cr.GenLegalMoveList(moves);

    for(auto x: moves){
    	cr.PlayMove(x);
        display_position(cr, "Possible move");
        cr.PopMove(x);
    }

	return 0;
}
