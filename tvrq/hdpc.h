#ifndef HDPC_H
#define HDPC_H

#include "parameters.h"
#include "m256v.h"

void hdpc_generate_mat_specexact(m256v* H, const parameters* P);
void hdpc_generate_mat_faster(m256v* H, const parameters* P);

#define hdpc_generate_mat hdpc_generate_mat_faster

#endif /* HDPC_H */
