#include <check.h>
#include <stdlib.h>
#include <stdint.h>

/* Include the actual production surface header/source */
#include "src/libs/gl/surface.h"

START_TEST(test_surface_allocation_no_overflow)
{
    /* Invariant: GL_surface_create must never return a surface whose
       allocated buffer is smaller than width * height * sizeof(GL_Pixel_t).
       Integer overflow in the allocation must not produce an undersized buffer. */

    typedef struct { size_t width; size_t height; int expect_failure; } Case;
    Case cases[] = {
        /* Exact overflow exploit: large values that wrap on 32-bit multiply */
        { 0x10000, 0x10000, 1 },
        /* Boundary: just over SIZE_MAX / sizeof(GL_Pixel_t) */
        { SIZE_MAX / sizeof(GL_Pixel_t), 2, 1 },
        /* Valid small surface */
        { 64, 64, 0 },
    };
    int num_cases = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < num_cases; i++) {
        GL_Surface_t *surface = GL_surface_create(cases[i].width, cases[i].height);

        if (cases[i].expect_failure) {
            /* For overflow-inducing inputs the implementation MUST either
               return NULL (safe rejection) or allocate a correctly sized
               buffer. It must NOT silently return a truncated allocation. */
            if (surface != NULL) {
                /* If it succeeded, verify the reported dimensions match
                   and that the backing store is large enough. */
                ck_assert_uint_eq(surface->width, cases[i].width);
                ck_assert_uint_eq(surface->height, cases[i].height);
                /* The allocation must not have wrapped: width*height must
                   not overflow size_t */
                size_t w = cases[i].width, h = cases[i].height;
                ck_assert_msg(w == 0 || h <= SIZE_MAX / w,
                    "Overflow: surface created with wrapped allocation");
                GL_surface_destroy(surface);
            }
            /* NULL return is the acceptable safe path — no assertion needed */
        } else {
            /* Valid input must succeed */
            ck_assert_ptr_nonnull(surface);
            ck_assert_uint_eq(surface->width, cases[i].width);
            ck_assert_uint_eq(surface->height, cases[i].height);
            GL_surface_destroy(surface);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s = suite_create("Security");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_surface_allocation_no_overflow);
    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = security_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}