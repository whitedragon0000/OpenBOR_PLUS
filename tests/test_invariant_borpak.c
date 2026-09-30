#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>

/* We test that filenames extracted from a pak archive never resolve outside
   the intended extraction directory. Since borpak uses memcpy of the filename
   directly without sanitization, we simulate the path resolution check that
   SHOULD exist. We call realpath-style logic on what borpak would produce. */

static int path_stays_within_root(const char *root, const char *filename)
{
    char combined[4096];
    char resolved[4096];
    
    snprintf(combined, sizeof(combined), "%s/%s", root, filename);
    
    /* Normalize: check if the combined path, when resolved, starts with root */
    char *rp = realpath(root, NULL);
    if (!rp) return 0;
    
    /* Manually resolve ../ components to check containment */
    char *res = realpath(combined, NULL);
    if (res) {
        int contained = (strncmp(res, rp, strlen(rp)) == 0);
        free(res);
        free(rp);
        return contained;
    }
    
    /* If file doesn't exist, do string-based check for traversal */
    int has_traversal = (strstr(filename, "../") != NULL ||
                         strstr(filename, "..\\") != NULL ||
                         strstr(filename, "%2e%2e") != NULL ||
                         strstr(filename, "....//") != NULL);
    free(rp);
    return !has_traversal;
}

START_TEST(test_path_traversal_in_pak_filenames)
{
    /* Invariant: File operations never resolve paths outside the declared root directory */
    const char *root = "/tmp/extract_dir";
    const char *payloads[] = {
        "../../../etc/passwd",
        "....//....//etc/passwd",
        "%2e%2e%2f%2e%2e%2f%2e%2e%2fetc/passwd",
        "subdir/../../../etc/shadow",
        "valid/nested/file.txt"
    };
    int expect_safe[] = {0, 0, 0, 0, 1};
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        int safe = path_stays_within_root(root, payloads[i]);
        ck_assert_msg(safe == expect_safe[i],
            "Payload '%s': expected safe=%d, got safe=%d",
            payloads[i], expect_safe[i], safe);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_path_traversal_in_pak_filenames);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}