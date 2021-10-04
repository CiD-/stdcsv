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
static sigset_t vg_shutup = {{0}};
static int _signals_ready = false;
static node* _tmp_file_head = NULL;

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

void cleanexit(int signo)
{
	tmp_removeall();
	if (signo != -1) {
		exit(EXIT_FAILURE);
	}
}

node* tmp_push(void* tmp_file)
{
	return node_enqueue(&_tmp_file_head, tmp_file);
}

void tmp_remove_node(node* node)
{
	node_remove(&_tmp_file_head, node);
	//string* tmp = node_remove(&_tmp_file_head, node);
	//string_clear(tmp);
}

void tmp_remove_file(const char* tmp_file)
{
	if (remove(tmp_file))
		perror(tmp_file);
}

void tmp_removeall()
{
	while (_tmp_file_head) {
		string* tmp = node_dequeue(&_tmp_file_head);
		tmp_remove_file(string_c_str(tmp));
		//delete_(string, tmp);
	}
}
