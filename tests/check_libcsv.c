#include <check.h>
#include <stdlib.h>
#include "../lib/include/csv.h"

char* buf = NULL;
size_t buflen = 0;
size_t linelen = 0;
FILE* _file = NULL;

void sgl_setup(void)
{
        _file = fopen("tests/test1.txt", "r");
}

void sgl_teardown(void)
{
        free(buf);
        fclose(_file);
}

START_TEST(test_safegetline1)
{
        int ret = sgetline(_file, &buf, &buflen, &linelen);
        ck_assert_ptr_nonnull(buf);
        ck_assert_str_eq(buf, "aaa,bbb,ccc");
        ck_assert_int_eq(ret, 0);
        ck_assert_uint_eq(linelen, 11);
        ck_assert_uint_eq(buflen, BUFFER_FACTOR);

        ret = sgetline(_file, &buf, &buflen, &linelen);
        ck_assert_int_eq(ret, 0);
        ck_assert_uint_eq(linelen, 0);
        ck_assert_uint_eq(buflen, BUFFER_FACTOR);

        ret = sgetline(_file, &buf, &buflen, &linelen);
        ck_assert_int_eq(ret, EOF);

}

//START_TEST(test_rfc1)
//{
//        rec = csv_parse(rfcRead, "aaa,bbb,ccc");
//
//        ck_assert_str_eq(rec->fields[0], "aaa");
//        ck_assert_str_eq(rec->fields[0], "bbb");
//        ck_assert_str_eq(rec->fields[0], "ccc");
//}
//END_TEST
//
//START_TEST(test_rfc2)
//{
//        rec = csv_parse(rfcRead, "\"aaa\",\"bbb\",\"ccc\"");
//
//        ck_assert_str_eq(rec->fields[0], "aaa");
//        ck_assert_str_eq(rec->fields[0], "bbb");
//        ck_assert_str_eq(rec->fields[0], "ccc");
//}
//END_TEST

Suite* csv_reader_suite(void)
{
        Suite* s;
        s = suite_create("Reader");

        TCase* tc_sgl = tcase_create("safegetline");
        tcase_add_checked_fixture(tc_sgl, sgl_setup, sgl_teardown);
        tcase_add_test(tc_sgl, test_safegetline1);
        suite_add_tcase(s, tc_sgl);

        /** Tests are examples from https://tools.ietf.org/html/rfc4180 **/
        //TCase* tc_rfc = tcase_create("RFC");
        //tcase_add_test(tc_rfc, test_rfc1);
        //tcase_add_test(tc_rfc, test_rfc2);
        //suite_add_tcase(s, tc_rfc);

        return s;
}

int main(void)
{
        int number_failed;
        Suite* s = csv_reader_suite();
        SRunner* sr = srunner_create(s);
        //srunner_set_fork_status (sr, CK_NOFORK);

        srunner_run_all(sr, CK_VERBOSE);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

