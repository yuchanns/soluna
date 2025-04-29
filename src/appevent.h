#ifndef soluna_app_event_h
#define soluna_app_event_h

struct event_message {
	const char *typestr;
	int p1;
	int p2;
};

static inline void
mouse_message(struct event_message *em, const sapp_event* ev) {
	em->typestr = NULL;
	em->p1 = 0;
	em->p2 = 0;
	switch (ev->type) {
	case SAPP_EVENTTYPE_MOUSE_MOVE:
		em->typestr = "mouse_move";
		em->p1 = ev->mouse_x;
		em->p2 = ev->mouse_y;
		break;
	case SAPP_EVENTTYPE_MOUSE_DOWN:
	case SAPP_EVENTTYPE_MOUSE_UP:
		em->typestr = "mouse_button";
		em->p1 = ev->mouse_button;
		em->p2 = ev->type == SAPP_EVENTTYPE_MOUSE_DOWN;
		break;
	case SAPP_EVENTTYPE_MOUSE_SCROLL:
		em->typestr = "mouse_scroll";
		em->p1 = ev->scroll_y;
		em->p2 = ev->scroll_x;
		break;
	default:
		em->typestr = "mouse";
		em->p1 = ev->type;
		break;
	}
}

static inline void
window_message(struct event_message *em, const sapp_event *ev) {
	em->typestr = NULL;
	em->p1 = 0;
	em->p2 = 0;	
	switch (ev->type) {
	case SAPP_EVENTTYPE_RESIZED:
		em->typestr = "window_resize";
		em->p1 = ev->window_width;
		em->p2 = ev->window_height;
		break;
	default:
		em->typestr = "window";
		em->p1 = ev->type;
		break;
	}
}

static inline void
app_event_unpack(struct event_message *em, const sapp_event* ev) {
	switch (ev->type) {
	case SAPP_EVENTTYPE_MOUSE_MOVE:
	case SAPP_EVENTTYPE_MOUSE_DOWN:
	case SAPP_EVENTTYPE_MOUSE_UP:
	case SAPP_EVENTTYPE_MOUSE_SCROLL:
	case SAPP_EVENTTYPE_MOUSE_ENTER:
	case SAPP_EVENTTYPE_MOUSE_LEAVE:
		mouse_message(em, ev);
		break;
	case SAPP_EVENTTYPE_RESIZED:
		window_message(em, ev);
		break;
	default:
		em->typestr = "message";
		em->p1 = ev->type;
		em->p2 = 0;
		break;
	}
}

#endif
