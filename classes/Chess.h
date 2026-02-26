#pragma once

#include "Game.h"
#include "Bitboard.h"
#include "Grid.h"
#include <vector>


constexpr int pieceSize = 80;

/*enum ChessPiece
{
    NoPiece,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};
*/

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    bool _whiteToMove = true;
    std::vector<BitMove> _legalMoves;

    void regenerateLegalMoves();
    int holderToIndex(BitHolder& h) const;
    bool isWhiteBit(const Bit& bit) const;
    ChessPiece bitToPiece(const Bit& bit) const;
    bool isOccupied(int idx) const;
    bool isOccupiedByWhite(int idx) const;
    bool isOccupiedByBlack(int idx) const;
    Grid* _grid;
};