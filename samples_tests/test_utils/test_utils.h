#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compute a set of random, distinct ESIs. */
void get_random_esis(int nESIs, uint32_t* ESIs);

/* Compute the ESIs, taking losses into account.
 *
 * Sect B.1.1 of Amin & Mike's monograph.
 */
void get_esis_after_loss(int nESIs, uint32_t* ESIs, float loss);

#ifdef __cplusplus
}
#endif

#endif // TEST_UTILS_H
