#include <check.h>
#include <stdlib.h>
#include "csv.h"

struct csv_reader* reader = NULL;
struct csv_record* record = NULL;

void parse_setup(void)
{
        reader = csv_reader_new();
        record = csv_record_new();
}

void parse_teardown(void)
{
        csv_reader_free(reader);
}

/**
 * Parse Testing
 */

void _field_check(struct csv_field* field, const char* s1)
{
	char* s0 = strndup(field->data, field->len);
	ck_assert_str_eq(s0, s1);
	free(s0);
}

void file_setup(void)
{
        parse_setup();
        csv_reader_open_mmap(reader, "basic.csv");
}


START_TEST(test_file_rfc)
{
        int ret = 0;

        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "123");
        _field_check(&record->fields[1], "456");
        _field_check(&record->fields[2], "789");

        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "d|ef");
        _field_check(&record->fields[2], "ghi");

        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\nf");
        _field_check(&record->fields[2], "ghi");

        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\"f");
        _field_check(&record->fields[2], "ghi");

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, EOF);

        unsigned rows = csv_reader_row_count(reader);
        unsigned breaks = csv_reader_embedded_breaks(reader);

        ck_assert_uint_eq(rows, 4);
        ck_assert_uint_eq(breaks, 1);
}
END_TEST

START_TEST(test_file_weak)
{
        int ret = 0;

        reader->quotes = QUOTE_WEAK;
        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "123");
        _field_check(&record->fields[1], "456");
        _field_check(&record->fields[2], "789");

        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "d|ef");
        _field_check(&record->fields[2], "ghi");

        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\nf");
        _field_check(&record->fields[2], "ghi");

        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\"\"f");
        _field_check(&record->fields[2], "ghi");

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, EOF);

        unsigned rows = csv_reader_row_count(reader);
        unsigned breaks = csv_reader_embedded_breaks(reader);

        ck_assert_uint_eq(rows, 4);
        ck_assert_uint_eq(breaks, 1);
}
END_TEST

START_TEST(test_file_none)
{
        int ret = 0;

        reader->quotes = QUOTE_NONE;
        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "123");
        _field_check(&record->fields[1], "456");
        _field_check(&record->fields[2], "789");

        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "\"abc\"");
        _field_check(&record->fields[1], "\"d");
        _field_check(&record->fields[2], "ef\"");
        _field_check(&record->fields[3], "\"ghi\"");

        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 2);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "\"de");

        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 2);
        _field_check(&record->fields[0], "f\"");
        _field_check(&record->fields[1], "ghi");

        ret = csv_get_record(reader, record);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "\"de\"\"f\"");
        _field_check(&record->fields[2], "ghi");

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, EOF);

        unsigned rows = csv_reader_row_count(reader);
        unsigned breaks = csv_reader_embedded_breaks(reader);

        ck_assert_uint_eq(rows, 5);
        ck_assert_uint_eq(breaks, 0);
}
END_TEST

START_TEST(test_multiple_eol)
{
        int ret = 0;

        reader->failsafe_mode = 1;

        csv_reader_open(reader, "test_multi_eol.txt");
        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, CSV_GOOD);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "123");
        _field_check(&record->fields[1], "4\n5\n6");
        _field_check(&record->fields[2], "789");

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, EOF);

        unsigned rows = csv_reader_row_count(reader);
        unsigned breaks = csv_reader_embedded_breaks(reader);

        ck_assert_uint_eq(rows, 1);
        ck_assert_uint_eq(breaks, 2);
}


START_TEST(test_realloc_append)
{
        int ret = 0;

        reader->failsafe_mode = 1;

        csv_reader_open(reader, "test_realloc_append.txt");
        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, CSV_GOOD);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "aaa");
        _field_check(&record->fields[1], "1234567890\n1234567890123456789012345678901234567890123456789012345678901234567890\n1234567890123456789012345678901234567890123456789012345678901234567890");
        _field_check(&record->fields[2], "ccc");

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, CSV_GOOD);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "def");
        _field_check(&record->fields[2], "ghi");

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, EOF);

        unsigned rows = csv_reader_row_count(reader);
        unsigned breaks = csv_reader_embedded_breaks(reader);

        ck_assert_uint_eq(rows, 2);
        ck_assert_uint_eq(breaks, 2);
}


START_TEST(test_fs_eof)
{
        int ret = 0;

        reader->failsafe_mode = 1;

        csv_reader_open(reader, "test_fs_eof.txt");
        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, CSV_RESET);

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, CSV_RESET);

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, CSV_GOOD);
        _field_check(&record->fields[0], "\"");

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, EOF);
}

START_TEST(test_fs_max)
{
        int ret = 0;

        reader->failsafe_mode = 1;

        csv_reader_open(reader, "test_fs_max.txt");
        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, CSV_RESET);

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, CSV_RESET);

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, CSV_GOOD);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "\"def");

        while ((ret = csv_get_record(reader, record)) == CSV_GOOD);

        unsigned rows = csv_reader_row_count(reader);
        ck_assert_uint_eq(rows, 41);
}

START_TEST(test_fs_weak)
{
        int ret = 0;

        reader->failsafe_mode = 1;

        csv_reader_open(reader, "test_fs_weak.txt");
        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, CSV_RESET);

        ret = csv_get_record(reader, record);
        ck_assert_int_eq(ret, CSV_GOOD);
        ck_assert_uint_eq(record->size, 3);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\"f");
        _field_check(&record->fields[2], "ghi");
}



Suite* mmap_suite(void)
{
        Suite* s;
        s = suite_create("Reader");

        TCase* tc_file_rfc = tcase_create("rfc4180");
        tcase_add_checked_fixture(tc_file_rfc, file_setup, parse_teardown);
        tcase_add_test(tc_file_rfc, test_file_rfc);
        suite_add_tcase(s, tc_file_rfc);

        TCase* tc_file_weak = tcase_create("weak");
        tcase_add_checked_fixture(tc_file_weak, file_setup, parse_teardown);
        tcase_add_test(tc_file_weak, test_file_weak);
        suite_add_tcase(s, tc_file_weak);

        TCase* tc_file_none = tcase_create("none");
        tcase_add_checked_fixture(tc_file_none, file_setup, parse_teardown);
        tcase_add_test(tc_file_none, test_file_none);
        suite_add_tcase(s, tc_file_none);

        TCase* tc_multi_eol = tcase_create("multi_eol");
        tcase_add_checked_fixture(tc_multi_eol, parse_setup, parse_teardown);
        tcase_add_test(tc_multi_eol, test_multiple_eol);
        suite_add_tcase(s, tc_multi_eol);

        TCase* tc_realloc_append = tcase_create("realloc_append");
        tcase_add_checked_fixture(tc_realloc_append, parse_setup, parse_teardown);
        tcase_add_test(tc_realloc_append, test_realloc_append);
        suite_add_tcase(s, tc_realloc_append);

        TCase* tc_failsafe_eof = tcase_create("failsafe_eof");
        tcase_add_checked_fixture(tc_failsafe_eof, parse_setup, parse_teardown);
        tcase_add_test(tc_failsafe_eof, test_fs_eof);
        suite_add_tcase(s, tc_failsafe_eof);

        TCase* tc_failsafe_max = tcase_create("failsafe_max");
        tcase_add_checked_fixture(tc_failsafe_max, parse_setup, parse_teardown);
        tcase_add_test(tc_failsafe_max, test_fs_max);
        suite_add_tcase(s, tc_failsafe_max);

        TCase* tc_failsafe_weak = tcase_create("failsafe_weak");
        tcase_add_checked_fixture(tc_failsafe_weak, parse_setup, parse_teardown);
        tcase_add_test(tc_failsafe_weak, test_fs_weak);
        suite_add_tcase(s, tc_failsafe_weak);

        //TCase* tc_weak_trailing = tcase_create("failsafe_weak");
        //tcase_add_checked_fixture(tc_weak_trailing, parse_setup, parse_teardown);
        //tcase_add_test(tc_weak_trailing, test_weak_trailing);
        //suite_add_tcase(s, tc_weak_trailing);

        return s;
}

int main(void)
{
        int number_failed;
        Suite* s = mmap_suite();
        SRunner* sr = srunner_create(s);
        srunner_set_fork_status (sr, CK_NOFORK);

        srunner_run_all(sr, CK_VERBOSE);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

