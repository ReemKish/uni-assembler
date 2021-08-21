/* ===== scan.h ===========================================
 * Header file for "scan.c".
 * Exposes the main functions: write_memory_image & write_instruction_image.
 */
#ifndef SCAN_H
#define SCAN_H

#include <stdio.h>
#include "types.h"


void write_memory_image(Statement_t *statements);
int write_instruction_image(Statement_t *statements);


#endif
