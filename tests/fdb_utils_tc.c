/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief utils testcases.
 *
 * This testcases is be used in RT-Thread Utest framework.
 * If you want run it, please add it to RT-Thread project.
 *
 * These cases cover the pure helper functions in fdb_utils.c which do not
 * depend on any flash/file storage backend:
 *   - fdb_calc_crc32()
 *   - fdb_blob_make()
 *   - _fdb_set_status() / _fdb_get_status()
 */

#include "utest.h"
#include <flashdb.h>
#include <fdb_low_lvl.h>
#include <stdio.h>
#include <string.h>

#if defined(RT_USING_UTEST)

/* Status number used by the status table round-trip test. The library only
 * ever uses small status counts (FDB_KV_STATUS_NUM / FDB_TSL_STATUS_NUM are 6,
 * the sector store/dirty counts are 4), i.e. a single status byte for nor
 * flash (FDB_WRITE_GRAN == 1). _fdb_set_status() re-erases the whole table on
 * every call, so the set/get round trip is only well defined within that
 * single-byte range; use the largest real status count here. */
#define TEST_STATUS_NUM               FDB_KV_STATUS_NUM

static rt_err_t utest_tc_init(void)
{
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

/* CRC32 must match the well known IEEE 802.3 (zlib) result. */
static void test_fdb_calc_crc32(void)
{
    const char *check = "123456789";

    /* the standard check value of the CRC32 algorithm */
    uassert_true(fdb_calc_crc32(0, check, strlen(check)) == 0xCBF43926);
    /* an empty buffer keeps the accumulated value unchanged */
    uassert_true(fdb_calc_crc32(0, check, 0) == 0);
}

/* Calculating in one shot must equal calculating incrementally. */
static void test_fdb_calc_crc32_incremental(void)
{
    const char *check = "123456789";
    uint32_t whole, part;

    whole = fdb_calc_crc32(0, check, strlen(check));

    part = fdb_calc_crc32(0, check, 5);
    part = fdb_calc_crc32(part, check + 5, strlen(check) - 5);

    uassert_true(part == whole);
    uassert_true(part == 0xCBF43926);
}

/* fdb_blob_make must fill the blob and return the same object. */
static void test_fdb_blob_make(void)
{
    struct fdb_blob blob;
    uint8_t buf[8];
    fdb_blob_t result;

    memset(&blob, 0, sizeof(blob));
    result = fdb_blob_make(&blob, buf, sizeof(buf));

    uassert_true(result == &blob);
    uassert_true(blob.buf == buf);
    uassert_true(blob.size == sizeof(buf));

    /* a zero length blob is allowed */
    result = fdb_blob_make(&blob, NULL, 0);
    uassert_true(result == &blob);
    uassert_true(blob.buf == NULL);
    uassert_true(blob.size == 0);
}

/*
 * Setting a status and then reading it back must return the same status index,
 * for every valid index. This property holds for any FDB_WRITE_GRAN value.
 */
static void test_fdb_set_get_status(void)
{
    uint8_t status_table[FDB_STATUS_TABLE_SIZE(TEST_STATUS_NUM)];
    size_t index, byte_index;

    for (index = 0; index < TEST_STATUS_NUM; index++) {
        byte_index = _fdb_set_status(status_table, TEST_STATUS_NUM, index);
        /* the first status is all 1, so no flash write (byte index) is needed */
        if (index == 0) {
            uassert_true(byte_index == SIZE_MAX);
        } else {
            uassert_true(byte_index != SIZE_MAX);
        }
        uassert_true(_fdb_get_status(status_table, TEST_STATUS_NUM) == index);
    }
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_fdb_calc_crc32);
    UTEST_UNIT_RUN(test_fdb_calc_crc32_incremental);
    UTEST_UNIT_RUN(test_fdb_blob_make);
    UTEST_UNIT_RUN(test_fdb_set_get_status);
}

UTEST_TC_EXPORT(testcase, "packages.system.flashdb.utils", utest_tc_init, utest_tc_cleanup, 10);
#endif /* defined(RT_USING_UTEST) */
