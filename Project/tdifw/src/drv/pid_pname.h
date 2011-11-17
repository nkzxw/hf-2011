// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: pid_pname.h,v 1.1 2002/12/03 12:14:42 dev Exp $

#ifndef _pid_pname_h_
#define _pid_pname_h_

void		pid_pname_init(void);
void		pid_pname_free(void);

BOOLEAN		pid_pname_resolve(ULONG pid, char *buf, int buf_size);

NTSTATUS	pid_pname_set_event(ULONG pid, KEVENT *event);

NTSTATUS	pid_pname_set(ULONG pid, const char *pname, int context);
int			pid_pname_get_context(ULONG pid);

#endif
