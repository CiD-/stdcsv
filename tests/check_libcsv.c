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

void file_setup(void)
{
        parse_setup();
        csv_reader_open(reader, "basic.csv");
}

START_TEST(test_parse_rfc)
{
        record = csv_parse(reader, "123,456,789");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "123");
        ck_assert_str_eq(record->fields[1], "456");
        ck_assert_str_eq(record->fields[2], "789");

        record = csv_parse(reader, "\"abc\",\"d,ef\",\"ghi\"");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "d,ef");
        ck_assert_str_eq(record->fields[2], "ghi");
        
        record = csv_parse(reader, "abc,\"de\nf\",ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\nf");
        ck_assert_str_eq(record->fields[2], "ghi");

        record = csv_parse(reader, "abc,\"de\"\"f\",ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\"f");
        ck_assert_str_eq(record->fields[2], "ghi");

}

START_TEST(test_parse_weak)
{
        reader->quotes = QUOTE_WEAK;
        record = csv_parse(reader, "123,456,789");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "123");
        ck_assert_str_eq(record->fields[1], "456");
        ck_assert_str_eq(record->fields[2], "789");

        record = csv_parse(reader, "\"abc\",\"d,ef\",\"ghi\"");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "d,ef");
        ck_assert_str_eq(record->fields[2], "ghi");
        
        record = csv_parse(reader, "abc,\"de\nf\",ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\nf");
        ck_assert_str_eq(record->fields[2], "ghi");

        record = csv_parse(reader, "abc,\"de\"\"f\",ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\"\"f");
        ck_assert_str_eq(record->fields[2], "ghi");

}

START_TEST(test_parse_none)
{
        reader->quotes = QUOTE_NONE;
        record = csv_parse(reader, "123,456,789");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "123");
        ck_assert_str_eq(record->fields[1], "456");
        ck_assert_str_eq(record->fields[2], "789");

        record = csv_parse(reader, "\"abc\",\"d,ef\",\"ghi\"");
        ck_assert_int_eq(record->size, 4);
        ck_assert_str_eq(record->fields[0], "\"abc\"");
        ck_assert_str_eq(record->fields[1], "\"d");
        ck_assert_str_eq(record->fields[2], "ef\"");
        ck_assert_str_eq(record->fields[3], "\"ghi\"");
        
        record = csv_parse(reader, "abc,\"de\nf\",ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "\"de\nf\"");
        ck_assert_str_eq(record->fields[2], "ghi");

        record = csv_parse(reader, "abc,\"de\"\"f\",ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "\"de\"\"f\"");
        ck_assert_str_eq(record->fields[2], "ghi");
}

START_TEST(test_parse_ldrfc)
{
        strcpy(reader->delimiter, "~^_");
        record = csv_parse(reader, "123~^_456~^_789");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "123");
        ck_assert_str_eq(record->fields[1], "456");
        ck_assert_str_eq(record->fields[2], "789");

        record = csv_parse(reader, "\"abc\"~^_\"d~^_ef\"~^_\"ghi\"");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "d~^_ef");
        ck_assert_str_eq(record->fields[2], "ghi");
        
        record = csv_parse(reader, "abc~^_\"de\nf\"~^_ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\nf");
        ck_assert_str_eq(record->fields[2], "ghi");

        record = csv_parse(reader, "abc~^_\"de\"\"f\"~^_ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\"f");
        ck_assert_str_eq(record->fields[2], "ghi");

}

START_TEST(test_parse_ldweak)
{
        reader->quotes = QUOTE_WEAK;
        strcpy(reader->delimiter, "~^_");
        record = csv_parse(reader, "123~^_456~^_789");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "123");
        ck_assert_str_eq(record->fields[1], "456");
        ck_assert_str_eq(record->fields[2], "789");

        record = csv_parse(reader, "\"abc\"~^_\"d~^_ef\"~^_\"ghi\"");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "d~^_ef");
        ck_assert_str_eq(record->fields[2], "ghi");
        
        record = csv_parse(reader, "abc~^_\"de\nf\"~^_ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\nf");
        ck_assert_str_eq(record->fields[2], "ghi");

        record = csv_parse(reader, "abc~^_\"de\"\"f\"~^_ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\"\"f");
        ck_assert_str_eq(record->fields[2], "ghi");

}

START_TEST(test_parse_ldnone)
{
        reader->quotes = QUOTE_NONE;
        strcpy(reader->delimiter, "~^_");
        record = csv_parse(reader, "123~^_456~^_789");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "123");
        ck_assert_str_eq(record->fields[1], "456");
        ck_assert_str_eq(record->fields[2], "789");

        record = csv_parse(reader, "\"abc\"~^_\"d~^_ef\"~^_\"ghi\"");
        ck_assert_int_eq(record->size, 4);
        ck_assert_str_eq(record->fields[0], "\"abc\"");
        ck_assert_str_eq(record->fields[1], "\"d");
        ck_assert_str_eq(record->fields[2], "ef\"");
        ck_assert_str_eq(record->fields[3], "\"ghi\"");
        
        record = csv_parse(reader, "abc~^_\"de\nf\"~^_ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "\"de\nf\"");
        ck_assert_str_eq(record->fields[2], "ghi");

        record = csv_parse(reader, "abc~^_\"de\"\"f\"~^_ghi");
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "\"de\"\"f\"");
        ck_assert_str_eq(record->fields[2], "ghi");
}


START_TEST(test_file_rfc)
{
        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "123");
        ck_assert_str_eq(record->fields[1], "456");
        ck_assert_str_eq(record->fields[2], "789");

        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "d|ef");
        ck_assert_str_eq(record->fields[2], "ghi");
        
        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\nf");
        ck_assert_str_eq(record->fields[2], "ghi");

        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\"f");
        ck_assert_str_eq(record->fields[2], "ghi");

        record = csv_get_record(reader);
        ck_assert_ptr_null(record);

}

START_TEST(test_file_weak)
{
        reader->quotes = QUOTE_WEAK;
        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "123");
        ck_assert_str_eq(record->fields[1], "456");
        ck_assert_str_eq(record->fields[2], "789");

        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "d|ef");
        ck_assert_str_eq(record->fields[2], "ghi");
        
        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\nf");
        ck_assert_str_eq(record->fields[2], "ghi");

        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "de\"\"f");
        ck_assert_str_eq(record->fields[2], "ghi");

        record = csv_get_record(reader);
        ck_assert_ptr_null(record);

}

START_TEST(test_file_none)
{
        reader->quotes = QUOTE_NONE;
        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "123");
        ck_assert_str_eq(record->fields[1], "456");
        ck_assert_str_eq(record->fields[2], "789");

        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 4);
        ck_assert_str_eq(record->fields[0], "\"abc\"");
        ck_assert_str_eq(record->fields[1], "\"d");
        ck_assert_str_eq(record->fields[2], "ef\"");
        ck_assert_str_eq(record->fields[3], "\"ghi\"");
        
        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 2);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "\"de");

        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 2);
        ck_assert_str_eq(record->fields[0], "f\"");
        ck_assert_str_eq(record->fields[1], "ghi");

        record = csv_get_record(reader);
        ck_assert_int_eq(record->size, 3);
        ck_assert_str_eq(record->fields[0], "abc");
        ck_assert_str_eq(record->fields[1], "\"de\"\"f\"");
        ck_assert_str_eq(record->fields[2], "ghi");

        record = csv_get_record(reader);
        ck_assert_ptr_null(record);

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

        TCase* tc_parse_weak = tcase_create("weak");
        tcase_add_checked_fixture(tc_parse_weak, parse_setup, parse_teardown);
        tcase_add_test(tc_parse_weak, test_parse_weak);
        suite_add_tcase(s, tc_parse_weak);

        TCase* tc_parse_none = tcase_create("none");
        tcase_add_checked_fixture(tc_parse_none, parse_setup, parse_teardown);
        tcase_add_test(tc_parse_none, test_parse_none);
        suite_add_tcase(s, tc_parse_none);

        TCase* tc_parse_ldrfc = tcase_create("ld_rfc4180");
        tcase_add_checked_fixture(tc_parse_ldrfc, parse_setup, parse_teardown);
        tcase_add_test(tc_parse_ldrfc, test_parse_ldrfc);
        suite_add_tcase(s, tc_parse_ldrfc);

        TCase* tc_parse_ldweak = tcase_create("ld_weak");
        tcase_add_checked_fixture(tc_parse_ldweak, parse_setup, parse_teardown);
        tcase_add_test(tc_parse_ldweak, test_parse_ldweak);
        suite_add_tcase(s, tc_parse_ldweak);

        TCase* tc_parse_ldnone = tcase_create("ld_none");
        tcase_add_checked_fixture(tc_parse_ldnone, parse_setup, parse_teardown);
        tcase_add_test(tc_parse_ldnone, test_parse_ldnone);
        suite_add_tcase(s, tc_parse_ldnone);

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
