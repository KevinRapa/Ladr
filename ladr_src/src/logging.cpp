#include <ctime>

#include <logging.h>

static char LOG_DIR[512] = {0};

constexpr size_t MAX_ID_LEN = 128;
constexpr size_t MAX_RES_PATH_LEN = sizeof(LOG_DIR) + MAX_ID_LEN + 32;

struct results_log {
	char actual_name[MAX_RES_PATH_LEN];
	char expect_name[MAX_RES_PATH_LEN];
	FILE *actual;
	FILE *expected;
};

static bool open_result_file(const char *pref, const char *id,
                             FILE **file, char path[MAX_RES_PATH_LEN])
{
	snprintf(path, MAX_RES_PATH_LEN, "%s/%s__%s.bin", LOG_DIR, id, pref);

	fopen_s(file, path, "w");
	if (*file) {
		return true;
	} else {
		fprintf(stderr, "Failed to open %s\n", path);
		return false;
	}
}

struct results_log *get_results_log(const char *id)
{
	struct results_log *log;

	if (!*LOG_DIR) {
		return NULL;
	}

	if (strnlen(id, MAX_ID_LEN) == MAX_ID_LEN) {
		fprintf(stderr,
		        "Failed to open results file. Max call ID length is %zu\n",
		        MAX_ID_LEN);
		return NULL;
	}

	log = new struct results_log;

	if (!open_result_file("expected", id, &log->expected, log->expect_name)) {
		goto err;
	}
	if (!open_result_file("actual", id, &log->actual, log->actual_name)) {
		fclose(log->expected);
		remove(log->expect_name);
		goto err;
	}

	return log;
err:
	delete log;
	return NULL;
}

void log_results(struct results_log *log, void *expect,
                    void *actual, size_t elem_sz, size_t block_len)
{
	fwrite(expect, elem_sz, block_len, log->expected);
	fwrite(actual, elem_sz, block_len, log->actual);
}

void close_results_log(struct results_log *log)
{
	fclose(log->actual);
	fclose(log->expected);
	delete log;
}

bool open_log(FILE **log)
{
	char log_path[sizeof(LOG_DIR) + sizeof("log.txt")];

	if (*LOG_DIR) {
		snprintf(log_path, sizeof(log_path), "%s/%s", LOG_DIR, "log.txt");

		if (fopen_s(log, log_path, "a")) {
			fprintf(stderr, "Failed to open %s\n", log_path);
		} else {
			return true;
		}
	}

	return false;
}

bool make_log_dir(const char *path)
{
	std::time_t t = std::time(0);
	std::tm now;
	int ret;

	if (*LOG_DIR) {
		return true; // Was already made from an earlier call to this
	}

	if (localtime_s(&now, &t)) {
		fprintf(stderr, "Failed to get local time. Log directory is %s/ladr\n",
		        path);
		ret = snprintf(LOG_DIR, sizeof(LOG_DIR), "%s/ladr", path);
	} else {
		ret = snprintf(LOG_DIR, sizeof(LOG_DIR),
		               "%s/ladr__%d-%d-%d__%d.%d.%d", path,
		               now.tm_mon + 1, now.tm_mday, now.tm_year % 100,
		               now.tm_hour, now.tm_min, now.tm_sec);
	}

	if (ret == sizeof(LOG_DIR)) {
		fprintf(stderr, "Log directory path is too long\n");
		goto err;
	}
	if (ret < 0) {
		fprintf(stderr, "Unknown error composing directory name\n");
		goto err;
	}

	if (!CreateDirectoryA(LOG_DIR, NULL)) {
		DWORD last_error = GetLastError();

		if (last_error == ERROR_ALREADY_EXISTS) {
			fprintf(stderr,
			        "%s already exists. Move or delete before running again\n",
			        LOG_DIR);
		} else {
			fprintf(stderr, "Failed to make log directory [%lu]\n",
			        last_error);
		}
		
		goto err;
	}

	return true;
err:
	ZeroMemory(LOG_DIR, sizeof(LOG_DIR));
	return false;
}