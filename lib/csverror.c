#include "misc.h"
#include "csverror.h"
#include "util.h"

/**
 * Error Handlers
 */

static node* _err_str_head = NULL;

void err_remove(node* node)
{
	node_remove(&_err_str_head, node);
}

node* err_push(const char* err)
{
	return node_enqueue(&_err_str_head, (void*)err);
}

void err_printall()
{
	while (_err_str_head) {
		const char* data = node_dequeue(&_err_str_head);
		fprintf(stderr, "%s\n", data);
		free_(data);
	}
}
