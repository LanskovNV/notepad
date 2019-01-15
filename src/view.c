#include <windows.h>
#include "view.h"
#include "model.h"

static int MoveLeft(HWND hwnd, view_t *view, text_t *text)
{
	int curStringLen = 0;
	int decrease = 1;
	LPSTR *buffer = SelectStrings(*text);

	if ((text->pos.y > 0 || text->pos.x > 0))
	{
		do
		{
			if (view->caret.x <= 0 && text->pos.x != 0)
			{
				int iHscrollInc = -1;

				decrease = 0;
				view->iHscrollPos += iHscrollInc;
				ScrollWindow(hwnd, -view->charSize.x * iHscrollInc, 0, NULL, NULL);
				SetScrollPos(hwnd, SB_HORZ, view->iHscrollPos, TRUE);
				text->pos.x--;
			}
			else
			{
				if (text->pos.x == 0 && text->pos.y > 0)
				{
					text->pos.y -= 1;
					curStringLen = strlen(buffer[text->pos.y]);
					text->pos.x = curStringLen - 1;
				}
				else
				{
					text->pos.x--;
				}
			}
		} while (text->pos.x >= 0 && (text->pos.y > 0 || text->pos.x > 0) && IsSpace(buffer[text->pos.y][text->pos.x]));
	}
	return decrease;
}

static int MoveRight(HWND hwnd, view_t *view, text_t *text)
{
	LPSTR *buffer = SelectStrings(*text);
	int curStringLen = strlen(buffer[text->pos.y]);
	int increase = 1;
	int nOfLines = SelectNOfLines(*text);
	int lastLineLen = strlen(buffer[nOfLines - 1]);
	int cxBuffer = max(1, view->client.x / view->charSize.x);
	int cyBuffer = max(1, view->client.y / view->charSize.y);

	if ((text->pos.y < nOfLines || text->pos.x < lastLineLen - 1))
	{
		do
		{
			if (view->caret.x == cxBuffer && text->pos.x != curStringLen)
			{
				int iHscrollInc = 1; 

				increase = 0;
				view->iHscrollPos += iHscrollInc;
				ScrollWindow(hwnd, -view->charSize.x * iHscrollInc, 0, NULL, NULL);
				SetScrollPos(hwnd, SB_HORZ, view->iHscrollPos, TRUE);
				text->pos.x++;
			}
			else
			{
				if (text->pos.x == curStringLen)
				{
					text->pos.y += 1;
					if (text->pos.y < nOfLines)
						curStringLen = strlen(buffer[text->pos.y]);
					if (text->pos.y != nOfLines)
						text->pos.x = 0;
				}
				else
				{
					text->pos.x++;
				}
			}
		} while ((text->pos.y < nOfLines - 1 || text->pos.x < lastLineLen - 1) && IsSpace(buffer[text->pos.y][text->pos.x]));
	}
	return increase;
}

/***************************************
	Processing movements (arrows etc.) */

int UpdateClassPos(HWND hwnd, text_t *text, view_t *view, case_t c)
{
	LPSTR *buffer = SelectStrings(*text);
	pos_t curPos = text->pos;
	int numClStrings = SelectNOfLines(*text);
	int lastLineLen = strlen(buffer[numClStrings - 1]);
	int curStringLen = strlen(buffer[curPos.y]);
	int chX = 1, chY = 1;

	switch (c)
	{
	case right:
		chX = MoveRight(hwnd, view, text);
		break;
	case left:
		chX = MoveLeft(hwnd, view, text);
		break;
	case up:
		if (curPos.y > 0)
		{
			curPos.y -= 1;
			curStringLen = strlen(buffer[curPos.y]);
			if (curPos.x > curStringLen)
				curPos.x = curStringLen - 1;

			if (IsSpace(buffer[curPos.y][curPos.x]))
			{
				MoveLeft(hwnd, view, text);
			}
		}
		break;
	case down:
		if (curPos.y < numClStrings - 1)
		{
			curPos.y += 1;
			curStringLen = strlen(buffer[curPos.y]);
			if (curStringLen <= curPos.x)
				curPos.x = max(0, curStringLen - 1);

			if (IsSpace(buffer[curPos.y][curPos.x]))
			{
				if (IsLastSpaces(buffer[curPos.y] + curPos.x, curStringLen - curPos.x, curStringLen))
					MoveLeft(hwnd, view, text);
				else
					MoveRight(hwnd, view, text);
			}
		}
		break;
	default:
		printf("error in UpdatePos func: incorrect parameter c");
		return 1;
	}

	text->pos = curPos;
	if (chX)
		view->caret.x = text->pos.x;
	if (chY)
		view->caret.y = text->pos.y;

	return 0;
}

/************************************
	Dialog functions (used in main) */

LRESULT CALLBACK DialogProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (wp)
		{
		case 1:
			DestroyWindow(hwnd);
			break;
		case 2:
			PostQuitMessage(0);
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	default:
		return DefWindowProcW(hwnd, msg, wp, lp);
	}
	return DefWindowProcW(hwnd, msg, wp, lp);
}

void RegisterDialogClass(HINSTANCE hInst)
{
	WNDCLASSW dialog = { 0 };

	dialog.hbrBackground = (HBRUSH)COLOR_WINDOW;
	dialog.hCursor = LoadCursor(NULL, IDC_ARROW);  
	dialog.hInstance = hInst;
	dialog.lpszClassName = L"Dialog";
	dialog.lpfnWndProc = DialogProcedure;

	RegisterClassW(&dialog);
}

void DisplayDialog(HWND hwnd)
{
	HWND hDlg = CreateWindowW(L"Dialog", L"Are you really want to exit?", WS_VISIBLE | WS_OVERLAPPEDWINDOW, 750, 450, 300, 120, hwnd, NULL, NULL, NULL);

	CreateWindowW(L"Button", L"no", WS_VISIBLE | WS_CHILD, 160, 20, 100, 40, hDlg, (HMENU)1, NULL, NULL); 
	CreateWindowW(L"Button", L"yes", WS_VISIBLE | WS_CHILD, 20, 20, 100, 40, hDlg, (HMENU)2, NULL, NULL);
}

/********************************
	Windows messages processing */

void CheckMode(HWND hwnd, int *iSelection, HMENU hMenu, WPARAM wParam)
{
	CheckMenuItem(hMenu, *iSelection, MF_UNCHECKED);
	*iSelection = LOWORD(wParam);
	CheckMenuItem(hMenu, *iSelection, MF_CHECKED);
	InvalidateRect(hwnd, NULL, TRUE);
}
///***********************
/// continue here!!!!!!!!!
///***********************
int ResizeMsg(HWND hwnd, LPARAM lParam, text_t *text, view_t *view)
{
	int tmp = 0;
	int nwidth, curnumClStrings;
	RECT rect;

	if (text->mode == classic)
		*iMaxWidth = text->maxWidth;
	else
		*iMaxWidth = *cxClient;

	if (LOWORD(lParam) != 0)
		*cxClient = LOWORD(lParam);
	if (HIWORD(lParam) != 0)
		*cyClient = HIWORD(lParam);
	GetClientRect(hwnd, &rect);
	nwidth = rect.right / cxChar - 1;

	if (text->mode == width && (text->trStrings == NULL || text->curWidth != nwidth))
	{
		BuildtrStrings(text, nwidth);
		curnumClStrings = text->numTrStrings;
	}
	else
		curnumClStrings = text->numClStrings;
	tmp = curnumClStrings + 2 - *cyClient / cyChar;
	*iVscrollMax = max(tmp, 0);
	*iVscrollPos = min(*iVscrollPos, *iVscrollMax);
	SetScrollRange(hwnd, SB_VERT, 0, *iVscrollMax, FALSE);
	SetScrollPos(hwnd, SB_VERT, *iVscrollPos, TRUE);

	if (text->mode != width)
	{
		tmp = 2 + (*iMaxWidth - *cxClient) / cxChar;
		*iHscrollMax = max(0, tmp);
		*iHscrollPos = min(*iHscrollPos, *iHscrollMax);
		SetScrollRange(hwnd, SB_HORZ, 0, *iHscrollMax, FALSE);
		SetScrollPos(hwnd, SB_HORZ, *iHscrollPos, TRUE);
	}

	return 0;
}

int CreateMsg(HWND hwnd, int *cxChar, int *cyChar)
{
	HFONT hfnt;
	TEXTMETRIC tm;
	HDC hdc;

	hfnt = GetStockObject(SYSTEM_FIXED_FONT);
	hdc = GetDC(hwnd);
	SelectObject(hdc, hfnt);
	GetTextMetrics(hdc, &tm);
	* cxChar = tm.tmMaxCharWidth;
	* cyChar = tm.tmHeight + tm.tmExternalLeading;
	ReleaseDC(hwnd, hdc);
	return 0;
}

int PaintMsg(HWND hwnd, text_t *text, int iVscrollPos, int iHscrollPos, int cxChar, int cyChar)
{
	PAINTSTRUCT ps;
	LPSTR *curStrings = NULL;
	int curLen = 0, iPaintBeg, iPaintEnd, x, y, i;
	HDC hdc;

	curStrings = SelectStrings(*text);
	curLen = SelectNOfLines(*text);
	hdc = BeginPaint(hwnd, &ps);
	SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
	iPaintBeg = max(0, iVscrollPos + ps.rcPaint.top / cyChar - 1);
	iPaintEnd = min((int)curLen, iVscrollPos + ps.rcPaint.bottom / cyChar);

	for (i = iPaintBeg; i < iPaintEnd; i++)
	{
		x = cxChar * (-1 * iHscrollPos);
		y = cyChar * (i - iVscrollPos);
		TextOut(
			hdc, x, y,
			curStrings[i],
			strlen(curStrings[i])
		);
		SetTextAlign(hdc, TA_LEFT | TA_TOP);
	}
	EndPaint(hwnd, &ps);
	return 0;
}

int KeydownMsg(HWND hwnd, WPARAM wParam, text_t *text, int *xCaret, int *yCaret, int cxChar, int cyChar, int cxClient, int cyClient, int *iVscrollMax, int *iVscrollPos, int *iHscrollMax, int *iHscrollPos)
{
	int cxBuffer = max(1, cxClient / cxChar);
	int cyBuffer = max(1, cyClient / cyChar);

	switch (wParam)
	{
	case VK_PRIOR:
		SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0L);
		break;
	case VK_NEXT:
		SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
		break;
	case VK_UP:
		UpdateClassPos(hwnd, text, up, xCaret, yCaret, cxBuffer, cyBuffer, iVscrollPos, iVscrollMax, cyChar);
		break;
	case VK_DOWN:
		UpdateClassPos(hwnd, text, down, xCaret, yCaret, cxBuffer, cyBuffer, iVscrollPos, iVscrollMax, cyChar);
		break;
	case VK_LEFT:
		UpdateClassPos(hwnd, text, left, xCaret, yCaret, cxBuffer, cyBuffer, iHscrollPos, iHscrollMax, cxChar);
		break;
	case VK_RIGHT:
		UpdateClassPos(hwnd, text, right, xCaret, yCaret, cxBuffer, cyBuffer, iHscrollPos, iHscrollMax, cxChar);
		break;
	}

	SetCaretPos(*xCaret * cxChar, *yCaret * cyChar);
	return 0;
}

int CommandMsg(HWND hwnd, WPARAM wParam, LPARAM lParam, text_t *text, int *iSelection, int cxChar, int cyChar, int *iMaxWidth, int *cxClient, int *cyClient, int *iVscrollMax, int *iVscrollPos, int *iHscrollMax, int *iHscrollPos, int xCaret, int yCaret)
{
	HMENU hMenu = GetMenu(hwnd);
	int isClassic = 1;

	switch (LOWORD(wParam))
	{
	case IDM_OPEN:
		OpenFileFunc(hwnd, text, *cxClient);
		return 0;
	case IDM_EXIT:
		SendMessage(hwnd, WM_CLOSE, 0, 0L);
		return 0;
	case IDM_WIDTH: // assumes that IDM_WHITE
		text->mode = width;
		isClassic = ResizeMsg(hwnd, lParam, text, iMaxWidth, cxClient, cyClient, iVscrollMax, iVscrollPos, iHscrollMax, iHscrollPos, cxChar, cyChar);
		ClassToWidePos(text);
	case IDM_CLASSIC: // Note: Logic below
		if (isClassic)
		{
			text->mode = classic;
			isClassic = 1;
			WideToClassPos(text);
		}
		SetCaretPos(xCaret * cxChar, yCaret * cyChar);
		CheckMode(hwnd, iSelection, hMenu, wParam);
		return 0;
	}
	return 0;
}

int SetFocusMsg(HWND hwnd, view_t *view)
{
	CreateCaret(hwnd, NULL, view->charSize.x, view->charSize.y);
	SetCaretPos(view->caret.x * view->charSize.x, view->caret.y * view->charSize.y);
	ShowCaret(hwnd);

	return 0;
}

int KillFocusMsg(HWND hwnd)
{
	HideCaret(hwnd);
	DestroyCaret();

	return 0;
}

int VscrollMsg(HWND hwnd, WPARAM wParam, view_t *view)
{
	int iVscrollInc = 0;

	switch (LOWORD(wParam))
	{
	case SB_TOP:
		iVscrollInc = -view->iVscrollPos;
		break;
	case SB_BOTTOM:
		iVscrollInc = view->iVscrollMax - view->iVscrollPos;
		break;
	case SB_LINEUP:
		iVscrollInc = -1;
		break;
	case SB_LINEDOWN:
		iVscrollInc = 1;
		break;
	case SB_PAGEUP:
		iVscrollInc = min(-1, -view->cyClient / view->cyChar);
		break;
	case SB_PAGEDOWN:
		iVscrollInc = max(1, view->cyClient / view->cyChar);
		break;
	case SB_THUMBTRACK:
		iVscrollInc = HIWORD(wParam) - view->iVscrollPos;
		break;
	default:
		iVscrollInc = 0;
	}
	iVscrollInc = max(
		-view->iVscrollPos,
		min(iVscrollInc, view->iVscrollMax - view->iVscrollPos)
	);
	if (iVscrollInc != 0)
	{
		view->iVscrollPos += iVscrollInc;
		ScrollWindow(hwnd, 0, -view->cyChar * iVscrollInc, NULL, NULL);
		SetScrollPos(hwnd, SB_VERT, view->iVscrollPos, TRUE);
		UpdateWindow(hwnd);
	}
	return 0;
}

int HscrollMsg(mode_t mode, WPARAM wParam, HWND hwnd, view_t *view)
{
	int iHscrollInc = 0;

	if (mode != width)
	{
		switch (LOWORD(wParam))
		{
		case SB_LINEUP:
			iHscrollInc = -1;
			break;
		case SB_LINEDOWN:
			iHscrollInc = 1;
			break;
		case SB_THUMBPOSITION:
			iHscrollInc = HIWORD(wParam) - view->iHscrollPos;
			break;
		default:
			iHscrollInc = 0;
		}
		iHscrollInc = max(
			-view->iHscrollPos,
			min(iHscrollInc, view->iHscrollMax - view->iHscrollPos)
		);
		if (iHscrollInc != 0)
		{
			view->iHscrollPos += iHscrollInc;
			ScrollWindow(hwnd, -view->cxChar * iHscrollInc, 0, NULL, NULL);
			SetScrollPos(hwnd, SB_HORZ, view->iHscrollPos, TRUE);
		}
	}
	return 0;
}

