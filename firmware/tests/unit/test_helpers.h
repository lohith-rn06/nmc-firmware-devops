#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H
#include "nmc.h"
#include "security.h"
#include <string.h>
/* Deterministic config for tests: admin/Test#Pass1, community "lab-ro" */
static void test_config(void)
{
    memset(&g_cfg, 0, sizeof g_cfg);
    strcpy(g_cfg.admin_user, "admin");
    strcpy(g_cfg.salt, "0011223344556677");
    strcpy(g_cfg.snmp_community, "lab-ro");
    auth_hash_password(g_cfg.salt, "Test#Pass1", g_cfg.admin_pass_hash);
}
#endif
