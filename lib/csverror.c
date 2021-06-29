#include "misc.h"
#include "csverror.h"
#include "util.h"

/**
 * Error Handlers
 */

static queue* _err_str_head = NULL;

void err_remove(queue* node)
{
	queue_remove(&_err_str_head, node);
}

queue* err_push(const char* err)
{
	return queue_enqueue(&_err_str_head, (void*)err);
}

void err_printall()
{
	while (_err_str_head) {
		const char* data = queue_dequeue(&_err_str_head);
		fprintf(stderr, "%s\n", data);
		free_(data);
	}
}
