#ifndef CSV_ERROR_H
#define CSV_ERROR_H

#include "charnode.h"

void err_remove(struct charnode* node);
struct charnode* err_push(const char*);
void err_printall();

#endif /* CSV_ERROR_H */
