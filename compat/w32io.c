/**
 * Copyright (c)
 * 2022 Max Mruszczak
 * 2018 Yasuhiro Matsumoto
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *
 * VT100 compatibility layer based on `ansicolor-w32.c' by mattn
 * (https://github.com/mattn/ansicolor-w32.c)
 */


#include <windows.h>
#include <io.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

#ifndef FOREGROUND_MASK
# define FOREGROUND_MASK (FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_INTENSITY)
#endif
#ifndef BACKGROUND_MASK
# define BACKGROUND_MASK (BACKGROUND_RED|BACKGROUND_BLUE|BACKGROUND_GREEN|BACKGROUND_INTENSITY)
#endif

#define MAX_LEN 2048

ssize_t
__write_w32(int fd, const char *buf, size_t len) {
	static WORD attr_olds[2] = {-1, -1}, attr_old;
	int type;
	HANDLE handle = INVALID_HANDLE_VALUE;
	WORD attr;
	DWORD written, csize;
	CONSOLE_CURSOR_INFO cci;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD coord;
	const char *ptr = buf;
	unsigned char c;
	int i, n, m, v[6], w, h;

	if (fd == 2) /* stderr */
		type = 1;
	else
		type = 0;

	handle = (HANDLE)_get_osfhandle(fd);
	GetConsoleScreenBufferInfo(handle, &csbi);
	attr = csbi.wAttributes;

	if (attr_olds[type] == (WORD) -1) {
		attr_olds[type] = attr;
	}
	attr_old = attr;

	while (*ptr && len--) {
		if (*ptr != '\033') {
			putchar(*ptr);
			ptr++;
			continue;
		}
		n = 0;
		for (i = 0; i < 6; i++) v[i] = -1;
		ptr++;
retry:
		if ((c = *ptr++) == 0)
			break;
		if (isdigit(c)) {
			if (v[n] == -1) v[n] = c - '0';
			else v[n] = v[n] * 10 + c - '0';
			goto retry;
		}
		if (c == '[') {
			goto retry;
		}
		if (c == ';') {
			if (++n == 6) break;
			goto retry;
		}
		if (c == '>' || c == '?') {
			m = c;
			goto retry;
		}

		switch (c) {
		case 'h':
			if (m == '>' && v[0] == 5) {
				GetConsoleCursorInfo(handle, &cci);
				cci.bVisible = FALSE;
				SetConsoleCursorInfo(handle, &cci);
				break;
			} else if (m != '?')
				break;
			for (i = 0; i <= n; i++) {
				switch (v[i]) {
				case 3:
					GetConsoleScreenBufferInfo(handle, &csbi);
					w = csbi.dwSize.X;
					h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
					csize = w * (h + 1);
					coord.X = 0;
					coord.Y = csbi.srWindow.Top;
					FillConsoleOutputCharacter(handle, ' ', csize, coord, &written);
					FillConsoleOutputAttribute(handle, csbi.wAttributes, csize, coord, &written);
					SetConsoleCursorPosition(handle, csbi.dwCursorPosition);
					csbi.dwSize.X = 132;
					SetConsoleScreenBufferSize(handle, csbi.dwSize);
					csbi.srWindow.Right = csbi.srWindow.Left + 131;
					SetConsoleWindowInfo(handle, TRUE, &csbi.srWindow);
					break;
				case 5:
					attr =
						((attr & FOREGROUND_MASK) << 4) |
						((attr & BACKGROUND_MASK) >> 4);
					SetConsoleTextAttribute(handle, attr);
					break;
				case 9:
					break;
				case 25:
					GetConsoleCursorInfo(handle, &cci);
					cci.bVisible = TRUE;
					SetConsoleCursorInfo(handle, &cci);
					break;
				case 47:
					coord.X = 0;
					coord.Y = 0;
					SetConsoleCursorPosition(handle, coord);
					break;
				default:
					break;
				}
			}
			break;
		case 'l':
			if (m == '>' && v[0] == 5) {
				GetConsoleCursorInfo(handle, &cci);
				cci.bVisible = TRUE;
				SetConsoleCursorInfo(handle, &cci);
			} else if (m != '?')
				break;
			for (i = 0; i <= n; i++) {
				switch (v[i]) {
				case 3:
					GetConsoleScreenBufferInfo(handle, &csbi);
					w = csbi.dwSize.X;
					h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
					csize = w * (h + 1);
					coord.X = 0;
					coord.Y = csbi.srWindow.Top;
					FillConsoleOutputCharacter(handle, ' ', csize, coord, &written);
					FillConsoleOutputAttribute(handle, csbi.wAttributes, csize, coord, &written);
					SetConsoleCursorPosition(handle, csbi.dwCursorPosition);
					csbi.srWindow.Right = csbi.srWindow.Left + 79;
					SetConsoleWindowInfo(handle, TRUE, &csbi.srWindow);
					csbi.dwSize.X = 80;
					SetConsoleScreenBufferSize(handle, csbi.dwSize);
					break;
				case 5:
					attr =
						((attr & FOREGROUND_MASK) << 4) |
						((attr & BACKGROUND_MASK) >> 4);
					SetConsoleTextAttribute(handle, attr);
					break;
				case 25:
					GetConsoleCursorInfo(handle, &cci);
					cci.bVisible = FALSE;
					SetConsoleCursorInfo(handle, &cci);
					break;
				default:
					break;
				}
			}
			break;
		case 'm':
			attr = attr_old;
			for (i = 0; i <= n; i++) {
				if (v[i] == -1 || v[i] == 0)
					attr = attr_olds[type];
				else if (v[i] == 1)
					attr |= FOREGROUND_INTENSITY;
				else if (v[i] == 4)
					attr |= FOREGROUND_INTENSITY;
				else if (v[i] == 5)
					attr |= FOREGROUND_INTENSITY;
				else if (v[i] == 7)
					attr =
						((attr & FOREGROUND_MASK) << 4) |
						((attr & BACKGROUND_MASK) >> 4);
				else if (v[i] == 10)
					; /* symbol on */
				else if (v[i] == 11)
					; /* symbol off */
				else if (v[i] == 22)
					attr &= ~FOREGROUND_INTENSITY;
				else if (v[i] == 24)
					attr &= ~FOREGROUND_INTENSITY;
				else if (v[i] == 25)
					attr &= ~FOREGROUND_INTENSITY;
				else if (v[i] == 27)
					attr =
						((attr & FOREGROUND_MASK) << 4) |
						((attr & BACKGROUND_MASK) >> 4);
				else if (v[i] >= 30 && v[i] <= 37) {
					attr = (attr & BACKGROUND_MASK);
					if ((v[i] - 30) & 1)
						attr |= FOREGROUND_RED;
					if ((v[i] - 30) & 2)
						attr |= FOREGROUND_GREEN;
					if ((v[i] - 30) & 4)
						attr |= FOREGROUND_BLUE;
				}
				/**
				 * what's the deal here?
				 * //else if (v[i] == 39)
				 * //attr = (~attr & BACKGROUND_MASK);
				 */
				else if (v[i] >= 40 && v[i] <= 47) {
					attr = (attr & FOREGROUND_MASK);
					if ((v[i] - 40) & 1)
						attr |= BACKGROUND_RED;
					if ((v[i] - 40) & 2)
						attr |= BACKGROUND_GREEN;
					if ((v[i] - 40) & 4)
						attr |= BACKGROUND_BLUE;
				}
				else if (v[i] >= 90 && v[i] <= 97) {
					attr = (attr & BACKGROUND_MASK) | FOREGROUND_INTENSITY;
					if ((v[i] - 90) & 1)
						attr |= FOREGROUND_RED;
					if ((v[i] - 90) & 2)
						attr |= FOREGROUND_GREEN;
					if ((v[i] - 90) & 4)
						attr |= FOREGROUND_BLUE;
				}
				else if (v[i] >= 100 && v[i] <= 107) {
					attr = (attr & FOREGROUND_MASK) | BACKGROUND_INTENSITY;
					if ((v[i] - 100) & 1)
						attr |= BACKGROUND_RED;
					if ((v[i] - 100) & 2)
						attr |= BACKGROUND_GREEN;
					if ((v[i] - 100) & 4)
						attr |= BACKGROUND_BLUE;
				}
				/**
				 * TODO sort this bg/fg mask thing out
				 * //else if (v[i] == 49)
				 * //attr = (~attr & FOREGROUND_MASK);
				 */
				else if (v[i] == 100)
					attr = attr_old;
			}
			SetConsoleTextAttribute(handle, attr);
			break;
		case 'K':
			GetConsoleScreenBufferInfo(handle, &csbi);
			coord = csbi.dwCursorPosition;
			switch (v[0]) {
				default:
				case 0:
					csize = csbi.dwSize.X - coord.X;
					break;
				case 1:
					csize = coord.X;
					coord.X = 0;
					break;
				case 2:
					csize = csbi.dwSize.X;
					coord.X = 0;
					break;
			}
			FillConsoleOutputCharacter(handle, ' ', csize, coord, &written);
			FillConsoleOutputAttribute(handle, csbi.wAttributes, csize, coord, &written);
			SetConsoleCursorPosition(handle, csbi.dwCursorPosition);
			break;
		case 'J':
			GetConsoleScreenBufferInfo(handle, &csbi);
			w = csbi.dwSize.X;
			h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
			coord = csbi.dwCursorPosition;
			switch (v[0]) {
				default:
				case 0:
					csize = w * (h - coord.Y) - coord.X;
					coord.X = 0;
					break;
				case 1:
					csize = w * coord.Y + coord.X;
					coord.X = 0;
					coord.Y = csbi.srWindow.Top;
					break;
				case 2:
					csize = w * (h + 1);
					coord.X = 0;
					coord.Y = csbi.srWindow.Top;
					break;
			}
			FillConsoleOutputCharacter(handle, ' ', csize, coord, &written);
			FillConsoleOutputAttribute(handle, csbi.wAttributes, csize, coord, &written);
			SetConsoleCursorPosition(handle, csbi.dwCursorPosition);
			break;
		case 'H':
			GetConsoleScreenBufferInfo(handle, &csbi);
			coord = csbi.dwCursorPosition;
			if (v[0] != -1) {
				if (v[1] != -1) {
					coord.Y = csbi.srWindow.Top + v[0] - 1;
					coord.X = v[1] - 1;
				} else
					coord.X = v[0] - 1;
			} else {
				coord.X = 0;
				coord.Y = csbi.srWindow.Top;
			}
			if (coord.X < csbi.srWindow.Left)
				coord.X = csbi.srWindow.Left;
			else if (coord.X > csbi.srWindow.Right)
				coord.X = csbi.srWindow.Right;
			if (coord.Y < csbi.srWindow.Top)
				coord.Y = csbi.srWindow.Top;
			else if (coord.Y > csbi.srWindow.Bottom)
				coord.Y = csbi.srWindow.Bottom;
			SetConsoleCursorPosition(handle, coord);
			break;
		case 'A': /* move up */
			GetConsoleScreenBufferInfo(handle, &csbi);
			coord = csbi.dwCursorPosition;
			if (v[0] != -1) coord.Y = coord.Y - v[0];
			if (coord.Y < csbi.srWindow.Top) coord.Y = csbi.srWindow.Top;
			SetConsoleCursorPosition(handle, coord);
			break;
		case 'B': /* move down */
			GetConsoleScreenBufferInfo(handle, &csbi);
			coord = csbi.dwCursorPosition;
			if (v[0] != -1) coord.Y = coord.Y + v[0];
			if (coord.Y > csbi.srWindow.Bottom) coord.Y = csbi.srWindow.Bottom;
			SetConsoleCursorPosition(handle, coord);
			break;
		case 'C': /* move forward / right */
			GetConsoleScreenBufferInfo(handle, &csbi);
			coord = csbi.dwCursorPosition;
			if (v[0] != -1) coord.X = coord.X + v[0];
			if (coord.X > csbi.srWindow.Right) coord.X = csbi.srWindow.Right;
			SetConsoleCursorPosition(handle, coord);
			break;
		case 'D': /* move backward / left */
			GetConsoleScreenBufferInfo(handle, &csbi);
			coord = csbi.dwCursorPosition;
			if (v[0] != -1) coord.X = coord.X - v[0];
			if (coord.X < csbi.srWindow.Left) coord.X = csbi.srWindow.Left;
			SetConsoleCursorPosition(handle, coord);
			break;
		default:
			break;
		}
	}
	return ptr - buf;
}

int
dprintf(int fd, const char *fmt, ...)
{
	va_list ap;
	int r;
	char buf[MAX_LEN];

	va_start(ap, fmt);
	r = vsnprintf(buf, MAX_LEN, fmt, ap);
	va_end(ap);
	buf[MAX_LEN-1] = '\0';
	if (r > -1)
		r = __write_w32(fd, buf, MAX_LEN);

	return r;
}

int
isatty(int fd)
{
	if (fd == 1 || fd == 2)
		return 1;
	return 0;
}
