#ifndef LOGGING_H
#define LOGGING_H

#include <complex>
#include <fstream>
#include <Windows.h>

struct results_log;

bool make_log_dir(const char *path);
struct results_log *get_results_log(const char *id);
void log_results(struct results_log *files, void *expect,
                 void *actual, size_t elem_sz, size_t block_len);
void close_results_log(struct results_log *files);
bool open_log(FILE **log);

static inline void wsa_error(const char *msg)
{
	fprintf(stderr, "%s [%u]\n", msg, WSAGetLastError());
}

constexpr WORD DEF_ROW_COLOR =
	FOREGROUND_BLUE
	| FOREGROUND_GREEN
	| FOREGROUND_RED
	| FOREGROUND_INTENSITY;

constexpr WORD COLORED_ROW =
	DEF_ROW_COLOR
	| BACKGROUND_GREEN
	| BACKGROUND_BLUE;

template <typename T_> struct DPT_FMT {
	static const char *head;
	static const char *data;
};

const char *DPT_FMT<float>::head = "%-13s";
const char *DPT_FMT<float>::data = "%13.5g";

const char *DPT_FMT<double>::head = "%-13s";
const char *DPT_FMT<double>::data = "%13.5g";

const char *DPT_FMT<int>::head = "%-12s";
const char *DPT_FMT<int>::data = "%12d";

const char *DPT_FMT< std::complex<float> >::head = "%-30s";
const char *DPT_FMT< std::complex<float> >::data = "%13.5g + %13.5gi";

const char *DPT_FMT< std::complex<double> >::head = "%-30s";
const char *DPT_FMT< std::complex<double> >::data = "%13.5g + %13.5gi";

static char log_fmt[128];

template <typename T_>
static void log_header(FILE *log)
{
	sprintf_s(log_fmt, "\t| %%-7s |  %s  |  %s  |  %%s\n"
	                   "\t| %%-7s |  %s  |  %s  |\n",
	          DPT_FMT<T_>::head, DPT_FMT<T_>::head,
	          DPT_FMT<T_>::head, DPT_FMT<T_>::head);
	fprintf(log, log_fmt,
	        "Index", "Expected", "Actual", "Error",
	        "     ", "        ", "      ");
}

template <typename T_>
static void log_diff(FILE *log, unsigned i, T_ p1, T_ p2, double diff)
{
	sprintf_s(log_fmt, "\t| %% 7u |  %s  |  %s  |  %%-12g\n",
	          DPT_FMT<T_>::data, DPT_FMT<T_>::data);
	fprintf(log, log_fmt, i, p1, p2, diff);
}

template <typename T_>
static void log_diff(FILE *log, unsigned i, std::complex<T_> p1,
                     std::complex<T_> p2, double diff)
{
	sprintf_s(log_fmt, "\t| %% 7u |  %s  |  %s  |  %%-12g\n",
	          DPT_FMT< std::complex<T_> >::data,
	          DPT_FMT< std::complex<T_> >::data);
	fprintf(log, log_fmt, i, p1.real(), p1.imag(),
	                         p2.real(), p2.imag(), diff);
}

template <typename T_>
static void log_diff_color(FILE *log, unsigned i, T_ p1, T_ p2, double diff)
{
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(console, COLORED_ROW);
	log_diff(log, i, p1, p2, diff);
	SetConsoleTextAttribute(console, DEF_ROW_COLOR);
}

template <typename T_>
static void log_result(FILE *log, unsigned i, T_ p)
{
	sprintf_s(log_fmt, "%% 7u | %s\n", DPT_FMT<T_>::data);
	fprintf(log, log_fmt, i, p);
}

template <typename T_>
static void log_result(FILE *log, unsigned i, std::complex<T_> p)
{
	sprintf_s(log_fmt, "%% 7u | %s\n", DPT_FMT< std::complex<T_> >::data);
	fprintf(log, log_fmt, i, p.real(), p.imag());
}

template <typename T_>
static void log_result_color(FILE *log, unsigned i, T_ p)
{
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(console, COLORED_ROW);
	log_result(log, i, p);
	SetConsoleTextAttribute(console, DEF_ROW_COLOR);
}

#endif