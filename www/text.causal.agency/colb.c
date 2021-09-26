#include <locale.h>
#include <stdio.h>
#include <wchar.h>
int main(void) {
	setlocale(LC_CTYPE, "en_US.UTF-8");
	wint_t next, prev = WEOF;
	while (WEOF != (next = getwchar())) {
		if (next == L'\b') {
			prev = WEOF;
		} else {
			if (prev != WEOF) putwchar(prev);
			prev = next;
		}
	}
	if (prev != WEOF) putwchar(prev);
}
