#include "Chess.h"
#include <limits>
#include <cmath>
#include <cctype>
#include <string>
#include <vector>


Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    bit->setGameTag((playerNumber == 0) ? (int)piece : (128 + (int)piece));


    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)

    std::string boardField = fen;
    size_t spacePos = fen.find(' ');
    if (spacePos != std::string::npos) {
        boardField = fen.substr(0, spacePos);
    }

    // CHANGE: clear existing pieces so calling FENtoBoard multiple times works
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });

    int x = 0;
    int y = 0; // FEN starts at rank 8 (top row)

    // CHANGE: helper converts FEN letter -> ChessPiece enum
    auto charToPiece = [](char c) -> ChessPiece {
        switch (std::tolower((unsigned char)c)) {
            case 'p': return Pawn;
            case 'n': return Knight;
            case 'b': return Bishop;
            case 'r': return Rook;
            case 'q': return Queen;
            case 'k': return King;
            default:  return Pawn;
        }
    };

    for (size_t i = 0; i < boardField.size(); i++) {
        char c = boardField[i];

        if (c == '/') {
            y++;
            x = 0;
            continue;
        }

        if (std::isdigit((unsigned char)c)) {
            x += (c - '0');
            continue;
        }

        // piece letter
        if (x < 0 || x >= 8 || y < 0 || y >= 8) {
            continue;
        }

        int playerNumber = std::isupper((unsigned char)c) ? 0 : 1;
        ChessPiece piece = charToPiece(c);

        // CHANGE: flip Y when placing pieces
        // Your engine likely treats (0,0) as the BOTTOM-left, but FEN starts at the TOP (rank 8).
        // So we map FEN row y to board row (7 - y).
        ChessSquare* square = _grid->getSquare(x, y);  // CORRECT (no 7 - y)
        if (square) {
        square->setBit(PieceForPlayer(playerNumber, piece));
    }

        x++;
    }
    regenerateLegalMoves();
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::isWhiteBit(const Bit& bit) const
{
    // white tags: 1..6, black tags: 128+1..6
    return (bit.gameTag() & 128) == 0;
}

ChessPiece Chess::bitToPiece(const Bit& bit) const
{
    int tag = bit.gameTag();
    int piece = tag & 127; // strip black bit
    if (piece < 0 || piece > 6) return NoPiece;
    return (ChessPiece)piece;
}

int Chess::holderToIndex(BitHolder& h) const
{
    int found = -1;
    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        if ((BitHolder*)sq == &h) {
            found = y * 8 + x; // y=0 is top row in your current system
        }
    });
    return found;
}

bool Chess::isOccupied(int idx) const
{
    int x = idx % 8;
    int y = idx / 8;
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return false;
    return _grid->getSquare(x, y)->bit() != nullptr;
}

bool Chess::isOccupiedByWhite(int idx) const
{
    int x = idx % 8;
    int y = idx / 8;
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return false;

    Bit* b = _grid->getSquare(x, y)->bit();
    return b && isWhiteBit(*b);
}

bool Chess::isOccupiedByBlack(int idx) const
{
    int x = idx % 8;
    int y = idx / 8;
    if (x < 0 || x >= 8 || y < 0 || y >= 8) return false;

    Bit* b = _grid->getSquare(x, y)->bit();
    return b && !isWhiteBit(*b);
}

void Chess::regenerateLegalMoves()
{
    _legalMoves.clear();

    _whiteToMove = (getCurrentPlayer()->playerNumber() == 0);

    // Build moves for the side to move by scanning the board
    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        Bit* b = sq->bit();
        if (!b) return;

        bool pieceIsWhite = isWhiteBit(*b);
        if (pieceIsWhite != _whiteToMove) return; // enforce turn

        ChessPiece piece = bitToPiece(*b);
        if (piece != Pawn && piece != Knight && piece != King) return;

        int from = y * 8 + x;

        //  PAWNS 
        if (piece == Pawn)
        {
            // IMPORTANT: your board uses y=0 at the top (because FEN uses y++ as you go down)
            // White pawns start at y=6 and move toward y-1
            // Black pawns start at y=1 and move toward y+1
            int dir = _whiteToMove ? -8 : +8;

            int one = from + dir;
            if (one >= 0 && one < 64)
            {
                // forward move must be empty
                if (!isOccupied(one))
                {
                    _legalMoves.push_back(BitMove(from, one, Pawn));

                    // two forward from starting rank, only if intermediate was empty too
                    bool onStartRank = _whiteToMove ? (y == 6) : (y == 1);
                    if (onStartRank)
                    {
                        int two = from + 2 * dir;
                        if (two >= 0 && two < 64 && !isOccupied(two))
                        {
                            _legalMoves.push_back(BitMove(from, two, Pawn));
                        }
                    }
                }
            }

            // pawn captures: diagonals only (and must be enemy)
            // white (dir=-8): captures to from-9 and from-7
            // black (dir=+8): captures to from+7 and from+9
            int capL = from + dir - 1;
            int capR = from + dir + 1;

            // prevent wrap around files
            bool canCapL = (x > 0);
            bool canCapR = (x < 7);

            if (canCapL && capL >= 0 && capL < 64)
            {
                if (_whiteToMove && isOccupiedByBlack(capL)) _legalMoves.push_back(BitMove(from, capL, Pawn));
                if (!_whiteToMove && isOccupiedByWhite(capL)) _legalMoves.push_back(BitMove(from, capL, Pawn));
            }

            if (canCapR && capR >= 0 && capR < 64)
            {
                if (_whiteToMove && isOccupiedByBlack(capR)) _legalMoves.push_back(BitMove(from, capR, Pawn));
                if (!_whiteToMove && isOccupiedByWhite(capR)) _legalMoves.push_back(BitMove(from, capR, Pawn));
            }
        }

        //  KNIGHTS 
        if (piece == Knight)
        {
            // use (dx,dy) so we never do wrap-around bugs
            const int dx[8] = { 1, 2,  2,  1, -1, -2, -2, -1 };
            const int dy[8] = { 2, 1, -1, -2, -2, -1,  1,  2 };

            for (int i = 0; i < 8; i++)
            {
                int nx = x + dx[i];
                int ny = y + dy[i];
                if (nx < 0 || nx > 7 || ny < 0 || ny > 7) continue;

                int to = ny * 8 + nx;

                // can't land on friendly
                if (_whiteToMove && isOccupiedByWhite(to)) continue;
                if (!_whiteToMove && isOccupiedByBlack(to)) continue;

                _legalMoves.push_back(BitMove(from, to, Knight));
            }
        }

        // KING 
        if (piece == King)
        {
            for (int oy = -1; oy <= 1; oy++)
            {
                for (int ox = -1; ox <= 1; ox++)
                {
                    if (ox == 0 && oy == 0) continue;

                    int nx = x + ox;
                    int ny = y + oy;
                    if (nx < 0 || nx > 7 || ny < 0 || ny > 7) continue;

                    int to = ny * 8 + nx;

                    // can't land on friendly
                    if (_whiteToMove && isOccupiedByWhite(to)) continue;
                    if (!_whiteToMove && isOccupiedByBlack(to)) continue;

                    _legalMoves.push_back(BitMove(from, to, King));
                }
            }
        }
    });
}


bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // use our turn flag, not a gameTag hack
    bool pieceIsWhite = isWhiteBit(bit);
    return pieceIsWhite == _whiteToMove;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessPiece piece = bitToPiece(bit);

    // only pawn/knight/king for this assignment
    if (piece != Pawn && piece != Knight && piece != King) return false;

    int from = holderToIndex(src);
    int to   = holderToIndex(dst);
    if (from < 0 || to < 0) return false;

    // ensure moves list is current
    regenerateLegalMoves();

    for (const BitMove& m : _legalMoves)
    {
        if (m.from == from && m.to == to && m.piece == piece)
        {
            // allow the engine to execute the move (including capture by replacement)
            regenerateLegalMoves();
            return true;
        }
    }

    return false;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });

    regenerateLegalMoves();
}
