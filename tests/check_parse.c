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
	csv_record_free(record);
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
        csv_reader_open(reader, "basic.csv");
}

START_TEST(test_parse_rfc)
{
        int ret = 0;
        ret = csv_parse(reader, record, "123,456,789,,");
        ck_assert_uint_eq(record->size, 5);
        _field_check(&record->fields[0], "123");
        _field_check(&record->fields[1], "456");
        _field_check(&record->fields[2], "789");
        _field_check(&record->fields[3], "");
        _field_check(&record->fields[4], "");

        ret = csv_parse(reader, record, "\"abc\",\"d,ef\",\"ghi\",\"\"");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "d,ef");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        ret = csv_parse(reader, record, "abc,\"de\nf\",ghi,");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\nf");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        ret = csv_parse(reader, record, "abc,\"de\"\"f\",ghi,");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\"f");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        unsigned rows = csv_reader_row_count(reader);
        //unsigned breaks = csv_reader_embedded_breaks(reader);

        ck_assert_uint_eq(rows, 4);
        //ck_assert_uint_eq(breaks, 1);

}
END_TEST

START_TEST(test_parse_weak)
{
        int ret = 0;
        reader->quotes = QUOTE_WEAK;
        ret = csv_parse(reader, record, "123,456,789,,");
        ck_assert_uint_eq(record->size, 5);
        _field_check(&record->fields[0], "123");
        _field_check(&record->fields[1], "456");
        _field_check(&record->fields[2], "789");
        _field_check(&record->fields[3], "");
        _field_check(&record->fields[4], "");

        ret = csv_parse(reader, record, "\"abc\",\"d,ef\",\"ghi\",\"\"");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "d,ef");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        ret = csv_parse(reader, record, "abc,\"de\nf\",ghi,");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\nf");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        ret = csv_parse(reader, record, "abc,\"de\"\"f\",ghi,");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\"\"f");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        unsigned rows = csv_reader_row_count(reader);
        //unsigned breaks = csv_reader_embedded_breaks(reader);

        ck_assert_uint_eq(rows, 4);
        //ck_assert_uint_eq(breaks, 1);
}
END_TEST

START_TEST(test_parse_none)
{
        int ret = 0;
        reader->quotes = QUOTE_NONE;
        ret = csv_parse(reader, record, "123,456,789,,");
        ck_assert_uint_eq(record->size, 5);
        _field_check(&record->fields[0], "123");
        _field_check(&record->fields[1], "456");
        _field_check(&record->fields[2], "789");
        _field_check(&record->fields[3], "");
        _field_check(&record->fields[4], "");

        ret = csv_parse(reader, record, "\"abc\",\"d,ef\",\"ghi\",\"\"");
        ck_assert_uint_eq(record->size, 5);
        _field_check(&record->fields[0], "\"abc\"");
        _field_check(&record->fields[1], "\"d");
        _field_check(&record->fields[2], "ef\"");
        _field_check(&record->fields[3], "\"ghi\"");
        _field_check(&record->fields[4], "\"\"");

        ret = csv_parse(reader, record, "abc,\"de\nf\",ghi,");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "\"de\nf\"");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        ret = csv_parse(reader, record, "abc,\"de\"\"f\",ghi,");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "\"de\"\"f\"");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        unsigned rows = csv_reader_row_count(reader);
        //unsigned breaks = csv_reader_embedded_breaks(reader);

        ck_assert_uint_eq(rows, 4);
        //ck_assert_uint_eq(breaks, 1);
}
END_TEST

START_TEST(test_parse_ldrfc)
{
        int ret = 0;
        csv_reader_set_delim(reader, "~^_");
        ret = csv_parse(reader, record, "123~^_456~^_789~^_~^_");
        ck_assert_uint_eq(record->size, 5);
        _field_check(&record->fields[0], "123");
        _field_check(&record->fields[1], "456");
        _field_check(&record->fields[2], "789");
        _field_check(&record->fields[3], "");
        _field_check(&record->fields[4], "");

        ret = csv_parse(reader, record, "\"abc\"~^_\"d~^_ef\"~^_\"ghi\"~^_\"\"");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "d~^_ef");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        ret = csv_parse(reader, record, "abc~^_\"de\nf\"~^_ghi~^_");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\nf");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        ret = csv_parse(reader, record, "abc~^_\"de\"\"f\"~^_ghi~^_");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\"f");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        unsigned rows = csv_reader_row_count(reader);
        //unsigned breaks = csv_reader_embedded_breaks(reader);

        ck_assert_uint_eq(rows, 4);
        //ck_assert_uint_eq(breaks, 1);
}
END_TEST

START_TEST(test_parse_ldweak)
{
        int ret = 0;
        reader->quotes = QUOTE_WEAK;
        csv_reader_set_delim(reader, "~^_");
        ret = csv_parse(reader, record, "123~^_456~^_789~^_~^_");
        ck_assert_uint_eq(record->size, 5);
        _field_check(&record->fields[0], "123");
        _field_check(&record->fields[1], "456");
        _field_check(&record->fields[2], "789");
        _field_check(&record->fields[3], "");
        _field_check(&record->fields[4], "");

        ret = csv_parse(reader, record, "\"abc\"~^_\"d~^_ef\"~^_\"ghi\"~^_\"\"");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "d~^_ef");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        ret = csv_parse(reader, record, "abc~^_\"de\nf\"~^_ghi~^_");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\nf");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        ret = csv_parse(reader, record, "abc~^_\"de\"\"f\"~^_ghi~^_");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "de\"\"f");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        unsigned rows = csv_reader_row_count(reader);
        //unsigned breaks = csv_reader_embedded_breaks(reader);

        ck_assert_uint_eq(rows, 4);
        //ck_assert_uint_eq(breaks, 1);
}
END_TEST

START_TEST(test_parse_ldnone)
{
        int ret = 0;
        reader->quotes = QUOTE_NONE;
        csv_reader_set_delim(reader,  "~^_");
        ret = csv_parse(reader, record, "123~^_456~^_789~^_~^_");
        ck_assert_uint_eq(record->size, 5);
        _field_check(&record->fields[0], "123");
        _field_check(&record->fields[1], "456");
        _field_check(&record->fields[2], "789");
        _field_check(&record->fields[3], "");
        _field_check(&record->fields[4], "");

        ret = csv_parse(reader, record, "\"abc\"~^_\"d~^_ef\"~^_\"ghi\"~^_\"\"");
        ck_assert_uint_eq(record->size, 5);
        _field_check(&record->fields[0], "\"abc\"");
        _field_check(&record->fields[1], "\"d");
        _field_check(&record->fields[2], "ef\"");
        _field_check(&record->fields[3], "\"ghi\"");
        _field_check(&record->fields[4], "\"\"");

        ret = csv_parse(reader, record, "abc~^_\"de\nf\"~^_ghi~^_");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "\"de\nf\"");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        ret = csv_parse(reader, record, "abc~^_\"de\"\"f\"~^_ghi~^_");
        ck_assert_uint_eq(record->size, 4);
        _field_check(&record->fields[0], "abc");
        _field_check(&record->fields[1], "\"de\"\"f\"");
        _field_check(&record->fields[2], "ghi");
        _field_check(&record->fields[3], "");

        unsigned rows = csv_reader_row_count(reader);
        //unsigned breaks = csv_reader_embedded_breaks(reader);

        ck_assert_uint_eq(rows, 4);
        //ck_assert_uint_eq(breaks, 1);
}
END_TEST

Suite* parse_suite(void)
{
        Suite* s;
        s = suite_create("Reader");

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

        return s;
}

int main(void)
{
        int number_failed;
        Suite* s = parse_suite();
        SRunner* sr = srunner_create(s);
        srunner_set_fork_status (sr, CK_NOFORK);

        srunner_run_all(sr, CK_VERBOSE);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

