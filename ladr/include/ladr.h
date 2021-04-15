#ifndef LADR_H
#define LADR_H

#ifdef LADR
#include "private/ladr_toggle.h"
#endif

/**
 * ladr_config
 * @brief Configures the call to ladr_check
 * @details This may be called multiple times and at any time
 *          before a call to ladr_check. Calling this may
 *          change the behavior of the next call to ladr_check.
 *
 *          Option      | Value | Description
 *                      | type  |
 *          ------------|-------|----------------------------------------
 *          L_LOG       | const | Pointer to a C string of a path to a
 *                      | char* | directory to store logs to. If specified,
 *                      |       | Ladr will make a directory here and
 *                      |       | store logs to a file inside it instead
 *                      |       | of printing to standard out.
 *          ------------|-------|----------------------------------------
 *          L_LOG_COUNT | int   | Number of mismatches to report for
 *                      |       | each call to ladr_check [default 30].
 *          ------------|-------|----------------------------------------
 *          L_SAVE      | char  | How to dump intermediate results. Data
 *                      |       | will be dumped in a binary format into
 *                      |       | the log directory supplied to start. If
 *                      |       | it was not given, this option is ignored.
 *                      |       |
 *                      |       | Values:  'n' - Never save [default]
 *                      |       |          '1' - Save on next ladr_check
 *                      |       |          'y' - Always save
 *          ------------|-------|----------------------------------------
 *          L_MISMATCH  | char  | Set a special action to take after a check.
 *                      |       |
 *                      |       | Values:  'n' - No special action
 *                      |       |          'f' - Fix actual results by setting
 *                      |       |                them equal to the expected
 *                      |       |                results
 *                      |       |          's' - Skip subsequent checks after
 *                      |       |                a mismatch is found
 *          ------------|-------|----------------------------------------
 *          L_COLOR     | bool  | Alternate row color in output table if
 *                      |       | log is not being written to a file.
 *                      |       | [default 'true']
 *          ------------|-------|----------------------------------------
 *          L_DRY_RUN   | bool  | If true, do not compare data or attempt
 *                      |       | to contact the client process. Only log
 *                      |       | the results of the test data.
 *                      |       | [default 'false']
 *          ------------|-------|----------------------------------------
 *          L_ONLY      | const | Specify a list of strings. Treat these
 *                      | char* | strings as IDs, and only run checks that
 *                      | ...   | correspond to these IDs.
 *                      |       | Example: ladr_config(L_ONLY, "one", "two", "three");
 *
 * @param opt Config option
 * @param ... Value[s] for the option
 */
#ifdef LADR
#define ladr_config(opt, ...) _ladr_config(opt, __VA_ARGS__)
#else
#define ladr_config(...)
#endif

/**
 * ladr_check
 * @brief Checks @data against data produced by the client program
 * @details The client program, which produces 'expected' results,
 *          is assumed to have an equal number of calls to its own
 *          implementation of this. This function compares @data
 *          against the results produced by the client program at
 *          its analogous call in the client. If a mismatch is
 *          found, this prints a log.
 *
 *          The current types supported are:
 *              float, double, int
 *              std::complex<float>
 *              std::complex<double>
 *
 * @param data Array of data to check
 * @param len  Number of data points in @data
 * @param eps  Tolerance for differences in results.
 *             A value in @data is a mismatch if
 *                 @eps < abs(value - expected) / abs(expected)
 *             If @eps is 0, then the value must match exactly.
 * @param id   Optional parameter for logging.
 *             Instead of a number for identifying
 *             this call, this string will be used.
 * @warning    If saving intermediate results, @id will be used in
 *             the file names. Ensure that @id does not contain
 *             illegal characters (e.g. ':' for Windows)
 * @warning    Do not include the '<type>'; let the compiler do that,
 *             or compiler errors results if LADR is undefined
 */
#ifdef LADR
template <typename T_>
static inline void ladr_check(T_ *data, unsigned len, double eps,
                              const char *id = NULL)
{
	_ladr_check(data, len, eps, id);
}
#else
#define ladr_check(...)
#endif

#endif
