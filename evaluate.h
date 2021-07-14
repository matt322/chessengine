#include "position.h"
#include "types.h"

const int values[15] = {100, 320, 330, 500, 900, 20000, 0, 0, -100, -320, -330, -500, -900, -20000, 0};

const int PawnTable[64] = {  0,  0,  0,  0,  0,  0,  0,  0,
                        50, 50, 50, 50, 50, 50, 50, 50,
                        10, 10, 20, 30, 30, 20, 10, 10,
                        5,  5, 10, 25, 25, 10,  5,  5,
                        0,  0,  0, 20, 20,  0,  0,  0,
                        5, -5,-10,  0,  0,-10, -5,  5,
                        5, 10, 10,-20,-20, 10, 10,  5,
                        0,  0,  0,  0,  0,  0,  0,  0 };

const int KnightTable[64] = {-50,-40,-30,-30,-30,-30,-40,-50,
                            -40,-20,  0,  0,  0,  0,-20,-40,
                            -30,  0, 10, 15, 15, 10,  0,-30,
                            -30,  5, 15, 20, 20, 15,  5,-30,
                            -30,  0, 15, 20, 20, 15,  0,-30,
                            -30,  5, 10, 15, 15, 10,  5,-30,
                            -40,-20,  0,  5,  5,  0,-20,-40,
                            -50,-40,-30,-30,-30,-30,-40,-50};

const int BishopTable[64] = {   -20,-10,-10,-10,-10,-10,-10,-20,
                                -10,  0,  0,  0,  0,  0,  0,-10,
                                -10,  0,  5, 10, 10,  5,  0,-10,
                                -10,  5,  5, 10, 10,  5,  5,-10,
                                -10,  0, 10, 10, 10, 10,  0,-10,
                                -10, 10, 10, 10, 10, 10, 10,-10,
                                -10,  5,  0,  0,  0,  0,  5,-10,
                                -20,-10,-10,-10,-10,-10,-10,-20};

const int RookTable[64] = {0,  0,  0,  0,  0,  0,  0,  0,
                            5, 10, 10, 10, 10, 10, 10,  5,
                            -5,  0,  0,  0,  0,  0,  0, -5,
                            -5,  0,  0,  0,  0,  0,  0, -5,
                            -5,  0,  0,  0,  0,  0,  0, -5,
                            -5,  0,  0,  0,  0,  0,  0, -5,
                            -5,  0,  0,  0,  0,  0,  0, -5,
                            0,  0,  0,  5,  5,  0,  0,  0};

const int QueenTable[64] = {-20,-10,-10, -5, -5,-10,-10,-20,
                            -10,  0,  0,  0,  0,  0,  0,-10,
                            -10,  0,  5,  5,  5,  5,  0,-10,
                            -5,  0,  5,  5,  5,  5,  0, -5,
                            0,  0,  5,  5,  5,  5,  0, -5,
                            -10,  5,  5,  5,  5,  5,  0,-10,
                            -10,  0,  5,  0,  0,  0,  0,-10,
                            -20,-10,-10, -5, -5,-10,-10,-20};

const int KingMiddleTable[64] = {-30,-40,-40,-50,-50,-40,-40,-30,
                                -30,-40,-40,-50,-50,-40,-40,-30,
                                -30,-40,-40,-50,-50,-40,-40,-30,
                                -30,-40,-40,-50,-50,-40,-40,-30,
                                -20,-30,-30,-40,-40,-30,-30,-20,
                                -10,-20,-20,-20,-20,-20,-20,-10,
                                20, 20,  0,  0,  0,  0, 20, 20,
                                20, 30, 10,  0,  0, 10, 30, 20};

const int KingEndgameTable[64] = {-50,-40,-30,-20,-20,-30,-40,-50,
                                -30,-20,-10,  0,  0,-10,-20,-30,
                                -30,-10, 20, 30, 30, 20,-10,-30,
                                -30,-10, 30, 40, 40, 30,-10,-30,
                                -30,-10, 30, 40, 40, 30,-10,-30,
                                -30,-10, 20, 30, 30, 20,-10,-30,
                                -30,-30,  0,  0,  0,  0,-30,-30,
                                -50,-30,-30,-30,-30,-30,-30,-50};

int reflect(int n){
    return (8*(7-floor(n/8))+n%8);
}

template <Color Us>
float evaluate(Position p){
    float e;

    int mult = Us == WHITE ? 1 : -1;
    MoveList<Us> list(p);
    if(list.size() == 0){
        if (p.in_check<Us>()){
            return (-1000000);
        }
        return 0;
    }
    MoveList<~Us> them(p);
    float m = (float)list.size() - (float)them.size();
    e = m*5*mult;
    int wkingsquare, bkingsquare;
    int square = 0;
    int pieces = 0;
    for (Piece piece : p.board){
        e += values[piece];
        pieces++;
        switch (piece){
            case NO_PIECE:
                pieces--;
                break;
            case WHITE_PAWN:
                e += PawnTable[reflect(square)];
                break;
            case BLACK_PAWN:
                e -= PawnTable[square];
                break;
            case WHITE_ROOK:
                e += RookTable[reflect(square)];
                break;
            case BLACK_ROOK:
                e -= RookTable[square];
                break;
            case WHITE_KNIGHT:
                e += KnightTable[reflect(square)];
                break;
            case BLACK_KNIGHT:
                e -= KnightTable[square];
                break;
            case WHITE_BISHOP:
                e += BishopTable[reflect(square)];
                break;
            case BLACK_BISHOP:
                e -= BishopTable[square];
                break;
            case WHITE_QUEEN:
                e += QueenTable[reflect(square)];
                break;
            case BLACK_QUEEN:
                e -= QueenTable[square];
                break;
            case WHITE_KING:
                wkingsquare = square;
                break;
            case BLACK_KING:
                bkingsquare = square;
                break;
            
            
        }

        square++;
    }
    if (pieces < 14){ // make linear weight
        e += KingEndgameTable[reflect(wkingsquare)];
        e -= KingEndgameTable[bkingsquare];
    }
    else {
        e += KingMiddleTable[reflect(wkingsquare)];
        e -= KingMiddleTable[bkingsquare];
    }
    return(e*mult);
}

float moveValue(Move m, Position p){
	return (abs(values[p.board[m.to()]]) - abs(values[p.board[m.from()]]));
    
}


