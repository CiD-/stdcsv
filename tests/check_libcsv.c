#include <check.h>
#include <stdlib.h>
#include "../lib/include/csv.h"

char* buf = NULL;
size_t buflen = 0;
size_t linelen = 0;
FILE* _file = NULL;

/**
 * safegetline Testing
 */

void sgl_setup(void)
{
        buf = NULL;
        buflen = 0;
        linelen = 0;
        _file = NULL;
}

void sgl_teardown(void)
{
        free(buf);
        buf = NULL;
        fclose(_file);
}

void test_getline1()
{
        int ret = sgetline(_file, &buf, &buflen, &linelen);
        ck_assert_ptr_nonnull(buf);
        ck_assert_str_eq(buf, "123,456,789");
        ck_assert_int_eq(ret, 0);
        ck_assert_uint_eq(linelen, 11);
        ck_assert_uint_eq(buflen, BUFFER_FACTOR);

        ret = sgetline(_file, &buf, &buflen, &linelen);
        ck_assert_str_eq(buf, "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345");
        ck_assert_uint_eq(linelen, strlen("012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345"));
        ck_assert_int_eq(ret, 0);
        printf("BUFFER SIZE: %lu\n", buflen);
        //ck_assert_uint_eq(buflen, BUFFER_FACTOR);

        ret = sgetline(_file, &buf, &buflen, &linelen);
        ck_assert_int_eq(ret, EOF);
}

START_TEST(test_safegetline_lf)
{
        _file = fopen("test_lf.txt", "r");
        if (!_file) {
                perror("test_lf.txt");
                exit(EXIT_FAILURE);
        }
        test_getline1();
}
END_TEST

START_TEST(test_safegetline_cr)
{
        _file = fopen("test_cr.txt", "r");
        if (!_file) {
                perror("test_cr.txt");
                exit(EXIT_FAILURE);
        }
        test_getline1();
}
END_TEST

START_TEST(test_safegetline_crlf)
{
        _file = fopen("test_crlf.txt", "r");
        if (!_file) {
                perror("test_crlf.txt");
                exit(EXIT_FAILURE);
        }
        test_getline1();
}
END_TEST

START_TEST(test_safegetline_notrail)
{
        _file = fopen("test_notrail.txt", "r");
        if (!_file) {
                perror("test_notrail.txt");
                exit(EXIT_FAILURE);
        }
        test_getline1();
}
END_TEST

START_TEST(test_safegetline_long)
{
        _file = fopen("test_long.txt", "r");
        if (!_file) {
                perror("test_long.txt");
                exit(EXIT_FAILURE);
        }
        int ret = sgetline(_file, &buf, &buflen, &linelen);
        ck_assert_ptr_nonnull(buf);
        ck_assert_str_eq(buf, "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
        ck_assert_uint_eq(linelen, strlen("012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"));
        ck_assert_int_eq(ret, 0);
        ck_assert_uint_gt(buflen, BUFFER_FACTOR);

        ret = sgetline(_file, &buf, &buflen, &linelen);
        ck_assert_int_eq(ret, EOF);
}
END_TEST




/**
 * RFC-4180 Testing
 */

struct csv_reader* reader = NULL;
struct csv_record* record = NULL;

void parse_setup(void)
{
        reader = csv_reader_new();
        record = NULL;
}

void parse_teardown(void)
{
        csv_reader_free(reader);
}


START_TEST(test_parse_rfc)
{
        record = csv_parse(reader, "123,456,789");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "123");
        ck_assert_str_eq(record->fields[1], "456");
        ck_assert_str_eq(record->fields[2], "789");
}


Suite* csv_reader_suite(void)
{
        Suite* s;
        s = suite_create("Reader");

        TCase* tc_sgl = tcase_create("safegetline");
        tcase_add_checked_fixture(tc_sgl, sgl_setup, sgl_teardown);
        tcase_add_test(tc_sgl, test_safegetline_lf);
        tcase_add_test(tc_sgl, test_safegetline_cr);
        tcase_add_test(tc_sgl, test_safegetline_crlf);
        tcase_add_test(tc_sgl, test_safegetline_notrail);
        tcase_add_test(tc_sgl, test_safegetline_long);
        suite_add_tcase(s, tc_sgl);

        TCase* tc_parse_rfc = tcase_create("rfc4180");
        tcase_add_checked_fixture(tc_parse_rfc, parse_setup, parse_teardown);
        tcase_add_test(tc_parse_rfc, test_parse_rfc);
        suite_add_tcase(s, tc_parse_rfc);

        return s;
}

int main(void)
{
        int number_failed;
        Suite* s = csv_reader_suite();
        SRunner* sr = srunner_create(s);
        srunner_set_fork_status (sr, CK_NOFORK);

        srunner_run_all(sr, CK_VERBOSE);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

