#ifndef LADR_COMMON_H
#define LADR_COMMON_H

#ifndef __cplusplus
#error "Ladr is currently only C++ compatible."
#endif

enum LADR_OPT {
	L_OPT_LOG, L_OPT_LOG_COUNT, L_OPT_SAVE, L_OPT_ON_MISMATCH,
	L_OPT_COLOR, L_OPT_DRY_RUN, L_OPT_ONLY,
};

/* Configuration options. User shouldn't use the enum directly because
   their build process may complain when LADR is undefined */
#define L_LOG       LADR_OPT::L_OPT_LOG
#define L_LOG_COUNT LADR_OPT::L_OPT_LOG_COUNT
#define L_SAVE      LADR_OPT::L_OPT_SAVE
#define L_MISMATCH  LADR_OPT::L_OPT_ON_MISMATCH
#define L_COLOR     LADR_OPT::L_OPT_COLOR
#define L_DRY_RUN   LADR_OPT::L_OPT_DRY_RUN
#define L_ONLY      LADR_OPT::L_OPT_ONLY

#ifdef DLL_EXPORTS
#define __dll_func __declspec(dllexport)
#else
#define __dll_func __declspec(dllimport)
#endif

enum LADR_CHECK_T {
	RESERVED = 0,  // Sending this tells client to return from check
	CHK_FLT = 1,
	CHK_DBL = 2,
	CHK_INT = 3,
	CHK_CMP_FLT = 4,
	CHK_CMP_DBL = 5,
};

extern "C" {
	void __dll_func ladr_dll_config(enum LADR_OPT, unsigned long);
	void __dll_func ladr_dll_check(void *, unsigned, double,
	                               enum LADR_CHECK_T, const char *);
}

#endif