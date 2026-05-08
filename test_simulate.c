/*
 * 模拟测试程序 - 验证核心逻辑（数据打包、数据库操作）
 * 在 Windows/MinGW 环境下测试 common 库的功能
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "common/packet.h"
#include "common/database.h"
#include "common/cJSON.h"
#include "common/log.h"

static int test_count = 0;
static int pass_count = 0;

void assert_eq(const char *name, int expected, int actual)
{
    test_count++;
    if(expected == actual)
    {
        printf("[PASS] %s\n", name);
        pass_count++;
    }
    else
    {
        printf("[FAIL] %s: expected %d, got %d\n", name, expected, actual);
    }
}

void assert_not_null(const char *name, const void *ptr)
{
    test_count++;
    if(ptr != NULL)
    {
        printf("[PASS] %s\n", name);
        pass_count++;
    }
    else
    {
        printf("[FAIL] %s: got NULL\n", name);
    }
}

void assert_str_contains(const char *name, const char *haystack, const char *needle)
{
    test_count++;
    if(strstr(haystack, needle) != NULL)
    {
        printf("[PASS] %s\n", name);
        pass_count++;
    }
    else
    {
        printf("[FAIL] %s: '%s' not found in '%s'\n", name, needle, haystack);
    }
}

int main(void)
{
    data_t data;
    char buf[512];
    int ret;
    sqlite3 *db = NULL;

    printf("========================================\n");
    printf("  APUE IoT 温度采集系统 - 模拟测试\n");
    printf("========================================\n\n");

    /* Test 1: get_devid */
    printf("--- Test 1: get_devid ---\n");
    memset(&data, 0, sizeof(data));
    get_devid(&data, 1);
    assert_eq("get_devid returns correct ID format", 0, strncmp(data.id, "RPI#0001", 8));
    printf("  Device ID: %s\n\n", data.id);

    /* Test 2: date_packet */
    printf("--- Test 2: date_packet (JSON打包) ---\n");
    strncpy(data.time, "2026-05-08 22:00:00", sizeof(data.time) - 1);
    data.temperature = 25.678;

    ret = date_packet(&data, buf, sizeof(buf));
    assert_eq("date_packet success", 0, ret);
    assert_str_contains("JSON contains ID", buf, "RPI#0001");
    assert_str_contains("JSON contains TIME", buf, "2026-05-08 22:00:00");
    assert_str_contains("JSON contains TEMPERATURE", buf, "25.678");
    printf("  JSON output: %s\n\n", buf);

    /* Test 3: cJSON parsing */
    printf("--- Test 3: cJSON 解析验证 ---\n");
    cJSON *root = cJSON_Parse(buf);
    assert_not_null("cJSON_Parse success", root);

    if(root)
    {
        cJSON *id = cJSON_GetObjectItem(root, "ID");
        cJSON *time = cJSON_GetObjectItem(root, "TIME");
        cJSON *temp = cJSON_GetObjectItem(root, "TEMPERATURE");

        assert_not_null("ID field exists", id);
        assert_not_null("TIME field exists", time);
        assert_not_null("TEMPERATURE field exists", temp);

        if(id)
            assert_eq("ID value correct", 0, strcmp(id->valuestring, "RPI#0001"));
        if(time)
            assert_eq("TIME value correct", 0, strcmp(time->valuestring, "2026-05-08 22:00:00"));
        if(temp)
            assert_eq("TEMPERATURE value correct", 0,
                       (temp->valuedouble > 25.6 && temp->valuedouble < 25.7) ? 0 : 1);

        cJSON_Delete(root);
    }
    printf("\n");

    /* Test 4: Database operations */
    printf("--- Test 4: 数据库操作 ---\n");

    /* Remove old test db */
    remove("test_temp.db");

    /* Create database */
    ret = sqlite3_open("test_temp.db", &db);
    assert_eq("sqlite3_open success", 0, ret);

    if(db)
    {
        /* Create table */
        char *zErrMsg = NULL;
        char *sql = "CREATE TABLE TEMP_RECDS(DATA TEXT NOT NULL);";
        ret = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
        assert_eq("CREATE TABLE success", 0, ret);
        if(ret != SQLITE_OK)
        {
            printf("  Error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }

        /* Insert data */
        sqlite3_stmt *stmt;
        sql = "INSERT INTO TEMP_RECDS (DATA) VALUES(?);";
        ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        assert_eq("INSERT prepare success", 0, ret);

        if(ret == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, buf, -1, SQLITE_STATIC);
            ret = sqlite3_step(stmt);
            /* SQLITE_DONE (101) is expected for INSERT */
            assert_eq("INSERT step success", 101, ret);
            sqlite3_finalize(stmt);
        }

        /* Insert multiple records */
        for(int i = 0; i < 5; i++)
        {
            data_t temp_data;
            char temp_buf[512];

            memset(&temp_data, 0, sizeof(temp_data));
            get_devid(&temp_data, i + 2);
            snprintf(temp_data.time, sizeof(temp_data.time), "2026-05-08 22:0%d:00", i);
            temp_data.temperature = 20.0 + i * 1.5;

            date_packet(&temp_data, temp_buf, sizeof(temp_buf));

            sql = "INSERT INTO TEMP_RECDS (DATA) VALUES(?);";
            sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
            sqlite3_bind_text(stmt, 1, temp_buf, -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        /* Count records */
        int count = 0;
        sql = "SELECT COUNT(*) FROM TEMP_RECDS;";
        ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if(ret == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);

        test_count++;
        if(count == 6)
        {
            printf("[PASS] Record count: %d\n", count);
            pass_count++;
        }
        else
        {
            printf("[FAIL] Record count: expected 6, got %d\n", count);
        }

        /* Read back and verify */
        sql = "SELECT DATA FROM TEMP_RECDS LIMIT 1;";
        ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        assert_eq("SELECT prepare success", 0, ret);

        if(ret == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char *retrieved = sqlite3_column_text(stmt, 0);
            assert_not_null("Retrieved data not null", retrieved);
            if(retrieved)
            {
                assert_str_contains("Retrieved data contains ID", (const char*)retrieved, "RPI#");
                printf("  Retrieved: %s\n", retrieved);
            }
        }
        sqlite3_finalize(stmt);

        sqlite3_close(db);
    }
    printf("\n");

    /* Cleanup */
    remove("test_temp.db");

    /* Summary */
    printf("========================================\n");
    printf("  测试结果: %d/%d 通过\n", pass_count, test_count);
    printf("========================================\n");

    if(pass_count == test_count)
    {
        printf("\n  所有测试通过! 核心逻辑工作正常。\n");
        printf("  注意: 网络(socket)和传感器(ds18b20)模块\n");
        printf("  需要在 Linux 环境下编译测试。\n");
    }
    else
    {
        printf("\n  有 %d 个测试失败，请检查代码。\n", test_count - pass_count);
    }

    return (pass_count == test_count) ? 0 : 1;
}
