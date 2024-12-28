#include "TicTac.h"
#include <playsoundapi.h>
#include <time.h>
#include "resource.h"

using namespace D2D1;

#define TIMER_ID 1000
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#define DRAW 2
#define EMPTY -1
#define PLAYER 0
#define COMPUTER 1
#define GotWinner() (winner >= 0)
#define GameDraw() (moves == 9)
#define ToFloat(c) (static_cast<float>(c))

TicTac::TicTac() : ThisInst(NULL), factory(NULL), render(NULL), writefct(NULL),
                   msgfmt(NULL), brush(NULL), Cells{}, ComputerColor(0xFF32A2),
                   PlayerColor(0x0096FF), BoardBGColor(0x22272E), BoardColor(0xFFFFFF),
                   MsgColor(0x660099), window(NULL), moves(0), winner(EMPTY), canplay(1),
                   tries(0), from{}, to{}, level(EASY), Score{}
{
}

TicTac::~TicTac()
{
    SafeRelease(&brush);
    SafeRelease(&msgfmt);
    SafeRelease(&writefct);
    SafeRelease(&render);
    SafeRelease(&factory);
}

LRESULT CALLBACK TicTac::GameProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TicTac *game = (TicTac *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
        game = (TicTac *)pcs->lpCreateParams;
        game->window = hWnd;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pcs->lpCreateParams);
        return TRUE;
    }

    case WM_DESTROY:
    {
        if (game)
            game->window = 0;
        break;
    }

    case WM_RBUTTONUP:
    {
        if (game)
            game->Reset();
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        if (game && BeginPaint(hWnd, &ps))
        {
            game->DrawBoard();
            EndPaint(hWnd, &ps);
        }
        break;
    }

    case WM_LBUTTONUP:
    {
        if (game && game->PlayerTurn(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
        {
            SetTimer(hWnd, TIMER_ID, 500, NULL);
            game->canplay = 0;
        }
        break;
    }

    case WM_TIMER:
    {
        if (game)
        {
            KillTimer(hWnd, TIMER_ID);
            game->canplay = 1;
            game->ComputerTurn();
        }
        break;
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

BOOL TicTac::Initialize(HWND parent)
{
    RECT rect;
    GetClientRect(parent, &rect);
    return Initialize(parent, rect.left, rect.top, rect.right, rect.bottom);
}

BOOL TicTac::Initialize(HWND parent, int x, int y, int cx, int cy)
{
    int width = cx - (cx % BOARD_SIZE); // dummy
    int height = cy - ((cy - 30) % BOARD_SIZE);
    int cellx = width / BOARD_SIZE;
    int celly = (height - 30) / BOARD_SIZE;

    { // Create Window
        WNDCLASSEX wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = GameProc;
        wc.hInstance = ThisInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"D2DTicTacClass";

        if (!RegisterClassEx(&wc))
            return FALSE;

        window = CreateWindow(wc.lpszClassName, NULL, WS_CHILD | WS_VISIBLE,
                              x, y, width, height, parent, (HMENU)10000, wc.hInstance, this);

        ThisInst = wc.hInstance;
        if (!window)
            return FALSE;
    }

    { // Create D2D factory & resources
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);

        if (hr != S_OK)
            return FALSE;

        hr = factory->CreateHwndRenderTarget(RenderTargetProperties(),
                                             HwndRenderTargetProperties(window, SizeU(width, height)),
                                             &render);

        if (hr != S_OK)
            return FALSE;

        render->SetDpi(96.f, 96.f);
        hr = render->CreateSolidColorBrush(ColorF(1.0f, 1.0f, 1.0f), &brush);

        if (hr != S_OK)
            return FALSE;

        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(writefct),
                                 reinterpret_cast<IUnknown **>(&writefct));

        if (hr != S_OK)
            return FALSE;

        hr = writefct->CreateTextFormat(L"Arial", NULL, DWRITE_FONT_WEIGHT_BOLD,
                                        DWRITE_FONT_STYLE_NORMAL,
                                        DWRITE_FONT_STRETCH_NORMAL,
                                        ToFloat(cellx / 8), L"", &msgfmt);

        if (hr != S_OK)
            return FALSE;

        msgfmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        msgfmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    { // Score
        Score[PLAYER].left = 0;
        Score[PLAYER].top = 0;
        Score[PLAYER].right = cellx;
        Score[PLAYER].bottom = 30;
        Score[PLAYER].scr = 0;

        Score[COMPUTER].left = width - cellx;
        Score[COMPUTER].top = 0;
        Score[COMPUTER].right = width;
        Score[COMPUTER].bottom = 30;
        Score[COMPUTER].scr = 0;
    }

    { // Initialize Main Board
        for (int i = 0; i < BOARD_SIZE; i++)
        {
            for (int j = 0; j < BOARD_SIZE; j++)
            {
                Cells[i][j].left = j * cellx;
                Cells[i][j].top = (i * celly) + 30;
                Cells[i][j].right = Cells[i][j].left + cellx;
                Cells[i][j].bottom = Cells[i][j].top + celly;
                Cells[i][j].c = EMPTY;
            }
        }
    }

    return TRUE;
}

VOID TicTac::DrawBoard()
{
    D2D1_RECT_F rect;
    if (!render)
    {
        RECT rc;
        GetClientRect(window, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
        if (factory->CreateHwndRenderTarget(RenderTargetProperties(),
                                            HwndRenderTargetProperties(window, size),
                                            &render) == S_OK)
        {
            render->SetDpi(96.f, 96.f);
            render->CreateSolidColorBrush(ColorF(1.0f, 1.0f, 1.0f), &brush);
        }
    }

    render->BeginDraw();
    render->SetTransform(Matrix3x2F::Identity());
    render->Clear(ColorF(BoardBGColor));

    { // Score
        if (moves % 2)
        {
            brush->SetColor(ColorF(ComputerColor));
            render->DrawLine(Point2F(ToFloat(Score[COMPUTER].left + 5), ToFloat(Score[COMPUTER].bottom + 5)),
                             Point2F(ToFloat(Score[COMPUTER].right + 5), ToFloat(Score[COMPUTER].bottom + 5)),
                             brush, 2.f);
        }
        else
        {
            brush->SetColor(ColorF(PlayerColor));
            render->DrawLine(Point2F(ToFloat(Score[PLAYER].left + 5), ToFloat(Score[PLAYER].bottom + 5)),
                             Point2F(ToFloat(Score[PLAYER].right + 5), ToFloat(Score[PLAYER].bottom + 5)),
                             brush, 2.f);
        }
        brush->SetColor(ColorF(PlayerColor));
        rect = RectF(ToFloat(Score[PLAYER].left), ToFloat(Score[PLAYER].top),
                     ToFloat(Score[PLAYER].right), ToFloat(Score[PLAYER].bottom));
        wchar_t buf[25];
        wsprintf(buf, L"❎                 %d", Score[PLAYER].scr);
        render->DrawText(buf, lstrlen(buf), msgfmt, rect, brush);

        brush->SetColor(ColorF(0xFFFFFF));
        rect = RectF(ToFloat(Score[PLAYER].right), ToFloat(Score[PLAYER].top),
                     ToFloat(Score[COMPUTER].left), ToFloat(Score[COMPUTER].bottom));
        switch (level)
        {
        case EASY:
            lstrcpy(buf, L"Easy");
            break;
        case MEDIUM:
            lstrcpy(buf, L"Medium");
            break;
        case HARD:
            lstrcpy(buf, L"Impossible");
            break;
        }
        render->DrawText(buf, lstrlen(buf), msgfmt, rect, brush);

        brush->SetColor(ColorF(ComputerColor));
        rect = RectF(ToFloat(Score[COMPUTER].left), ToFloat(Score[COMPUTER].top),
                     ToFloat(Score[COMPUTER].right), ToFloat(Score[COMPUTER].bottom));
        wsprintf(buf, L"%d                 🅾️", Score[COMPUTER].scr);
        render->DrawText(buf, lstrlen(buf), msgfmt, rect, brush);
    }

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            brush->SetColor(ColorF(BoardColor));

            render->DrawLine(Point2F(ToFloat(Cells[0][0].right), ToFloat(Cells[0][0].top + 15)),
                             Point2F(ToFloat(Cells[2][0].right), ToFloat(Cells[2][0].bottom - 15)),
                             brush, 6.f);
            render->DrawLine(Point2F(ToFloat(Cells[0][1].right), ToFloat(Cells[0][0].top + 15)),
                             Point2F(ToFloat(Cells[2][1].right), ToFloat(Cells[2][1].bottom - 15)),
                             brush, 6.f);
            render->DrawLine(Point2F(ToFloat(Cells[0][0].left + 15), ToFloat(Cells[0][0].bottom)),
                             Point2F(ToFloat(Cells[0][2].right - 15), ToFloat(Cells[0][2].bottom)),
                             brush, 6.f);
            render->DrawLine(Point2F(ToFloat(Cells[0][0].left + 15), ToFloat(Cells[1][0].bottom)),
                             Point2F(ToFloat(Cells[1][2].right - 15), ToFloat(Cells[1][2].bottom)),
                             brush, 6.f);

            rect = RectF(ToFloat(Cells[i][j].left + 40), ToFloat(Cells[i][j].top + 40),
                         ToFloat(Cells[i][j].right - 40), ToFloat(Cells[i][j].bottom - 40));

            switch (Cells[i][j].c)
            {
            case PLAYER: // X
                brush->SetColor(ColorF(PlayerColor));
                render->DrawLine(Point2F(rect.left, rect.top), Point2F(rect.right, rect.bottom), brush, 22.f);
                render->DrawLine(Point2F(rect.left, rect.bottom), Point2F(rect.right, rect.top), brush, 22.f);
                break;
            case COMPUTER: // O
                brush->SetColor(ColorF(ComputerColor));
                render->DrawRoundedRectangle(RoundedRect(rect, rect.right, rect.right), brush, 22.f);
                break;
            }
        }
    }

    int xx = ((Cells[0][0].right / 2) / 3);
    rect = RectF(ToFloat(Cells[1][1].left - xx - 2), ToFloat(Cells[1][1].top + 49),
                 ToFloat(Cells[1][1].right + xx + 4), ToFloat(Cells[1][1].bottom - 46));

    if (GotWinner())
    {
        brush->SetColor(ColorF(0xA3D055));

        render->DrawLine(Point2F(ToFloat(from.x), ToFloat(from.y)),
                         Point2F(ToFloat(to.x), ToFloat(to.y)), brush, 11.f);

        brush->SetOpacity(0.75f);
        brush->SetColor(ColorF(0xE7D19E));
        render->FillRoundedRectangle(RoundedRect(rect, 15, 15), brush);

        brush->SetOpacity(1.f);
        rect = RectF(ToFloat(Cells[1][1].left - xx), ToFloat(Cells[1][1].top + 50),
                     ToFloat(Cells[1][1].right + xx), ToFloat(Cells[1][1].bottom - 50));

        brush->SetColor(ColorF(MsgColor));
        render->FillRoundedRectangle(RoundedRect(rect, 15, 15), brush);
        brush->SetColor(ColorF(ColorF::AntiqueWhite));
        render->DrawRoundedRectangle(RoundedRect(rect, 15, 15), brush, 2.5f);

        if (winner == COMPUTER)
            render->DrawText(L"🙃 You lose!", 13, msgfmt, rect, brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        else
            render->DrawText(L"😄 You won!", 12, msgfmt, rect, brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
    }
    else if (GameDraw())
    {
        brush->SetOpacity(0.75f);
        brush->SetColor(ColorF(0xE7D19E));
        render->FillRoundedRectangle(RoundedRect(rect, 15, 15), brush);
        brush->SetOpacity(1.f);

        rect = RectF(ToFloat(Cells[1][1].left - xx), ToFloat(Cells[1][1].top + 50),
                     ToFloat(Cells[1][1].right + xx), ToFloat(Cells[1][1].bottom - 50));

        brush->SetColor(ColorF(MsgColor));
        render->FillRoundedRectangle(RoundedRect(rect, 15, 15), brush);
        brush->SetColor(ColorF(ColorF::AntiqueWhite));
        render->DrawRoundedRectangle(RoundedRect(rect, 15, 15), brush, 2.5f);
        render->DrawText(L"🙁 Game draw!", 13, msgfmt, rect, brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
    }

    if (render->EndDraw() == (HRESULT)D2DERR_RECREATE_TARGET)
    {
        SafeRelease(&render);
        SafeRelease(&brush);
    }
    render->EndDraw();
}

VOID TicTac::UpdateFromTo(int i, Direction dir)
{
    int xx = (Cells[0][0].right - Cells[0][0].left) / 2;
    int yy = (Cells[0][0].bottom - Cells[0][0].top) / 2;

    switch (dir)
    {
    case HORIZONTAL:
        from = {Cells[i][0].left + (xx / 3), Cells[i][0].bottom - yy};
        to = {Cells[i][2].right - (xx / 3), Cells[i][2].bottom - yy};
        break;
    case VERTICAL:
        from = {Cells[0][i].left + xx, Cells[0][i].top + (yy / 3)};
        to = {Cells[2][i].left + xx, Cells[2][i].bottom - (yy / 3)};
        break;
    case DIAGONALLEFT:
        from = {Cells[0][0].left + (xx / 3), Cells[0][0].top + (yy / 3)};
        to = {Cells[2][2].right - (xx / 3), Cells[2][2].bottom - (yy / 3)};
        break;
    case DIAGONALRIGHT:
        from = {Cells[2][0].left + (xx / 3), Cells[2][0].bottom - (yy / 3)};
        to = {Cells[0][2].right - (xx / 3), Cells[0][2].top + (yy / 3)};
        break;
    }
}

INT TicTac::CheckWin(BOOL set) // -1: Continue, 0: Player, 1: Computer, 2: Draw
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        if (Cells[i][0].c != EMPTY && Cells[i][0].c == Cells[i][1].c && Cells[i][1].c == Cells[i][2].c)
        {
            if (set)
                UpdateFromTo(i, HORIZONTAL);

            return Cells[i][0].c;
        }
        if (Cells[0][i].c != EMPTY && Cells[0][i].c == Cells[1][i].c && Cells[1][i].c == Cells[2][i].c)
        {
            if (set)
                UpdateFromTo(i, VERTICAL);

            return Cells[0][i].c;
        }
    }
    if (Cells[0][0].c != EMPTY && Cells[0][0].c == Cells[1][1].c && Cells[1][1].c == Cells[2][2].c)
    {
        if (set)
            UpdateFromTo(0, DIAGONALLEFT);

        return Cells[0][0].c;
    }
    if (Cells[0][2].c != EMPTY && Cells[0][2].c == Cells[1][1].c && Cells[1][1].c == Cells[2][0].c)
    {
        if (set)
            UpdateFromTo(0, DIAGONALRIGHT);

        return Cells[0][2].c;
    }

    return (moves == 9) ? DRAW : EMPTY;
}

INT TicTac::MiniMax(Difficulty lvl, int depth, BOOL isMax)
{
    int result = CheckWin(FALSE);
    if (result == COMPUTER)
        return 10;
    if (result == PLAYER)
        return -10;
    if (result == DRAW || (depth == static_cast<int>(lvl)))
        return 0;

    int best;
    if (isMax)
    {
        best = INT_MIN;
        for (int i = 0; i < BOARD_SIZE; i++)
        {
            for (int j = 0; j < BOARD_SIZE; j++)
            {
                if (Cells[i][j].c == EMPTY)
                {
                    Cells[i][j].c = COMPUTER;
                    moves++;
                    int value = MiniMax(lvl, depth + 1, FALSE);
                    Cells[i][j].c = EMPTY;
                    moves--;
                    best = MAX(best, value);
                }
            }
        }
    }
    else
    {
        best = INT_MAX;
        for (int i = 0; i < BOARD_SIZE; i++)
        {
            for (int j = 0; j < BOARD_SIZE; j++)
            {
                if (Cells[i][j].c == EMPTY)
                {
                    Cells[i][j].c = PLAYER;
                    moves++;
                    int value = MiniMax(lvl, depth + 1, TRUE);
                    Cells[i][j].c = EMPTY;
                    moves--;
                    best = MIN(best, value);
                }
            }
        }
    }
    return best;
}

INT TicTac::MiniMax(Difficulty lvl, int depth, BOOL isMax, int alpha, int beta)
{
    int result = CheckWin(FALSE);
    if (result == COMPUTER)
        return 10;
    if (result == PLAYER)
        return -10;
    if (result == DRAW || (depth == static_cast<int>(lvl)))
        return 0;

    int best;
    if (isMax)
    {
        best = INT_MIN;
        for (int i = 0; i < BOARD_SIZE; i++)
        {
            for (int j = 0; j < BOARD_SIZE; j++)
            {
                if (Cells[i][j].c == EMPTY)
                {
                    Cells[i][j].c = COMPUTER;
                    moves++;
                    int value = MiniMax(lvl, depth + 1, FALSE, alpha, beta);
                    Cells[i][j].c = EMPTY;
                    moves--;
                    best = MAX(value, best);
                    alpha = MAX(alpha, best);

                    if (beta <= alpha)
                        break;
                }
            }
        }
    }
    else
    {
        best = INT_MAX;
        for (int i = 0; i < BOARD_SIZE; i++)
        {
            for (int j = 0; j < BOARD_SIZE; j++)
            {
                if (Cells[i][j].c == EMPTY)
                {
                    Cells[i][j].c = PLAYER;
                    moves++;
                    int value = MiniMax(lvl, depth + 1, TRUE, alpha, beta);
                    Cells[i][j].c = EMPTY;
                    moves--;
                    best = MIN(value, best);
                    beta = MIN(beta, best);

                    if (beta <= alpha)
                        break;
                }
            }
        }
    }
    return best;
}

VOID TicTac::ComputerTurn()
{
    if (GotWinner() || GameDraw() || !canplay)
        return;

    PlaySound(MAKEINTRESOURCE(IDR_CLICK2), ThisInst, SND_RESOURCE | SND_ASYNC);

    int bestVal = INT_MIN;
    int row = -1, col = -1;

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (Cells[i][j].c == EMPTY)
            {
                Cells[i][j].c = COMPUTER;
                moves++;
                int bestMove = MiniMax(level, 0, FALSE, INT_MIN, INT_MAX);
                // int bestMove = MiniMax(level, 0, FALSE);
                Cells[i][j].c = EMPTY;
                moves--;

                if (bestMove > bestVal)
                {
                    row = i;
                    col = j;
                    bestVal = bestMove;
                }
            }
        }
    }

    Cells[row][col].c = COMPUTER;
    moves++;

    int result = CheckWin(TRUE);
    switch (result)
    {
    case COMPUTER:
        PlaySound(MAKEINTRESOURCE(IDR_LOSS), ThisInst, SND_RESOURCE | SND_ASYNC);
        winner = COMPUTER;
        Score[COMPUTER].scr++;
        break;
    case DRAW:
        PlaySound(MAKEINTRESOURCE(IDR_DRAW), ThisInst, SND_RESOURCE | SND_ASYNC);
        break;
    }

    RECT rect = {Cells[row][col].left, Cells[row][col].top, Cells[row][col].right, Cells[row][col].bottom};
    InvalidateRect(window, &rect, 0);
}

BOOL TicTac::PlayerTurn(int x, int y)
{
    if (GotWinner() || GameDraw() || !canplay)
        return 0;

    POINT pt = {x, y};
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            RECT rect = {Cells[i][j].left, Cells[i][j].top, Cells[i][j].right, Cells[i][j].bottom};
            if (PtInRect(&rect, pt))
            {
                if (Cells[i][j].c != EMPTY)
                {
                    PlaySound(MAKEINTRESOURCE(IDR_ERROR), ThisInst, SND_RESOURCE | SND_ASYNC);
                    return 0;
                }
                else
                {
                    PlaySound(MAKEINTRESOURCE(IDR_CLICK), ThisInst, SND_RESOURCE | SND_ASYNC);
                    Cells[i][j].c = PLAYER;
                    moves++;

                    int result = CheckWin(TRUE);
                    switch (result)
                    {
                    case PLAYER:
                        PlaySound(MAKEINTRESOURCE(IDR_WIN), ThisInst, SND_RESOURCE | SND_ASYNC);
                        winner = PLAYER;
                        Score[PLAYER].scr++;
                        break;
                    case DRAW:
                        PlaySound(MAKEINTRESOURCE(IDR_DRAW), ThisInst, SND_RESOURCE | SND_ASYNC);
                        break;
                    }

                    InvalidateRect(window, &rect, 0);
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

VOID TicTac::Reset()
{
    canplay = 0;
    PlaySound(MAKEINTRESOURCE(IDR_RESET), ThisInst, SND_RESOURCE | SND_ASYNC);

    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            Cells[i][j].c = EMPTY;

    moves = 0;
    winner = EMPTY;
    tries++;

    if (tries >= 3 && tries < 6)
        level = MEDIUM;
    else if (tries >= 6)
        level = HARD;
    else
        level = EASY;

    InvalidateRect(window, 0, 0);
    canplay = 1;
}

VOID TicTac::ReSize(int cx, int cy)
{
    RECT rect;
    GetClientRect(window, &rect);
    rect.left = (cx / 2) - (rect.right / 2);
    rect.top = (cy / 2) - (rect.bottom / 2);
    rect.left = (rect.left < 0) ? 0 : rect.left;
    rect.top = (rect.top < 0) ? 0 : rect.top;
    SetWindowPos(window, 0, rect.left, rect.top, 0, 0, SWP_NOSIZE);
}