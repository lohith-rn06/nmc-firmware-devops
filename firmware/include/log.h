#ifndef NMC_LOG_H
#define NMC_LOG_H
enum { LOG_DBG = 0, LOG_INF, LOG_WRN, LOG_ERR };
void nmc_log(int level, const char *module, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
void nmc_log_set_level(int level);
#endif
