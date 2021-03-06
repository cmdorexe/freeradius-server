/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef _FR_EVENT_H
#define _FR_EVENT_H
/**
 * $Id$
 *
 * @file include/event.h
 * @brief A simple event queue.
 *
 * @copyright 2007  The FreeRADIUS server project
 * @copyright 2007  Alan DeKok <aland@deployingradius.com>
 */
RCSIDH(event_h, "$Id$")

#include <freeradius-devel/missing.h>
#include <stdbool.h>
#include <sys/event.h>

#ifdef __cplusplus
extern "C" {
#endif

/** An opaque file descriptor handle
 */
typedef struct fr_event_fd fr_event_fd_t;

/** An opaque event list handle
 */
typedef struct fr_event_list fr_event_list_t;

/** An opaque timer handle
 */
typedef struct fr_event_timer fr_event_timer_t;

/** An opaque PID status handle
 */
typedef struct fr_event_pid fr_event_pid_t;

/** The type of filter to install for an FD
 */
typedef enum {
	FR_EVENT_FILTER_IO,
	FR_EVENT_FILTER_VNODE
} fr_event_filter_t;

/** Called when a timer event fires
 *
 * @param[in] now	The current time.
 * @param[in] uctx	User ctx passed to #fr_event_timer_insert.
 */
typedef	void (*fr_event_cb_t)(fr_event_list_t *el, struct timeval *now, void *uctx);

/** Called after each event loop cycle
 *
 * Called before calling kqueue to put the thread in a sleeping state.
 *
 * @param[in] now	The current time.
 * @param[in] uctx	User ctx passed to #fr_event_list_alloc.
 */
typedef	int (*fr_event_status_cb_t)(void *uctx, struct timeval *now);

/** Called when an IO event occurs on a file descriptor
 *
 * @param[in] el	Event list the file descriptor was inserted into.
 * @param[in] fd	That experienced the IO event.
 * @param[in] flags	field as returned by kevent.
 * @param[in] uctx	User ctx passed to #fr_event_fd_insert.
 */
typedef void (*fr_event_fd_cb_t)(fr_event_list_t *el, int fd, int flags, void *uctx);

/** Called when an IO error event occurs on a file descriptor
 *
 * @param[in] el	Event list the file descriptor was inserted into.
 * @param[in] fd	That experienced the IO event.
 * @param[in] flags	field as returned by kevent.
 * @param[in] fd_errno	File descriptor error.
 * @param[in] uctx	User ctx passed to #fr_event_fd_insert.
 */
typedef void (*fr_event_error_cb_t)(fr_event_list_t *el, int fd, int flags, int fd_errno, void *uctx);

/** Called when a child process has exited
 *
 * @param[in] el	Event list
 * @param[in] pid	That exited
 * @param[in] status	exit status
 * @param[in] uctx	User ctx passed to #fr_event_fd_insert.
 */
typedef void (*fr_event_pid_cb_t)(fr_event_list_t *el, pid_t pid, int status, void *uctx);

/** Called when a user kevent occurs
 *
 * @param[in] kq	that received the user kevent.
 * @param[in] kev	The kevent.
 * @param[in] uctx	User ctx passed to #fr_event_user_insert.
 */
typedef void (*fr_event_user_handler_t)(int kq, struct kevent const *kev, void *uctx);

/** Callbacks for the #FR_EVENT_FILTER_IO filter
 */
typedef struct {
	fr_event_fd_cb_t	read;			//!< Callback for when data is available.
	fr_event_fd_cb_t	write;			//!< Callback for when we can write data.
} fr_event_io_func_t;

/** Callbacks for the #FR_EVENT_FILTER_VNODE filter
 */
typedef struct {
	fr_event_fd_cb_t	delete;			//!< The file was deleted.
	fr_event_fd_cb_t	write;			//!< The file was written to.
	fr_event_fd_cb_t	extend;			//!< Additional files were added to a directory.
	fr_event_fd_cb_t	attrib;			//!< File attributes changed.
	fr_event_fd_cb_t	link;			//!< The link count on the file changed.
	fr_event_fd_cb_t	rename;			//!< The file was renamed.
#ifdef NOTE_REVOKE
	fr_event_fd_cb_t	revoke;			//!< Volume containing the file was unmounted or
							///< access was revoked with revoke().
#endif
#ifdef NOTE_FUNLOCK
	fr_event_fd_cb_t	funlock;		//!< The file was unlocked.
#endif
} fr_event_vnode_func_t;

int		fr_event_list_num_fds(fr_event_list_t *el);
int		fr_event_list_num_elements(fr_event_list_t *el);
int		fr_event_list_kq(fr_event_list_t *el);
int		fr_event_list_time(struct timeval *when, fr_event_list_t *el);

int		fr_event_fd_delete(fr_event_list_t *el, int fd);

int		fr_event_fd_read_pause(fr_event_list_t *el, int fd);
int		fr_event_fd_read_continue(fr_event_list_t *el, int fd);

int		fr_event_filter_insert(TALLOC_CTX *ctx, fr_event_list_t *el, int fd,
				       fr_event_filter_t filter,
				       void *funcs,
				       fr_event_error_cb_t error,
				       void *uctx);

int		fr_event_fd_insert(TALLOC_CTX *ctx, fr_event_list_t *el, int fd,
				   fr_event_fd_cb_t read_fn,
				   fr_event_fd_cb_t write_fn,
				   fr_event_error_cb_t error,
				   void *uctx);

int		fr_event_pid_wait(TALLOC_CTX *ctx, fr_event_list_t *el, fr_event_pid_t const **ev_p,
				  pid_t pid, fr_event_pid_cb_t wait_fn, void *uctx) CC_HINT(nonnull(2,5));

int		fr_event_timer_insert(TALLOC_CTX *ctx, fr_event_list_t *el, fr_event_timer_t const **ev,
				      struct timeval *when, fr_event_cb_t callback, void const *uctx);
int		fr_event_timer_delete(fr_event_list_t *el, fr_event_timer_t const **ev);
int		fr_event_timer_run(fr_event_list_t *el, struct timeval *when);

uintptr_t      	fr_event_user_insert(fr_event_list_t *el, fr_event_user_handler_t user, void *uctx) CC_HINT(nonnull(1,2));
int		fr_event_user_delete(fr_event_list_t *el, fr_event_user_handler_t user, void *uctx) CC_HINT(nonnull(1,2));

int		fr_event_pre_insert(fr_event_list_t *el, fr_event_status_cb_t callback, void *uctx) CC_HINT(nonnull(1,2));
int		fr_event_pre_delete(fr_event_list_t *el, fr_event_status_cb_t callback, void *uctx) CC_HINT(nonnull(1,2));

int		fr_event_post_insert(fr_event_list_t *el, fr_event_cb_t callback, void *uctx) CC_HINT(nonnull(1,2));
int		fr_event_post_delete(fr_event_list_t *el, fr_event_cb_t callback, void *uctx) CC_HINT(nonnull(1,2));

int		fr_event_corral(fr_event_list_t *el, bool wait);
void		fr_event_service(fr_event_list_t *el);

void		fr_event_loop_exit(fr_event_list_t *el, int code);
bool		fr_event_loop_exiting(fr_event_list_t *el);
int		fr_event_loop(fr_event_list_t *el);

fr_event_list_t	*fr_event_list_alloc(TALLOC_CTX *ctx, fr_event_status_cb_t status, void *status_ctx);

#ifdef __cplusplus
}
#endif
#endif /* _FR_EVENT_H */
