#pragma once

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define _WIN32_WINNT _WIN32_WINNT_WIN10

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

template <class Interface>
inline void SafeRelease(Interface **ppInterface)
{
    if (*ppInterface != NULL)
    {
        (*ppInterface)->Release();
        (*ppInterface) = NULL;
    }
};

#define BOARD_SIZE 3

class TicTac
{
private:
    HINSTANCE ThisInst;
    ID2D1Factory *factory;
    ID2D1HwndRenderTarget *render;
    IDWriteFactory *writefct;
    IDWriteTextFormat *msgfmt;
    ID2D1SolidColorBrush *brush;

    struct cell
    {
        int left;
        int top;
        int right;
        int bottom;
        int c;
    } Cells[BOARD_SIZE][BOARD_SIZE];

    COLORREF ComputerColor, PlayerColor, BoardBGColor, BoardColor, MsgColor;
    HWND window;
    INT moves, winner, canplay, tries;
    POINT from, to;

    enum Difficulty
    {
        EASY = 1,
        MEDIUM = 4,
        HARD = 7
    } level;

    enum Direction
    {
        HORIZONTAL,
        VERTICAL,
        DIAGONALLEFT,
        DIAGONALRIGHT
    } direction;

    struct score
    {
        int left;
        int top;
        int right;
        int bottom;
        int scr;
    } Score[2];

    VOID UpdateFromTo(int, Direction);
    int CheckWin(BOOL);
    VOID DrawBoard();
    INT MiniMax(Difficulty, int, BOOL);
    INT MiniMax(Difficulty, int, BOOL, int, int);
    VOID ComputerTurn();
    BOOL PlayerTurn(int, int);
    static LRESULT CALLBACK GameProc(HWND, UINT, WPARAM, LPARAM);

public:
    TicTac();
    ~TicTac();
    BOOL Initialize(HWND);
    BOOL Initialize(HWND parent, int x, int y, int cx, int cy);
    VOID Reset();
    VOID ReSize(int cx, int cy);
};
