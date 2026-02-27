#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void abort(void) {
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, int *stream) {
    return nmemb;
}

int puts(const char *str) { return -1; }

int vsnprintf(char *s, size_t n, const char *format, va_list arg) {
    return 1;
}

int fprintf(int *stream, const char *format, ...) {
    return 1;
}

int sprintf(char *str, const char *format, ...) {
	return 1;
}

char *fgets(char *restrict s, int n, FILE *restrict stream) {
	return NULL;
}

int errno;


double fabs(double x) {
  return x > 0.0 ? x : -x;
}

int strerror_r(int errnum, char *strerrbuf, size_t  buflen) {
  return 0;
}

int sscanf(const char *str, const char *format, ...) {
  return 1;
}
int open(const char *path, int oflag, ... ) {
  return -1;
}

int read(int fildes, void *buf, size_t nbyte) {
  return -1;
}

clock_t clock(void) {
  return 1;
}

int rand(void) {
	return 0;
}

int remove(const char *filename) {
	return -1;
}

char *tmpnam(char *str) {
	return NULL;
}

// Copied from musl

double scalbn(double x, int n)
{
	union {double f; uint64_t i;} u;
	double_t y = x;

	if (n > 1023) {
		y *= 0x1p1023;
		n -= 1023;
		if (n > 1023) {
			y *= 0x1p1023;
			n -= 1023;
			if (n > 1023)
				n = 1023;
		}
	} else if (n < -1022) {
		/* make sure final n < -53 to avoid double
		   rounding in the subnormal range */
		y *= 0x1p-1022 * 0x1p53;
		n += 1022 - 53;
		if (n < -1022) {
			y *= 0x1p-1022 * 0x1p53;
			n += 1022 - 53;
			if (n < -1022)
				n = -1022;
		}
	}
	u.i = (uint64_t)(0x3ff+n)<<52;
	x = y * u.f;
	return x;
}

double frexp(double x, int *e)
{
	union { double d; uint64_t i; } y = { x };
	int ee = y.i>>52 & 0x7ff;

	if (!ee) {
		if (x) {
			x = frexp(x*0x1p64, e);
			*e -= 64;
		} else *e = 0;
		return x;
	} else if (ee == 0x7ff) {
		return x;
	}

	*e = ee - 0x3fe;
	y.i &= 0x800fffffffffffffull;
	y.i |= 0x3fe0000000000000ull;
	return y.d;
}

double ldexp(double x, int n)
{
	return scalbn(x, n);
}

int abs(int a)
{
	return a>0 ? a : -a;
}


#define SS (sizeof(size_t))
#define ALIGN (sizeof(size_t)-1)
#define ONES ((size_t)-1/UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX/2+1))
#define HASZERO(x) ((x)-ONES & ~(x) & HIGHS)


void *memchr(const void *src, int c, size_t n)
{
	const unsigned char *s = src;
	c = (unsigned char)c;
	for (; ((uintptr_t)s & ALIGN) && n && *s != c; s++, n--);
	if (n && *s != c) {
		const size_t *w;
		size_t k = ONES * c;
		for (w = (const void *)s; n>=SS && !HASZERO(*w^k); w++, n-=SS);
		for (s = (const void *)w; n && *s != c; s++, n--);
	}
	return n ? (void *)s : 0;
}


static char *twobyte_strstr(const unsigned char *h, const unsigned char *n)
{
	uint16_t nw = n[0]<<8 | n[1], hw = h[0]<<8 | h[1];
	for (h++; *h && hw != nw; hw = hw<<8 | *++h);
	return *h ? (char *)h-1 : 0;
}

static char *threebyte_strstr(const unsigned char *h, const unsigned char *n)
{
	uint32_t nw = (uint32_t)n[0]<<24 | n[1]<<16 | n[2]<<8;
	uint32_t hw = (uint32_t)h[0]<<24 | h[1]<<16 | h[2]<<8;
	for (h+=2; *h && hw != nw; hw = (hw|*++h)<<8);
	return *h ? (char *)h-2 : 0;
}

static char *fourbyte_strstr(const unsigned char *h, const unsigned char *n)
{
	uint32_t nw = (uint32_t)n[0]<<24 | n[1]<<16 | n[2]<<8 | n[3];
	uint32_t hw = (uint32_t)h[0]<<24 | h[1]<<16 | h[2]<<8 | h[3];
	for (h+=3; *h && hw != nw; hw = hw<<8 | *++h);
	return *h ? (char *)h-3 : 0;
}

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define BITOP(a,b,op) \
 ((a)[(size_t)(b)/(8*sizeof *(a))] op (size_t)1<<((size_t)(b)%(8*sizeof *(a))))

static char *twoway_strstr(const unsigned char *h, const unsigned char *n)
{
	const unsigned char *z;
	size_t l, ip, jp, k, p, ms, p0, mem, mem0;
	size_t byteset[32 / sizeof(size_t)] = { 0 };
	size_t shift[256];

	/* Computing length of needle and fill shift table */
	for (l=0; n[l] && h[l]; l++)
		BITOP(byteset, n[l], |=), shift[n[l]] = l+1;
	if (n[l]) return 0; /* hit the end of h */

	/* Compute maximal suffix */
	ip = -1; jp = 0; k = p = 1;
	while (jp+k<l) {
		if (n[ip+k] == n[jp+k]) {
			if (k == p) {
				jp += p;
				k = 1;
			} else k++;
		} else if (n[ip+k] > n[jp+k]) {
			jp += k;
			k = 1;
			p = jp - ip;
		} else {
			ip = jp++;
			k = p = 1;
		}
	}
	ms = ip;
	p0 = p;

	/* And with the opposite comparison */
	ip = -1; jp = 0; k = p = 1;
	while (jp+k<l) {
		if (n[ip+k] == n[jp+k]) {
			if (k == p) {
				jp += p;
				k = 1;
			} else k++;
		} else if (n[ip+k] < n[jp+k]) {
			jp += k;
			k = 1;
			p = jp - ip;
		} else {
			ip = jp++;
			k = p = 1;
		}
	}
	if (ip+1 > ms+1) ms = ip;
	else p = p0;

	/* Periodic needle? */
	if (memcmp(n, n+p, ms+1)) {
		mem0 = 0;
		p = MAX(ms, l-ms-1) + 1;
	} else mem0 = l-p;
	mem = 0;

	/* Initialize incremental end-of-haystack pointer */
	z = h;

	/* Search loop */
	for (;;) {
		/* Update incremental end-of-haystack pointer */
		if (z-h < l) {
			/* Fast estimate for MAX(l,63) */
			size_t grow = l | 63;
			const unsigned char *z2 = memchr(z, 0, grow);
			if (z2) {
				z = z2;
				if (z-h < l) return 0;
			} else z += grow;
		}

		/* Check last byte first; advance by shift on mismatch */
		if (BITOP(byteset, h[l-1], &)) {
			k = l-shift[h[l-1]];
			if (k) {
				if (k < mem) k = mem;
				h += k;
				mem = 0;
				continue;
			}
		} else {
			h += l;
			mem = 0;
			continue;
		}

		/* Compare right half */
		for (k=MAX(ms+1,mem); n[k] && n[k] == h[k]; k++);
		if (n[k]) {
			h += k-ms;
			mem = 0;
			continue;
		}
		/* Compare left half */
		for (k=ms+1; k>mem && n[k-1] == h[k-1]; k--);
		if (k <= mem) return (char *)h;
		h += p;
		mem = mem0;
	}
}

char *strstr(const char *h, const char *n)
{
	/* Return immediately on empty needle */
	if (!n[0]) return (char *)h;

	/* Use faster algorithms for short needles */
	h = strchr(h, *n);
	if (!h || !n[1]) return (char *)h;
	if (!h[1]) return 0;
	if (!n[2]) return twobyte_strstr((void *)h, (void *)n);
	if (!h[2]) return 0;
	if (!n[3]) return threebyte_strstr((void *)h, (void *)n);
	if (!h[3]) return 0;
	if (!n[4]) return fourbyte_strstr((void *)h, (void *)n);

	return twoway_strstr((void *)h, (void *)n);
}

int fclose(FILE *f) { return -1; }

int printf(const char *restrict fmt, ...) { return -1; }
long ftell(FILE *f) { return -1; }
int fseek(FILE *f, long off, int whence) { return -1; }
size_t fread(void *restrict destv, size_t size, size_t nmemb, FILE *restrict f) { return -1; }
int feof(FILE *f) { return -1; }
FILE *fopen(const char *restrict filename, const char *restrict mode) { return NULL; }

int isupper(int c)
{
	return (unsigned)c-'A' < 26;
}

int tolower(int c)
{
	if (isupper(c)) return c | 32;
	return c;
}

char *__stpncpy(char *restrict d, const char *restrict s, size_t n)
{
	size_t *wd;
	const size_t *ws;

	if (((uintptr_t)s & ALIGN) == ((uintptr_t)d & ALIGN)) {
		for (; ((uintptr_t)s & ALIGN) && n && (*d=*s); n--, s++, d++);
		if (!n || !*s) goto tail;
		wd=(void *)d; ws=(const void *)s;
		for (; n>=sizeof(size_t) && !HASZERO(*ws);
		       n-=sizeof(size_t), ws++, wd++) *wd = *ws;
		d=(void *)wd; s=(const void *)ws;
	}
	for (; n && (*d=*s); n--, s++, d++);
tail:
	memset(d, 0, n);
	return d;
}


char *strncpy(char *restrict d, const char *restrict s, size_t n)
{
	__stpncpy(d, s, n);
	return d;
}

size_t wcslen(const wchar_t *s)
{
	const wchar_t *a;
	for (a=s; *s; s++);
	return s-a;
}

char *__stpcpy(char *restrict d, const char *restrict s)
{
#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	word *wd;
	const word *ws;
	if ((uintptr_t)s % ALIGN == (uintptr_t)d % ALIGN) {
		for (; (uintptr_t)s % ALIGN; s++, d++)
			if (!(*d=*s)) return d;
		wd=(void *)d; ws=(const void *)s;
		for (; !HASZERO(*ws); *wd++ = *ws++);
		d=(void *)wd; s=(const void *)ws;
	}
#endif
	for (; (*d=*s); s++, d++);

	return d;
}


char *strcpy(char *restrict dest, const char *restrict src)
{
	__stpcpy(dest, src);
	return dest;
}

void *__memrchr(const void *m, int c, size_t n)
{
	const unsigned char *s = m;
	c = (unsigned char)c;
	while (n--) if (s[n]==c) return (void *)(s+n);
	return 0;
}

char *strrchr(const char *s, int c)
{
	return __memrchr(s, c, strlen(s) + 1);
}

char *__strchrnul(const char *s, int c)
{
	c = (unsigned char)c;
	if (!c) return (char *)s + strlen(s);

#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	const word *w;
	for (; (uintptr_t)s % ALIGN; s++)
		if (!*s || *(unsigned char *)s == c) return (char *)s;
	size_t k = ONES * c;
	for (w = (void *)s; !HASZERO(*w) && !HASZERO(*w^k); w++);
	s = (void *)w;
#endif
	for (; *s && *(unsigned char *)s != c; s++);
	return (char *)s;
}

char *strchr(const char *s, int c)
{
	char *r = __strchrnul(s, c);
	return *(unsigned char *)r == (unsigned char)c ? r : 0;
}

size_t strlen(const char *s)
{
	const char *a = s;
	for (; *s; s++);
	return s - a;
}

int strcmp(const char *l, const char *r)
{
	for (; *l == *r && *l; l++, r++);
	return *(unsigned char *)l - *(unsigned char *)r;
}

int strncmp(const char *_l, const char *_r, size_t n)
{
	const unsigned char *l = (const unsigned char *)_l;
	const unsigned char *r = (const unsigned char *)_r;
	if (!n--) return 0;
	for (; *l && *r && n && *l == *r; l++, r++, n--);
	return *l - *r;
}