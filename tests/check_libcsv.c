#include <check.h>
#include <stdlib.h>
#include <../lib/include/csv.h>

struct csv_reader* rfcRead = NULL;
struct csv_record* rec = NULL;
char checkStr[128] = "";

void setup(void)
{
        rfcRead = csv_reader_new();
}

void teardown(void)
{
        csv_reader_free(rfcRead);
}

START_TEST(test_rfc1)
{
        rec = csv_parse(rfcRead, "aaa,bbb,ccc");

        csv_get_string(&rec->fields[0], checkStr);
        ck_assert_str_eq(checkStr, "aaa");
        csv_get_string(&rec->fields[1], checkStr);
        ck_assert_str_eq(checkStr, "bbb");
        csv_get_string(&rec->fields[2], checkStr);
        ck_assert_str_eq(checkStr, "ccc");
}
END_TEST

START_TEST(test_rfc2)
{
        char orgString[100] = "\"aaa\",\"bbb\",\"ccc\"";
        rec = csv_parse(rfcRead, orgString);

        csv_get_string(&rec->fields[0], checkStr);
        ck_assert_str_eq(checkStr, "aaa");
        csv_get_string(&rec->fields[1], checkStr);
        ck_assert_str_eq(checkStr, "bbb");
        csv_get_string(&rec->fields[2], checkStr);
        ck_assert_str_eq(checkStr, "ccc");
}
END_TEST

Suite* csv_reader_suite(void)
{
        Suite* s;
        s = suite_create("Reader");
        /** Tests are examples from https://tools.ietf.org/html/rfc4180 **/
        TCase* tc_rfc = tcase_create("RFC");
        tcase_add_checked_fixture(tc_rfc, setup, teardown);
        tcase_add_test(tc_rfc, test_rfc1);
        tcase_add_test(tc_rfc, test_rfc2);
        suite_add_tcase(s, tc_rfc);

        return s;
}

int main(void)
{
        int number_failed;
        Suite* s = csv_reader_suite();
        SRunner* sr = srunner_create(s);

        srunner_run_all(sr, CK_VERBOSE);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

