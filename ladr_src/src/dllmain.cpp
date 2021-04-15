#define DLL_EXPORTS

#include <Windows.h>
#include <Winsock.h>

#include <complex>
#include <string>
#include <unordered_set>

#include <logging.h>
#include <private/ladr_common.h>

#pragma comment(lib, "wsock32.lib") // Link in Winsock

constexpr unsigned short PORT = 10000 + 'l' + 'a' + 'd' + 'r'; // 10419

static unsigned checks_made = 0; // For tracking # of calls to ladr_assert

static std::unordered_set<std::string> only_ids;

/*
 * Config values
 */
namespace CFG {
	static unsigned mismatch_threshold = 30;
	static char save_policy = 'n';
	static char on_mismatch = 'f';
	static bool log_to_file = false;
	static bool color_rows = true;
	static bool dry_run = false;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved)
{
	return TRUE;
}

static bool ladr_recv(SOCKET sock, void *buf, unsigned len)
{
	int bytes_read;
	unsigned i = 5000; // Tries. Will wait 5s max

	do {
		bytes_read = recv(sock, (char *)buf, len, MSG_PEEK);
		if (bytes_read == SOCKET_ERROR) {
			wsa_error("Failure receiving client data");
			return false;
		}

		--i;
		Sleep(1);
	} while (i != 0 && (unsigned)bytes_read < len);

	if (i == 0) {
		fprintf(stderr, "Timeout waiting for client message\n");
	} else {
		recv(sock, (char *)buf, len, 0);
	}

	return (i != 0);
}

static bool ladr_send(SOCKET sock, void *buf, unsigned len)
{
	int ret = send(sock, (char *)buf, len, 0);
	if (ret == SOCKET_ERROR) {
		wsa_error("Failed to send data to client");
	}
	return ret != SOCKET_ERROR;
}

static SOCKET ladr_connect(unsigned short port)
{
	sockaddr_in cfg;
	WSADATA dont_care;
	SOCKET lstn;
	SOCKET ret;

	cfg.sin_family = AF_INET;
	cfg.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	cfg.sin_port = htons(port);

	// I have no idea what 'MAKEWORD(2, 2)' is. API says it's a 'TBD' argument.
	if (WSAStartup(MAKEWORD(2, 2), &dont_care) != NOERROR) {
		wsa_error("WSAStartup failed");
		goto err;
	}

	lstn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (lstn == INVALID_SOCKET) {
		wsa_error("Failed to create socket");
		goto err_clean;
	}

	if (bind(lstn, (sockaddr *)&cfg, sizeof(cfg)) == SOCKET_ERROR) {
		wsa_error("Failed to bind socket");
		goto err_close;
	}

	if (listen(lstn, SOMAXCONN) == SOCKET_ERROR) {
		wsa_error("Failure during listening");
		goto err_close;
	}

	// This will block forever if client code has less calls to 'check'
	ret = accept(lstn, NULL, NULL);
	if (ret == INVALID_SOCKET) {
		wsa_error("Failed to accept client connection");
		goto err_close;
	}

	closesocket(lstn);
	return ret;

err_close:
	closesocket(lstn);
err_clean:
	WSACleanup();
err:
	return INVALID_SOCKET;
}

template <typename T_>
static void cmp_data_nr(FILE *log, T_ *data, unsigned len)
{
	unsigned i;

	for (i = 0; i < len && i < CFG::mismatch_threshold; i++) {
		if (CFG::color_rows && !CFG::log_to_file && (i & 1)) {
			log_result_color(log, i, data[i]);
		} else {
			log_result(log, i, data[i]);
		}
	}

	fprintf(log, "\n");
}

template <typename T_>
static bool cmp_data(
	SOCKET sock,
	FILE *log,
	struct results_log *results,
	T_ *test_data,
	unsigned len,
	double eps)
{
	/* I found in practice that MATLAB didn't want to send more
	then 10416 bytes at a time, so I chose 8192 as the max and
	as a power of 2 */
	T_ client_data[8192 / sizeof(T_)];

	T_ actual;
	T_ expected;
	double diff;
	double max_diff = 0.0;
	uint32_t block_len = 0;
	unsigned dpts_read = 0;
	unsigned num_mismatch = 0;
	unsigned i;
	bool ret = false;

	log_header<T_>(log);

	block_len = ARRAYSIZE(client_data);
	if (!ladr_send(sock, &block_len, sizeof(block_len))) {
		goto err;
	}

	while (dpts_read != len) {
		block_len = 0;

		// Receive the next block length, then read block
		if (!ladr_recv(sock, &block_len, sizeof(block_len)) ||
		    !ladr_recv(sock, client_data, block_len * sizeof(T_)))
		{
			goto err;
		}

		dpts_read += block_len;

		for (i = 0; i < block_len; i++) {
			actual = test_data[i];
			expected = client_data[i];

			diff = (double)std::abs(actual - expected)
			       / (double)std::abs(expected);

			if (diff > eps) {
				++num_mismatch;

				if (num_mismatch <= CFG::mismatch_threshold) {
					if (CFG::color_rows && !CFG::log_to_file && (num_mismatch & 1)) {
						log_diff_color(log, dpts_read - block_len + i,
						               expected, actual, diff);
					} else {
						log_diff(log, dpts_read - block_len + i,
						         expected, actual, diff);
					}
				}

				max_diff = max(max_diff, diff);
			}
		}

		if (results) {
			log_results(results, client_data, test_data,
			            sizeof(T_), block_len);
		}

		if (CFG::on_mismatch == 'f') {
			for (i = 0; i < block_len; i++) {
				test_data[i] = client_data[i];
			}
		}

		// Echo block length to tell client to continue
		if (!ladr_send(sock, &block_len, sizeof(block_len))) {
			goto err;
		}

		test_data += block_len;
	}

	if (!(CFG::on_mismatch == 's' && num_mismatch)) {
		ret = true;
	}

err:
	fprintf(log, "\n\tTotal mismatches: %u\n"
	             "\tMax error:          %f\n"
	             "\tResults %s\n\n",
	        num_mismatch, max_diff,
	        (num_mismatch ? "MISMATCH" : (ret ? "MATCH" : "UNKNOWN")));
	return ret;
}

void ladr_dll_config(enum LADR_OPT opt, unsigned long val)
{
	switch (opt) {
	case LADR_OPT::L_OPT_ON_MISMATCH:
		switch (val) {
		case 'n':
		case 'f':
		case 's':
			CFG::on_mismatch = (char)val;
			break;
		default:
			fprintf(stderr, "Unknown value for L_MISMATCH\n");
			break;
		}
		break;
	case LADR_OPT::L_OPT_COLOR:
		CFG::color_rows = !!val;
		break;
	case LADR_OPT::L_OPT_DRY_RUN:
		CFG::dry_run = !!val;
		break;
	case LADR_OPT::L_OPT_LOG:
		if (val) {
			CFG::log_to_file = make_log_dir((const char *)val);
		} else {
			CFG::log_to_file = false;
		}
		break;
	case LADR_OPT::L_OPT_LOG_COUNT:
		CFG::mismatch_threshold = (unsigned)val;
		break;
	case LADR_OPT::L_OPT_ONLY:
		only_ids.insert(std::string((const char *)val));
		break;
	case LADR_OPT::L_OPT_SAVE:
		switch (val) {
		case 'y':
		case '1':
		case 'n':
			CFG::save_policy = (char)val;
			break;
		default:
			fprintf(stderr, "Unknown value for L_SAVE\n");
			break;
		}
		break;
	default:
		break;
	}
}

void ladr_dll_check(void *data, unsigned len, double eps,
                    enum LADR_CHECK_T type, const char *id)
{
	CONSOLE_SCREEN_BUFFER_INFO console_info;
	char _id[8]; // Holds ID if none given
	HANDLE console;
	struct results_log *results;
	FILE *log;
	SOCKET sock;
	uint32_t type32;
	bool open_success;

	// What ladr_assert uses to know if it should fail early
	static bool critical_error = false;

	++checks_made;

	if (!data || !len) {
		return; // No data to compare, no mismatch. Thus, this passes.
	}

	// Standardize ID to a string
	if (!id) {
		snprintf(_id, sizeof(_id), "%u", checks_made);
		id = _id;
	}

	// Connect with the client
	if (!CFG::dry_run) {
		sock = ladr_connect(PORT);
		if (sock == INVALID_SOCKET) {
			return;
		} else if (critical_error) {
			type32 = (uint32_t)LADR_CHECK_T::RESERVED;
			ladr_send(sock, &type32, sizeof(type32));
			closesocket(sock);
			WSACleanup();
			return;
		}
	}

	// Open file to log to
	log = stdout;
	results = NULL;
	open_success = false;

	if (CFG::log_to_file) {
		open_success = open_log(&log);
		if (!open_success) {
			log = stdout;
		}
		if (CFG::save_policy == 'y' || CFG::save_policy == '1') {
			results = get_results_log(id);

			if (CFG::save_policy == '1') {
				CFG::save_policy = 'n';
			}
		}
	}

	// If coloring rows, set default row color.
	// Note that this has no effect on file output
	console = NULL;
	if (CFG::color_rows && !CFG::log_to_file) {
		console = GetStdHandle(STD_OUTPUT_HANDLE);
		if (console) {
			GetConsoleScreenBufferInfo(console, &console_info);
			SetConsoleTextAttribute(console, DEF_ROW_COLOR);
		}
	}

	if (!CFG::dry_run && eps < 0) {
		fprintf(log, "Check %s: negative epsilon provided\n", id);
		goto cleanup;
	}

	// If L_ONLY option has been used, skip check if not in set
	if (id && !only_ids.empty() && !only_ids.count(std::string(id))) {
		if (!CFG::dry_run) {
			type32 = (uint32_t)LADR_CHECK_T::RESERVED;
			ladr_send(sock, &type32, sizeof(type32));
		}
		goto cleanup;
	}

	/* Send the expected type of data to the client, then
	   receive number of data points it intends to send */
	if (!CFG::dry_run) {
		type32 = (uint32_t)type;
		uint32_t recv_len = 0;

		if (!ladr_send(sock, &type32, sizeof(type32)) ||
			!ladr_recv(sock, &recv_len, sizeof(recv_len)))
		{
			critical_error = true;
			goto cleanup;
		}

		if (recv_len == LADR_CHECK_T::RESERVED) {
			/* If the client has complex data, and our is real, we
			   can't correct for that. */
			fprintf(log, "Check %s: complex/real mismatch in data types\n\n", id);
			goto cleanup;
		}
		
		if (recv_len != len) {
			/* There's a mismatch in length. Alert the client
			   by sending the expected amount of data. */
			fprintf(log, "Check %s: data length mismatch (test: %u, client: %u)\n\n",
			        id, len, recv_len);
			recv_len = len;
			ladr_send(sock, &recv_len, sizeof(recv_len));
			goto cleanup;
		}

		// Echo back that we agree on the length
		if (!ladr_send(sock, &recv_len, sizeof(recv_len))) {
			critical_error = true;
			goto cleanup;
		}
	}

	fprintf(log, "Check %s (epsilon = %f)\n\n", id, eps);

#define _cmp_data_type(T_) \
if (CFG::dry_run) { \
	cmp_data_nr(log, (T_ *)data, len); \
} else if (!cmp_data(sock, log, results, (T_ *)data, len, eps)) { \
	critical_error = true; \
}

	switch (type) {
	case LADR_CHECK_T::CHK_FLT:
		_cmp_data_type(float);
		break;
	case LADR_CHECK_T::CHK_DBL:
		_cmp_data_type(double);
		break;
	case LADR_CHECK_T::CHK_INT:
		_cmp_data_type(int);
		break;
	case LADR_CHECK_T::CHK_CMP_FLT:
		_cmp_data_type(std::complex<float>);
		break;
	case LADR_CHECK_T::CHK_CMP_DBL:
		_cmp_data_type(std::complex<double>);
		break;
	default:
		break;
	}

#undef _cmp_data_type

cleanup:
	if (CFG::color_rows && !CFG::log_to_file && console) {
		SetConsoleTextAttribute(console, console_info.wAttributes);
	}
	if (results) {
		close_results_log(results);
	}
	if (open_success) {
		fclose(log);
	}
	if (!CFG::dry_run) {
		closesocket(sock);
		WSACleanup();
	}
}
