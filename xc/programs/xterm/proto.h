/* $XFree86: xc/programs/xterm/proto.h,v 3.0.4.1 1998/10/20 20:51:50 hohndel Exp $ */

#ifndef included_proto_h
#define included_proto_h

#ifdef HAVE_CONFIG_H
#include <xtermcfg.h>
#endif

#define PROTO_XT_ACTIONS_ARGS \
	(Widget w, XEvent *event, String *params, Cardinal *num_params)

#define PROTO_XT_CALLBACK_ARGS \
	(Widget gw, XtPointer closure, XtPointer data)

#define PROTO_XT_CVT_SELECT_ARGS \
	(Widget w, Atom *selection, Atom *target, Atom *type, XtPointer *value, unsigned long *length, int *format)

#define PROTO_XT_EV_HANDLER_ARGS \
	(Widget w, XtPointer closure, XEvent *event, Boolean *cont)

#define PROTO_XT_SEL_CB_ARGS \
	(Widget w, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, unsigned long *length, int *format)

#ifdef SIGNALRETURNSINT
#define SIGNAL_T int
#define SIGNAL_RETURN return 0
#else
#define SIGNAL_T void
#define SIGNAL_RETURN return
#endif

#endif/*included_proto_h*/
