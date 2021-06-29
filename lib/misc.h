#ifndef MISC_H
#define MISC_H

#include <stdlib.h>
#include <stdbool.h>

#define BUFFER_FACTOR 128

/* If the condition returns CSV_FAIL, then we
 * want to pass that up. A program relying on
 * user input has to check everything...
 */
#define try_(condition_)                 \
	({                               \
		int ret_ = (condition_); \
		if (ret_ == CSV_FAIL) {  \
			return CSV_FAIL; \
		}                        \
		ret_;                    \
	})

#define fail_if_(condition_)             \
	({                               \
		int ret_ = (condition_); \
		if (ret_) {              \
			return CSV_FAIL; \
		}                        \
		ret_;                    \
	})

/** **/
void increase_buffer(char**, size_t*);
void increase_buffer_to(char**, size_t*, size_t);

#endif
