#include "Chess.h"
#include <limits>
#include <cmath>
#include <cctype>
#include <string>

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
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    return true;
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
}
