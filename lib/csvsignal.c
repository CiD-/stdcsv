#include "csvsignal.h"
#include <string.h>
#include <stdio.h>
#include "util/stringy.h"
#include "util/util.h"
#include "misc.h"

/**
 * Signal Handlers
 * sigset_t and sa_flags only set to shut up valgrind
 */
static struct sigaction act;
static sigset_t vg_shutup = { {0} };
static int _signals_ready = false;
static queue* _tmp_file_head = NULL;

void init_sig()
{
	if (_signals_ready)
		return;
	/** Attach signal handlers **/
	act.sa_mask = vg_shutup;
	act.sa_flags = 0;
	act.sa_handler = cleanexit;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGHUP, &act, NULL);

	_signals_ready = true;
}

void cleanexit()
{
	tmp_removeall();
	exit(EXIT_FAILURE);
}

queue* tmp_push(void* tmp_file)
{
	return queue_enqueue(&_tmp_file_head, tmp_file);
}

void tmp_remove_node(queue* node)
{
	string* tmp = queue_remove(&_tmp_file_head, node);
	delete_ (string, tmp);
}

void tmp_remove_file(const char* tmp_file)
{
	if (remove(tmp_file))
		perror(tmp_file);
}

void tmp_removeall()
{
	while (_tmp_file_head) {
		string* tmp = queue_dequeue(&_tmp_file_head);
		tmp_remove_file(string_c_str(tmp));
		delete_ (string, tmp);
	}
}

