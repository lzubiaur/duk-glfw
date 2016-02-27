/*
 * duk_glfw.c
 * Copyright (c) 2016 Laurent Zubiaur
 * MIT License (http://opensource.org/licenses/MIT)
 */

#include "duk_glfw.h"
#include "GLFW/glfw3.h" /* GLFW library header */

/* Enable/disable features binding */
#define DUK__GLFW_CLIPBOARD_BIND
#define DUK__GLFW_CREATE_CURSOR_BIND
#define DUK__GLFW_ERROR_HANDLING_BIND
#define DUK__GLFW_INPUT_MODE_BIND
#define DUK__GLFW_JOYSTICK_BIND
#define DUK__GLFW_KEYBOARD_BIND
#define DUK__GLFW_MANUAL_EXT_LOADING_BIND
#define DUK__GLFW_MONITOR_BIND
#define DUK__GLFW_MONITOR_MODE_BIND
#define DUK__GLFW_MOUSE_BIND
#define DUK__GLFW_MOUSE_CALLBACK_BIND
#define DUK__GLFW_TIME_BIND
#define DUK__GLFW_VERSION_BIND
#define DUK__GLFW_WINDOW_CALLBACKS_BIND
#define DUK__GLFW_WINDOW_EXTRA_BIND

#if defined(DUK__GLFW_ERROR_HANDLING_BIND)
#define GLFW_ERR_CALLBACK_STASH_KEY "glfwErrCallbackKey"
#endif

/* Callback template (setKeyCallback, setWindowSizeCallback...)
 * @param __p1__ GLFW API function name used to register the C callback function.
 * @param __p2__ name of the javascript callback heap pointer in the user_data struct.
 * @param __p3__ name of the C function callback counterpart.
 */
#define DUK__REGISTER_CALLBACK(__p1__, __p2__, __p3__)       \
  do {                                                      \
    GLFWwindow *window;                                     \
    duk_glfw_user_data *u;                                       \
    window = duk_require_pointer(ctx, 0);                   \
    u = glfwGetWindowUserPointer(window);                   \
    if ((u->__p2__ = duk_get_heapptr(ctx, 1)) == NULL) {    \
      __p1__(window, NULL);                                 \
      duk_push_null(ctx);                                   \
      return 1;                                             \
    }                                                       \
    __p1__(window, __p3__);                                 \
    duk_push_heapptr(ctx, u->__p2__);                       \
  } while(0)

/* Internal user data associated to a GLFWwindow. It's used to store callback
 * pointers.
 */
typedef struct duk_glfw_user_data {
#if defined(DUK__GLFW_MOUSE_CALLBACK_BIND) || \
    defined(DUK__GLFW_WINDOW_CALLBACKS_BIND) || \
    defined(DUK__GLFW_KEYBOARD_BIND)
  duk_context *ctx;
#endif
  /* Callback pointers */
#if defined(DUK__GLFW_WINDOW_CALLBACKS_BIND)
  void *window_pos_callback_ptr;
  void *window_size_callback_ptr;
  void *window_close_callback_ptr;
  void *window_refresh_callback_ptr;
  void *window_focus_callback_ptr;
  void *window_iconify_callback_ptr;
  void *framebuffer_size_callback_ptr;
#endif
#if defined(DUK__GLFW_KEYBOARD_BIND)
  void *key_callback_ptr;
  void *char_callback_ptr;
  void *char_mods_callback_ptr;
#endif
#if defined(DUK__GLFW_MOUSE_CALLBACK_BIND)
  void *mouse_button_callback_ptr;
  void *cursor_pos_callback_ptr;
  void *cursor_enter_callback_ptr;
  void *scroll_callback_ptr;
  void *drop_callback_ptr;
#endif
#if defined(DUK__GLFW_WINDOW_EXTRA_BIND)
  /* User pointer set/returned in glfwSetWindowUserPointer/glfwGetWindowUserPointer */
  void *user_ptr;
#endif
} duk_glfw_user_data;


/* Context handling */

DUK_LOCAL duk_ret_t glfw_make_context_current(duk_context *ctx) {
  GLFWwindow *window = NULL;
  window = duk_require_pointer(ctx, 0);
  glfwMakeContextCurrent(window);
  return 0;
}

DUK_LOCAL duk_ret_t glfw_get_current_context(duk_context *ctx) {
  duk_push_pointer(ctx, glfwGetCurrentContext());
  return 1;
}

DUK_LOCAL duk_ret_t glfw_swap_interval(duk_context *ctx) {
  glfwSwapInterval(duk_require_int(ctx, -1));
  return 0;
}

#if defined(DUK__GLFW_MANUAL_EXT_LOADING_BIND)
DUK_LOCAL duk_ret_t glfw_extension_supported(duk_context *ctx) {
  duk_push_boolean(ctx, glfwExtensionSupported(duk_get_string(ctx, 0)));
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_proc_address(duk_context *ctx) {
#if defined(__GNUC__)
  duk_push_pointer(ctx, (__extension__ (void *)glfwGetProcAddress(duk_get_string(ctx, 0))));
#else
  duk_push_pointer(ctx, (void *)glfwGetProcAddress(duk_get_string(ctx, 0)));
#endif
  return 1;
}
#endif /* DUK__GLFW_MANUAL_EXT_LOADING_BIND */

/* Initialization */

DUK_LOCAL duk_ret_t glfw_init(duk_context *ctx) {
  duk_push_boolean(ctx, glfwInit());
  return 1;
}

DUK_LOCAL duk_ret_t glfw_terminate(duk_context *ctx) {
  glfwTerminate();
  return 0;
}

#if defined(DUK__GLFW_ERROR_HANDLING_BIND)
/* global reference to the duktape context. Required by GLFW error callback */
/* XXX is there any workaround to avoid keeping a global reference to the context? */
/* TODO use duk_get_heapptr + duk_push_heapptr for performance */
static duk_context *_ctx = NULL;

DUK_LOCAL void error_callback(int error, const char* description)
{
  duk_push_global_stash(_ctx);
  duk_get_prop_string(_ctx, -1, GLFW_ERR_CALLBACK_STASH_KEY);
  duk_push_int(_ctx, error);
  duk_push_string(_ctx, description);
  duk_call(_ctx, 2);
}

DUK_LOCAL duk_ret_t glfw_set_error_callback(duk_context *ctx) {
  if (duk_is_undefined(ctx, 0)) {
    glfwSetErrorCallback(NULL);
    return 0;
  }

  if (duk_is_function(ctx, 0) == 0) {
    duk_error(ctx, DUK_ERR_TYPE_ERROR, "not a function");
  }
  duk_push_global_stash(ctx);
  duk_dup(ctx, 0); /* Push the error function callback  */
  duk_put_prop_string(ctx, -2, GLFW_ERR_CALLBACK_STASH_KEY);

  glfwSetErrorCallback(error_callback);
  return 0;
}
#endif /* DUK__GLFW_ERROR_HANDLING_BIND */

/* Version */

#if defined(DUK__GLFW_VERSION_BIND)
/* Returns an array with major, minor and revision */
DUK_LOCAL duk_ret_t glfw_get_version(duk_context *ctx) {
  int major = 0, minor = 0, rev = 0;
  glfwGetVersion(&major, &minor, &rev);
  duk_push_array(ctx);
  duk_push_int(ctx, major);
  duk_put_prop_index(ctx, -2, 0);
  duk_push_int(ctx, minor);
  duk_put_prop_index(ctx, -2, 1);
  duk_push_int(ctx, rev);
  duk_put_prop_index(ctx, -2, 2);
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_version_string(duk_context *ctx) {
  duk_push_string(ctx, glfwGetVersionString());
  return 1;
}
#endif /* DUK__GLFW_VERSION_BIND */

/* Window handling */

DUK_LOCAL duk_ret_t glfw_default_window_hints(duk_context *ctx) {
  glfwDefaultWindowHints();
  return 0;
}

DUK_LOCAL duk_ret_t glfw_window_hint(duk_context *ctx) {
  glfwWindowHint(duk_require_int(ctx, 0)/* target */,
                 duk_require_int(ctx, 1)/* hint*/);
  return 0;
}

DUK_LOCAL duk_ret_t glfw_create_window(duk_context *ctx) {
#if defined(DUK__GLFW_MOUSE_CALLBACK_BIND) || \
    defined(DUK__GLFW_WINDOW_CALLBACKS_BIND) || \
    defined(DUK__GLFW_KEYBOARD_BIND) || \
    defined(DUK__GLFW_WINDOW_EXTRA_BIND)
  duk_glfw_user_data *u = NULL;
#endif
  GLFWwindow *window = NULL;
  int width = 0;
  int height = 0;
  const char *title = NULL;
  GLFWmonitor *monitor = NULL;
  GLFWwindow *share = NULL;

#if defined(DUK__GLFW_MOUSE_CALLBACK_BIND) || \
    defined(DUK__GLFW_WINDOW_CALLBACKS_BIND) || \
    defined(DUK__GLFW_KEYBOARD_BIND) || \
    defined(DUK__GLFW_WINDOW_EXTRA_BIND)
  u = (duk_glfw_user_data*)malloc(sizeof(duk_glfw_user_data));
#endif
#if defined(DUK__GLFW_MOUSE_CALLBACK_BIND) || \
    defined(DUK__GLFW_WINDOW_CALLBACKS_BIND) || \
    defined(DUK__GLFW_KEYBOARD_BIND)
  u->ctx = ctx;
#endif

  width = duk_require_int(ctx, 0);
  height = duk_require_int(ctx, 1);
  title = duk_require_string(ctx, 2);
  monitor =  duk_get_pointer(ctx, 3);
  share = duk_get_pointer(ctx, 4);

  if ((window = glfwCreateWindow(width, height, title, monitor, share)) != NULL) {
#if defined(DUK__GLFW_MOUSE_CALLBACK_BIND) || \
    defined(DUK__GLFW_WINDOW_CALLBACKS_BIND) || \
    defined(DUK__GLFW_KEYBOARD_BIND) || \
    defined(DUK__GLFW_WINDOW_EXTRA_BIND)
    glfwSetWindowUserPointer(window, u);
#endif
    duk_push_pointer(ctx, window);
  } else {
    duk_push_undefined(ctx);
  }

  return 1;
}

DUK_LOCAL duk_ret_t glfw_destroy_window(duk_context *ctx) {
  free((duk_glfw_user_data *)glfwGetWindowUserPointer(duk_require_pointer(ctx, 0)));
  glfwDestroyWindow(duk_require_pointer(ctx, 0));
  return 0;
}

DUK_LOCAL duk_ret_t glfw_window_should_close(duk_context *ctx) {
  int rc = 0;
  rc = glfwWindowShouldClose(duk_require_pointer(ctx ,0));
  duk_push_boolean(ctx, rc);
  return 1;
}

DUK_LOCAL duk_ret_t glfw_set_window_should_close(duk_context *ctx) {
  glfwSetWindowShouldClose(duk_require_pointer(ctx, 0), duk_require_boolean(ctx, 1));
  return 0;
}

#if defined(DUK__GLFW_WINDOW_EXTRA_BIND)
DUK_LOCAL duk_ret_t glfw_set_window_title(duk_context *ctx) {
  glfwSetWindowTitle(duk_require_pointer(ctx, 0), duk_require_string(ctx, 1));
  return 0;
}

DUK_LOCAL duk_ret_t glfw_get_window_pos(duk_context *ctx) {
  int xpos = 0, ypos = 0;
  glfwGetWindowPos(duk_require_pointer(ctx, 0), &xpos, &ypos);
  duk_push_array(ctx);
  duk_push_int(ctx, xpos);
  duk_put_prop_index(ctx, -2, 0);
  duk_push_int(ctx, ypos);
  duk_put_prop_index(ctx, -2, 1);
  return 1;
}

DUK_LOCAL duk_ret_t glfw_set_window_pos(duk_context *ctx) {
  glfwSetWindowPos(duk_require_pointer(ctx, 0),
                   duk_require_int(ctx, 1),
                   duk_require_int(ctx, 2));
  return 0;
}

DUK_LOCAL duk_ret_t glfw_get_window_size(duk_context *ctx) {
  int width = 0, height = 0;
  glfwGetWindowSize(duk_require_pointer(ctx, 0), &width, &height);
  duk_push_array(ctx);
  duk_push_int(ctx, width);
  duk_put_prop_index(ctx, -2, 0);
  duk_push_int(ctx, height);
  duk_put_prop_index(ctx, -2, 1);
  return 1;
}

DUK_LOCAL duk_ret_t glfw_set_window_size(duk_context *ctx) {
  glfwSetWindowSize(duk_require_pointer(ctx, 0),
                    duk_require_int(ctx, 1),
                    duk_require_int(ctx, 2));
  return 0;
}

DUK_LOCAL duk_ret_t glfw_get_framebuffer_size(duk_context *ctx) {
  int width = 0, height = 0;
  glfwGetFramebufferSize(duk_require_pointer(ctx, 0), &width, &height);
  duk_push_array(ctx);
  duk_push_int(ctx, width);
  duk_put_prop_index(ctx, -2, 0);
  duk_push_int(ctx, height);
  duk_put_prop_index(ctx, -2, 1);
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_window_frame_size(duk_context *ctx) {
  /* void glfwGetWindowFrameSize(GLFWwindow* window, int* left, int* top, int* right, int* bottom); */
  int left, top, right, bottom;
  glfwGetWindowFrameSize(duk_require_pointer(ctx, 0), &left, &top, &right, &bottom);
  duk_push_array(ctx);
  duk_push_int(ctx, left);
  duk_put_prop_index(ctx, -2, 0);
  duk_push_int(ctx, top);
  duk_put_prop_index(ctx, -2, 1);
  duk_push_int(ctx, right);
  duk_put_prop_index(ctx, -2, 2);
  duk_push_int(ctx, bottom);
  duk_put_prop_index(ctx, -2, 3);
  return 1;
}

DUK_LOCAL duk_ret_t glfw_iconify_window(duk_context *ctx) {
  glfwIconifyWindow(duk_require_pointer(ctx, 0));
  return 0;
}

DUK_LOCAL duk_ret_t glfw_restore_window(duk_context *ctx) {
  glfwRestoreWindow(duk_require_pointer(ctx, 0));
  return 0;
}

DUK_LOCAL duk_ret_t glfw_show_window(duk_context *ctx) {
  glfwShowWindow(duk_require_pointer(ctx ,0));
  return 0;
}

DUK_LOCAL duk_ret_t glfw_hide_window(duk_context *ctx) {
  glfwHideWindow(duk_require_pointer(ctx, 0));
  return 0;
}

DUK_LOCAL duk_ret_t glfw_get_window_monitor(duk_context *ctx) {
  duk_push_pointer(ctx, glfwGetWindowMonitor(duk_require_pointer(ctx, 0)));
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_window_attrib(duk_context *ctx) {
  int value = 0;
  value = glfwGetWindowAttrib(duk_require_pointer(ctx, 0), duk_require_int(ctx ,1));
  duk_push_int(ctx, value);
  return 1;
}

DUK_LOCAL duk_ret_t glfw_set_window_user_pointer(duk_context *ctx) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(duk_require_pointer(ctx ,0));
  u->user_ptr = duk_get_heapptr(ctx ,1);
  return 0;
}

DUK_LOCAL duk_ret_t glfw_get_window_user_pointer(duk_context *ctx) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(duk_require_pointer(ctx, 0));
  duk_push_heapptr(ctx, u->user_ptr);
  return 1;
}
#endif /* DUK__GLFW_WINDOW_EXTRA_BIND */

#if defined(DUK__GLFW_WINDOW_CALLBACKS_BIND)
/* FIXME window_pos_callback is not called */
DUK_LOCAL void duk__window_pos_callback(GLFWwindow *window, int x, int y) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->window_pos_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_int(u->ctx, x);
  duk_push_int(u->ctx, y);
  duk_call(u->ctx, 3);
}

DUK_LOCAL duk_ret_t glfw_set_window_pos_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetWindowPosCallback, window_pos_callback_ptr, duk__window_pos_callback);
  return 1;
}

DUK_LOCAL void duk__window_size_callback(GLFWwindow *window, int width, int height) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->window_size_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_int(u->ctx, width);
  duk_push_int(u->ctx, height);
  duk_call(u->ctx, 3);
}

DUK_LOCAL duk_ret_t glfw_set_window_size_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetWindowSizeCallback, window_size_callback_ptr, duk__window_size_callback);
  return 1;
}

DUK_LOCAL void duk__window_close_callback(GLFWwindow *window) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->window_close_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_call(u->ctx, 1);
}

DUK_LOCAL duk_ret_t glfw_set_window_close_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetWindowCloseCallback, window_close_callback_ptr, duk__window_close_callback);
  return 1;
}

/* TODO check refresh callback is actually called */
DUK_LOCAL void duk__window_refresh_callback(GLFWwindow *window) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->window_refresh_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_call(u->ctx, 1);
}

DUK_LOCAL duk_ret_t glfw_set_window_refresh_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetWindowRefreshCallback, window_refresh_callback_ptr, duk__window_refresh_callback);
  return 1;
}

DUK_LOCAL void duk__window_focus_callback(GLFWwindow *window, int focused) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->window_focus_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_boolean(u->ctx, focused);
  duk_call(u->ctx, 2);
}

DUK_LOCAL duk_ret_t glfw_set_window_focus_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetWindowFocusCallback, window_focus_callback_ptr, duk__window_focus_callback);
  return 1;
}

DUK_LOCAL void duk__window_iconify_callback(GLFWwindow *window, int iconified) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->window_iconify_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_boolean(u->ctx, iconified);
  duk_call(u->ctx, 2);
}

DUK_LOCAL duk_ret_t glfw_set_window_iconify_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetWindowIconifyCallback, window_iconify_callback_ptr, duk__window_iconify_callback);
  return 1;
}

DUK_LOCAL void duk__framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->framebuffer_size_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_int(u->ctx, width);
  duk_push_int(u->ctx, height);
  duk_call(u->ctx, 3);
}

DUK_LOCAL duk_ret_t glfw_set_framebuffer_size_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetFramebufferSizeCallback, framebuffer_size_callback_ptr, duk__framebuffer_size_callback);
  return 1;
}
#endif /* DUK__GLFW_WINDOW_CALLBACKS_BIND */

DUK_LOCAL duk_ret_t glfw_poll_events(duk_context *ctx) {
  glfwPollEvents();
  return 0;
}

DUK_LOCAL duk_ret_t glfw_wait_events(duk_context *ctx) {
  glfwWaitEvents();
  return 0;
}

DUK_LOCAL duk_ret_t glfw_post_empty_event(duk_context *ctx) {
  glfwPostEmptyEvent();
  return 0;
}

DUK_LOCAL duk_ret_t glfw_swap_buffers(duk_context *ctx) {
  glfwSwapBuffers(duk_require_pointer(ctx, 0));
  return 0;
}

/* Monitor handling */
/* http://www.glfw.org/docs/latest/group__monitor.html */

#if defined(DUK__GLFW_MONITOR_BIND)
DUK_LOCAL duk_ret_t glfw_get_monitors(duk_context *ctx) {
  int i, count = 0;
  GLFWmonitor **monitors;
  monitors = glfwGetMonitors(&count);
  duk_push_array(ctx);
  for (i=0; i<count; ++i) {
    duk_push_pointer(ctx, monitors[i]);
    duk_put_prop_index(ctx, -2, i);
  }
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_primary_monitor(duk_context *ctx) {
  duk_push_pointer(ctx, glfwGetPrimaryMonitor());
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_monitor_pos(duk_context *ctx) {
  int xpos = 0, ypos = 0;
  glfwGetMonitorPos(duk_require_pointer(ctx, 0), &xpos, &ypos);
  duk_push_array(ctx);
  duk_push_int(ctx, xpos);
  duk_put_prop_index(ctx, -2, 0);
  duk_push_int(ctx, ypos);
  duk_put_prop_index(ctx, -2, 1);
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_monitor_physical_size(duk_context *ctx) {
  int width = 0, height = 0;
  glfwGetMonitorPhysicalSize(duk_require_pointer(ctx, 0), &width, &height);
  duk_push_array(ctx);
  duk_push_int(ctx, width);
  duk_put_prop_index(ctx, -2, 0);
  duk_push_int(ctx, height);
  duk_put_prop_index(ctx, -2, 1);
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_monitor_name(duk_context *ctx) {
  duk_push_string(ctx, glfwGetMonitorName(duk_require_pointer(ctx, 0)));
  return 1;
}

/* TODO implement glfw_set_monitor_callback */
DUK_LOCAL duk_ret_t glfw_set_monitor_callback(duk_context *ctx) {
  fprintf(stderr, "FIXME: Not implemented\n");
  return 0;
}
#endif /* DUK__GLFW_MONITOR_BIND */

#if defined(DUK__GLFW_MONITOR_MODE_BIND)
DUK_LOCAL void duk__push_array_vidmode(duk_context *ctx, const GLFWvidmode *vidmode) {
  duk_push_array(ctx);
  duk_push_int(ctx, vidmode->width);
  duk_put_prop_index(ctx, -2, 0);
  duk_push_int(ctx, vidmode->height);
  duk_put_prop_index(ctx, -2, 1);
  duk_push_int(ctx, vidmode->redBits);
  duk_put_prop_index(ctx, -2, 2);
  duk_push_int(ctx, vidmode->greenBits);
  duk_put_prop_index(ctx, -2, 3);
  duk_push_int(ctx, vidmode->blueBits);
  duk_put_prop_index(ctx, -2, 4);
  duk_push_int(ctx, vidmode->refreshRate);
  duk_put_prop_index(ctx, -2, 5);
}

DUK_LOCAL duk_ret_t glfw_get_video_modes(duk_context *ctx) {
  int count = 0, i;
  const GLFWvidmode *modes = NULL;

  if ((modes = glfwGetVideoModes(duk_require_pointer(ctx, 0), &count)) != NULL) {
    duk_push_array(ctx);
    for (i=0; i<count; ++i) {
      duk__push_array_vidmode(ctx, &modes[i]);
      duk_put_prop_index(ctx, -2, i);
    }
  } else {
    duk_push_null(ctx);
  }
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_video_mode(duk_context *ctx) {
  const GLFWvidmode *mode = NULL;
  if ((mode = glfwGetVideoMode(duk_require_pointer(ctx, 0))) != NULL) {
    duk__push_array_vidmode(ctx, mode);
  } else {
    duk_push_null(ctx);
  }
  return 1;
}

DUK_LOCAL duk_ret_t glfw_set_gamma(duk_context *ctx) {
  glfwSetGamma(duk_require_pointer(ctx, 0), duk_require_number(ctx, 1));
  return 0;
}

DUK_LOCAL void duk__push_array_gamma_elem(duk_context *ctx, const unsigned short *elem, unsigned int size) {
  int i;
  duk_push_array(ctx);
  for (i=0; i<size; ++i) {
    duk_push_uint(ctx, elem[i]);
    duk_put_prop_index(ctx, -2, i);
  }
}

DUK_LOCAL void duk__push_array_gamma_ramp(duk_context *ctx, const GLFWgammaramp *ramp) {
  duk_push_array(ctx);
  duk__push_array_gamma_elem(ctx, ramp->red, ramp->size);
  duk_put_prop_index(ctx, -2, 0);
  duk__push_array_gamma_elem(ctx, ramp->green, ramp->size);
  duk_put_prop_index(ctx, -2, 1);
  duk__push_array_gamma_elem(ctx, ramp->blue, ramp->size);
  duk_put_prop_index(ctx, -2, 2);
}

DUK_LOCAL duk_ret_t glfw_get_gamma_ramp(duk_context *ctx) {
  const GLFWgammaramp *ramp = NULL;
  ramp = glfwGetGammaRamp(duk_require_pointer(ctx,0 ));
  duk__push_array_gamma_ramp(ctx, ramp);
  return 1;
}

/* Return an array of unsigned int at index `idx`. The caller must freed the array
 * Size can be NULL.
 */
DUK_LOCAL duk_ret_t duk__get_array_us(duk_context *ctx) {
  duk_size_t size;
  int i;
  unsigned short *ptr = NULL;

  if (duk_is_array(ctx, -1) == 0) {
    duk_error(ctx, DUK_ERR_TYPE_ERROR, "duk__get_array_ui: Not an array.");
  }
  size = duk_get_length(ctx, -1);
  if ((ptr = duk_alloc(ctx, sizeof(unsigned short) * size)) == NULL) {
    duk_error(ctx, DUK_ERR_ALLOC_ERROR, "duk__get_array_ui: Out of memory.");
  }
  for (i=0; i<size; ++i) {
    if (duk_get_prop_index(ctx, -1, i) == 0) {
      duk_free(ctx, ptr);
      duk_error(ctx, DUK_ERR_RANGE_ERROR, "duk__get_array_ui: invalid index %d.", i);
    }
    ptr[i] = duk_get_uint(ctx, -1);
    duk_pop(ctx);
  }
  duk_push_pointer(ctx, ptr);
  duk_push_uint(ctx, size);
  return 2;
}

/* TODO implement setGammaRamp using buffer instead of array */
DUK_LOCAL duk_ret_t glfw_set_gamma_ramp(duk_context *ctx) {
  GLFWgammaramp ramp = {0};
  int i;
  unsigned short **e;
  if (duk_is_array(ctx, 1) == 0) {
    duk_error(ctx, DUK_ERR_TYPE_ERROR, "glfw_set_gamma_ramp: Not an array.");
  }
  for (i=0, e=&ramp.red; i<3; ++i, e = i<2 ? &ramp.green : &ramp.blue) {
    duk_get_prop_index(ctx, 1, i);
    if (duk_safe_call(ctx, duk__get_array_us, 1, 2)) {
      duk_dup(ctx, -2); /* duplicate error object */
      goto rethrow;
    }
    *e = duk_get_pointer(ctx, -2);
    ramp.size = duk_get_uint(ctx, -1) < ramp.size ? duk_get_uint(ctx, -1) : ramp.size;
    /* No need to pop call to duk_get_prop_index because it's poped and
     * replaced by the return values by duk_safe_call */
    duk_pop_2(ctx);
  }
  glfwSetGammaRamp(duk_require_pointer(ctx, 0), &ramp);
  duk_free(ctx, ramp.red);
  duk_free(ctx, ramp.green);
  duk_free(ctx, ramp.blue);
  return 0;

rethrow:
  duk_free(ctx, ramp.red);
  duk_free(ctx, ramp.green);
  duk_free(ctx, ramp.blue);
  duk_throw(ctx);
  return 0; /* Not reachable */
}

#endif /* DUK__GLFW_MONITOR_MODE_BIND */

/* Input handling */

#if defined(DUK__GLFW_INPUT_MODE_BIND)
DUK_LOCAL duk_ret_t glfw_get_input_mode(duk_context *ctx) {
  duk_push_int(ctx, glfwGetInputMode(duk_require_pointer(ctx, 0), duk_require_int(ctx, 1)));
  return 1;
}

DUK_LOCAL duk_ret_t glfw_set_input_mode(duk_context *ctx) {
  glfwSetInputMode(duk_require_pointer(ctx, 0),
                   duk_require_int(ctx, 1),
                   duk_require_int(ctx, 2));
  return 0;
}
#endif /* DUK__GLFW_INPUT_MODE_BIND */

#if defined(DUK__GLFW_KEYBOARD_BIND)
DUK_LOCAL duk_ret_t glfw_get_key(duk_context *ctx) {
  duk_push_int(ctx, glfwGetKey(duk_require_pointer(ctx, 0), duk_require_int(ctx, 1)));
  return 1;
}
#endif /* DUK__GLFW_KEYBOARD_BIND */

#if defined(DUK__GLFW_MOUSE_BIND)
DUK_LOCAL duk_ret_t glfw_get_mouse_button(duk_context *ctx) {
  duk_push_int(ctx, glfwGetMouseButton(duk_require_pointer(ctx, 0), duk_require_int(ctx, 1)));
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_cursor_pos(duk_context *ctx) {
  double xpos, ypos;
  glfwGetCursorPos(duk_require_pointer(ctx, 0), &xpos, &ypos);
  duk_push_array(ctx);
  duk_push_number(ctx, xpos);
  duk_put_prop_index(ctx, -2, 0);
  duk_push_number(ctx, ypos);
  duk_put_prop_index(ctx, -2, 1);
  return 1;
}

DUK_LOCAL duk_ret_t glfw_set_cursor_pos(duk_context *ctx) {
  glfwSetCursorPos(duk_require_pointer(ctx, 0),
                   duk_require_number(ctx ,1),
                   duk_require_number(ctx ,2));
  return 0;
}
#endif /* DUK__GLFW_MOUSE_BIND */

#if defined(DUK__GLFW_CREATE_CURSOR_BIND)
/* @param buffer
 * @param width
 * @param height
 * @param xhot
 * @param yhot
 */
DUK_LOCAL duk_ret_t glfw_create_cursor(duk_context *ctx) {
  GLFWimage image;
  duk_size_t len;

  image.pixels = (unsigned char *)duk_require_buffer_data(ctx, 0, &len);
  image.width  = duk_require_int(ctx, 1);
  image.height = duk_require_int(ctx, 2);
  duk_push_pointer(ctx, glfwCreateCursor(&image,
                                         duk_require_int(ctx, 3),
                                         duk_require_int(ctx, 4)));
  return 1;
}

DUK_LOCAL duk_ret_t glfw_create_standard_cursor(duk_context *ctx) {
  duk_push_pointer(ctx, glfwCreateStandardCursor(duk_require_int(ctx, 0)));
  return 1;
}

DUK_LOCAL duk_ret_t glfw_destroy_cursor(duk_context *ctx) {
  glfwDestroyCursor(duk_require_pointer(ctx, 0));
  return 0;
}

DUK_LOCAL duk_ret_t glfw_set_cursor(duk_context *ctx) {
  glfwSetCursor(duk_require_pointer(ctx, 0), duk_require_pointer(ctx, 1));
  return 0;
}
#endif /* DUK__GLFW_CREATE_CURSOR_BIND */

#if defined(DUK__GLFW_KEYBOARD_BIND)
DUK_LOCAL void duk__key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->key_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_int(u->ctx, key);
  duk_push_int(u->ctx, scancode);
  duk_push_int(u->ctx, action);
  duk_push_int(u->ctx, mods);
  duk_call(u->ctx, 5);
}

DUK_LOCAL duk_ret_t glfw_set_key_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetKeyCallback, key_callback_ptr, duk__key_callback);
  return 1;
}

DUK_LOCAL void duk__char_callback(GLFWwindow *window, unsigned int character) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->char_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_int(u->ctx, character);
  duk_call(u->ctx, 2);
}

DUK_LOCAL duk_ret_t glfw_set_char_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetCharCallback, char_callback_ptr, duk__char_callback);
  return 1;
}

DUK_LOCAL void duk__set_char_mods_callback(GLFWwindow *window, unsigned int codepoint, int mods) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->char_mods_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_uint(u->ctx, codepoint);
  duk_push_int(u->ctx, mods);
  duk_call(u->ctx, 3);
}

DUK_LOCAL duk_ret_t glfw_set_char_mods_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetCharModsCallback, char_mods_callback_ptr, duk__set_char_mods_callback);
  return 1;
}
#endif /* DUK__GLFW_KEYBOARD_BIND */

#if defined(DUK__GLFW_MOUSE_CALLBACK_BIND)
DUK_LOCAL void duk__mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->mouse_button_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_int(u->ctx, button);
  duk_push_int(u->ctx, action);
  duk_push_int(u->ctx, mods);
  duk_call(u->ctx, 4);
}

DUK_LOCAL duk_ret_t glfw_set_mouse_button_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetMouseButtonCallback, mouse_button_callback_ptr, duk__mouse_button_callback);
  return 1;
}

DUK_LOCAL void duk__cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->cursor_pos_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_number(u->ctx, xpos);
  duk_push_number(u->ctx, ypos);
  duk_call(u->ctx, 3);
}

DUK_LOCAL duk_ret_t glfw_set_cursor_pos_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetCursorPosCallback, cursor_pos_callback_ptr, duk__cursor_pos_callback);
  return 1;
}

DUK_LOCAL void duk__cursor_enter_callback(GLFWwindow *window, int entered) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->cursor_enter_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_boolean(u->ctx, entered);
  duk_call(u->ctx, 2);
}

DUK_LOCAL duk_ret_t glfw_set_cursor_enter_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetCursorEnterCallback, cursor_enter_callback_ptr, duk__cursor_enter_callback);
  return 1;
}

DUK_LOCAL void duk__scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->scroll_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_number(u->ctx, xoffset);
  duk_push_number(u->ctx, yoffset);
  duk_call(u->ctx, 3);
}

DUK_LOCAL duk_ret_t glfw_set_scroll_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetScrollCallback, scroll_callback_ptr, duk__scroll_callback);
  return 1;
}

DUK_LOCAL void duk__drop_callback(GLFWwindow *window, int count, const char **paths) {
  int i;
  duk_glfw_user_data *u;
  u = glfwGetWindowUserPointer(window);
  duk_push_heapptr(u->ctx, u->drop_callback_ptr);
  duk_push_pointer(u->ctx, window);
  duk_push_array(u->ctx);
  for (i=0; i<count; ++i) {
    duk_push_string(u->ctx, paths[i]);
    duk_put_prop_index(u->ctx, -2, i);
  }
  duk_call(u->ctx,2);
}

DUK_LOCAL duk_ret_t glfw_set_drop_callback(duk_context *ctx) {
  DUK__REGISTER_CALLBACK(glfwSetDropCallback, drop_callback_ptr, duk__drop_callback);
  return 1;
}
#endif /* DUK__GLFW_MOUSE_CALLBACK_BIND */

#if defined(DUK__GLFW_JOYSTICK_BIND)
DUK_LOCAL duk_ret_t glfw_joystick_present(duk_context *ctx) {
  duk_push_boolean(ctx, glfwJoystickPresent(duk_require_int(ctx, 0)));
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_joystick_axes(duk_context *ctx) {
  const float *axes;
  int i, count = 0;
  /* The returned array is allocated and freed by GLFW. */
  axes = glfwGetJoystickAxes(duk_require_int(ctx, 0), &count);
  duk_push_array(ctx);
  for (i=0; i<count; ++i) {
    duk_push_number(ctx, axes[i]);
    duk_put_prop_index(ctx, -2, i);
  }
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_joystick_buttons(duk_context *ctx) {
  const unsigned char *buttons;
  int i, count = 0;
  /* The returned array is allocated and freed by GLFW. */
  buttons = glfwGetJoystickButtons(duk_require_int(ctx, 0), &count);
  duk_push_array(ctx);
  for (i=0; i<count; ++i) {
    duk_push_boolean(ctx, buttons[i]);
    duk_put_prop_index(ctx, -2, i);
  }
  return 1;
}

DUK_LOCAL duk_ret_t glfw_get_joystick_name(duk_context *ctx) {
  /* The returned string is allocated and freed by GLFW. */
  duk_push_string(ctx, glfwGetJoystickName(duk_get_int(ctx, 0)));
  return 1;
}
#endif /* DUK__GLFW_JOYSTICK_BIND */

#if defined(DUK__GLFW_CLIPBOARD_BIND)
DUK_LOCAL duk_ret_t glfw_set_clipboard_string(duk_context *ctx) {
  /* The string is copied by GLFW before the function returns. */
  glfwSetClipboardString(duk_get_pointer(ctx, 0), duk_get_string(ctx, 1));
  return 0;
}

DUK_LOCAL duk_ret_t glfw_get_clipboard_string(duk_context *ctx) {
  /* The returned string is allocated and freed by GLFW. */
  duk_push_string(ctx, glfwGetClipboardString(duk_get_pointer(ctx, 0)));
  return 1;
}
#endif /* DUK__GLFW_CLIPBOARD_BIND */

#if defined(DUK__GLFW_TIME_BIND)
DUK_LOCAL duk_ret_t glfw_get_time(duk_context *ctx) {
  duk_push_number(ctx, glfwGetTime());
  return 1;
}

DUK_LOCAL duk_ret_t glfw_set_time(duk_context *ctx) {
  glfwSetTime(duk_get_number(ctx, 0));
  return 0;
}
#endif /* DUK__GLFW_TIME_BIND */

DUK_EXTERNAL duk_ret_t dukopen_glfw(duk_context *ctx) {
  const duk_function_list_entry module_funcs[] = {
    /* Context handling */
    { "makeContextCurrent",          glfw_make_context_current,          1   },
    { "getCurrentContext",           glfw_get_current_context,           0   },
    { "swapInterval",                glfw_swap_interval,                 1   },
    #if defined(DUK__GLFW_MANUAL_EXT_LOADING_BIND)
    { "extensionSupported",          glfw_extension_supported,           1   },
    { "getProcAddress",              glfw_get_proc_address,              1   },
    #endif
    /* Initialization */
    { "init",                        glfw_init,                          0   },
    { "terminate",                   glfw_terminate,                     0   },
    #if defined(DUK__GLFW_ERROR_HANDLING_BIND)
    { "setErrorCallBack",            glfw_set_error_callback,            1   },
    #endif
    /* Version */
    #if defined(DUK__GLFW_VERSION_BIND)
    { "getVersion",                  glfw_get_version,                   0   },
    { "getVersionString",            glfw_get_version_string,            0   },
    #endif
    /* Window handling */
    { "defaultWindowHints",          glfw_default_window_hints,          0   },
    { "windowHint",                  glfw_window_hint,                   2   },
    { "createWindow",                glfw_create_window,                 5   },
    { "destroyWindow",               glfw_destroy_window,                1   },
    { "windowShouldClose",           glfw_window_should_close,           1   },
    { "setWindowShouldClose",        glfw_set_window_should_close,       2   },
    #if defined(DUK__GLFW_WINDOW_EXTRA_BIND)
    { "setWindowTitle",              glfw_set_window_title,              2   },
    { "getWindowPos",                glfw_get_window_pos,                1   },
    { "setWindowPos",                glfw_set_window_pos,                3   },
    { "getWindowSize",               glfw_get_window_size,               1   },
    { "setWindowSize",               glfw_set_window_size,               3   },
    { "getFramebufferSize",          glfw_get_framebuffer_size,          1   },
    { "getWindowFrameSize",          glfw_get_window_frame_size,         1   },
    { "iconifyWindow",               glfw_iconify_window,                1   },
    { "restoreWindow",               glfw_restore_window,                1   },
    { "showWindow",                  glfw_show_window,                   1   },
    { "hideWindow",                  glfw_hide_window,                   1   },
    { "getWindowMonitor",            glfw_get_window_monitor,            1   },
    { "getWindowAttrib",             glfw_get_window_attrib,             2   },
    { "setWindowUserPointer",        glfw_set_window_user_pointer,       2   },
    { "getWindowUserPointer",        glfw_get_window_user_pointer,       1   },
    #endif
    #if defined(DUK__GLFW_WINDOW_CALLBACKS_BIND)
    { "setWindowPosCallback",        glfw_set_window_pos_callback,       2   },
    { "setWindowSizeCallback",       glfw_set_window_size_callback,      2   },
    { "setWindowCloseCallback",      glfw_set_window_close_callback,     2   },
    { "setWindowRefreshCallback",    glfw_set_window_refresh_callback,   2   },
    { "setWindowFocusCallback",      glfw_set_window_focus_callback,     2   },
    { "setWindowIconifyCallback",    glfw_set_window_iconify_callback,   2   },
    { "setFramebufferSizeCallback",  glfw_set_framebuffer_size_callback, 2   },
    #endif
    { "pollEvents",                  glfw_poll_events,                   0   },
    { "waitEvents",                  glfw_wait_events,                   0   },
    { "postEmptyEvent",              glfw_post_empty_event,              0   },
    { "swapBuffers",                 glfw_swap_buffers,                  1   },
    #if defined(DUK__GLFW_MONITOR_BIND)
    /* Monitor handling */
    { "getMonitors",                 glfw_get_monitors,                  1   },
    { "getPrimaryMonitor",           glfw_get_primary_monitor,           0   },
    { "getMonitorPos",               glfw_get_monitor_pos,               3   },
    { "getMonitorPhysicalSize",      glfw_get_monitor_physical_size,     3   },
    { "getMonitorName",              glfw_get_monitor_name,              1   },
    { "setMonitorCallback",          glfw_set_monitor_callback,          1   },
    #endif
    #if defined(DUK__GLFW_MONITOR_MODE_BIND)
    { "getVideoModes",               glfw_get_video_modes,               1   },
    { "getVideoMode",                glfw_get_video_mode,                1   },
    { "setGamma",                    glfw_set_gamma,                     2   },
    { "getGammaRamp",                glfw_get_gamma_ramp,                1   },
    { "setGammaRamp",                glfw_set_gamma_ramp,                2   },
    #endif
    /* Input handling */
    #if defined(DUK__GLFW_INPUT_MODE_BIND)
    { "getInputMode",                glfw_get_input_mode,                2   },
    { "setInputMode",                glfw_set_input_mode,                3   },
    #endif
    #if defined(DUK__GLFW_KEYBOARD_BIND)
    { "getKey",                      glfw_get_key,                       2   },
    #endif
    #if defined(DUK__GLFW_MOUSE_BIND)
    { "getMouseButton",              glfw_get_mouse_button,              2   },
    { "getCursorPos",                glfw_get_cursor_pos,                1   },
    { "setCursorPos",                glfw_set_cursor_pos,                3   },
    #endif
    #if defined(DUK__GLFW_CREATE_CURSOR_BIND)
    { "createCursor",                glfw_create_cursor,                 5   },
    { "createStandardCursor",        glfw_create_standard_cursor,        1   },
    { "destroyCursor",               glfw_destroy_cursor,                1   },
    { "setCursor",                   glfw_set_cursor,                    2   },
    #endif
    #if defined(DUK__GLFW_KEYBOARD_BIND)
    { "setKeyCallback",              glfw_set_key_callback,              2   },
    { "setCharCallback",             glfw_set_char_callback,             2   },
    { "setCharModsCallback",         glfw_set_char_mods_callback,        2   },
    #endif
    #if defined(DUK__GLFW_MOUSE_CALLBACK_BIND)
    { "setMouseButtonCallback",      glfw_set_mouse_button_callback,     2   },
    { "setCursorPosCallback",        glfw_set_cursor_pos_callback,       2   },
    { "setCursorEnterCallback",      glfw_set_cursor_enter_callback,     2   },
    { "setScrollCallback",           glfw_set_scroll_callback,           2   },
    { "setDropCallback",             glfw_set_drop_callback,             2   },
    #endif
    #if defined(DUK__GLFW_JOYSTICK_BIND)
    { "joystickPresent",             glfw_joystick_present,              1   },
    { "getJoystickAxes",             glfw_get_joystick_axes,             1   },
    { "getJoystickButtons",          glfw_get_joystick_buttons,          1   },
    { "getJoystickName",             glfw_get_joystick_name,             1   },
    #endif
    #if defined(DUK__GLFW_CLIPBOARD_BIND)
    { "setClipboardString",          glfw_set_clipboard_string,          2   },
    { "getClipboardString",          glfw_get_clipboard_string,          1   },
    #endif
    #if defined(DUK__GLFW_TIME_BIND)
    { "getTime",                     glfw_get_time,                      0   },
    { "setTime",                     glfw_set_time,                      1   },
    #endif
    { "makeContextCurrent",          glfw_make_context_current,          1   },
    { "getCurrentContext",           glfw_get_current_context,           0   },
    { NULL, NULL, 0 }
  };

  const duk_number_list_entry module_consts[] = {
    { "VERSION_MAJOR",               (double) GLFW_VERSION_MAJOR },
    { "VERSION_MINOR",               (double) GLFW_VERSION_MINOR },
    { "VERSION_REVISION",            (double) GLFW_VERSION_REVISION },
    #if defined(DUK__GLFW_KEYBOARD_BIND)
    /* Key and button actions */
    { "RELEASE",                     (double) GLFW_RELEASE },
    { "PRESS",                       (double) GLFW_PRESS },
    { "REPEAT",                      (double) GLFW_REPEAT },
    /* Keyboard keys */
    { "KEY_UNKNOWN",                 (double) GLFW_KEY_UNKNOWN },
    { "KEY_SPACE",                   (double) GLFW_KEY_SPACE },
    { "KEY_APOSTROPHE",              (double) GLFW_KEY_APOSTROPHE },
    { "KEY_COMMA",                   (double) GLFW_KEY_COMMA },
    { "KEY_MINUS",                   (double) GLFW_KEY_MINUS },
    { "KEY_PERIOD",                  (double) GLFW_KEY_PERIOD },
    { "KEY_SLASH",                   (double) GLFW_KEY_SLASH },
    { "KEY_0",                       (double) GLFW_KEY_0 },
    { "KEY_1",                       (double) GLFW_KEY_1 },
    { "KEY_2",                       (double) GLFW_KEY_2 },
    { "KEY_3",                       (double) GLFW_KEY_3 },
    { "KEY_4",                       (double) GLFW_KEY_4 },
    { "KEY_5",                       (double) GLFW_KEY_5 },
    { "KEY_6",                       (double) GLFW_KEY_6 },
    { "KEY_7",                       (double) GLFW_KEY_7 },
    { "KEY_8",                       (double) GLFW_KEY_8 },
    { "KEY_9",                       (double) GLFW_KEY_9 },
    { "KEY_SEMICOLON",               (double) GLFW_KEY_SEMICOLON },
    { "KEY_EQUAL",                   (double) GLFW_KEY_EQUAL },
    { "KEY_A",                       (double) GLFW_KEY_A },
    { "KEY_B",                       (double) GLFW_KEY_B },
    { "KEY_C",                       (double) GLFW_KEY_C },
    { "KEY_D",                       (double) GLFW_KEY_D },
    { "KEY_E",                       (double) GLFW_KEY_E },
    { "KEY_F",                       (double) GLFW_KEY_F },
    { "KEY_G",                       (double) GLFW_KEY_G },
    { "KEY_H",                       (double) GLFW_KEY_H },
    { "KEY_I",                       (double) GLFW_KEY_I },
    { "KEY_J",                       (double) GLFW_KEY_J },
    { "KEY_K",                       (double) GLFW_KEY_K },
    { "KEY_L",                       (double) GLFW_KEY_L },
    { "KEY_M",                       (double) GLFW_KEY_M },
    { "KEY_N",                       (double) GLFW_KEY_N },
    { "KEY_O",                       (double) GLFW_KEY_O },
    { "KEY_P",                       (double) GLFW_KEY_P },
    { "KEY_Q",                       (double) GLFW_KEY_Q },
    { "KEY_R",                       (double) GLFW_KEY_R },
    { "KEY_S",                       (double) GLFW_KEY_S },
    { "KEY_T",                       (double) GLFW_KEY_T },
    { "KEY_U",                       (double) GLFW_KEY_U },
    { "KEY_V",                       (double) GLFW_KEY_V },
    { "KEY_W",                       (double) GLFW_KEY_W },
    { "KEY_X",                       (double) GLFW_KEY_X },
    { "KEY_Y",                       (double) GLFW_KEY_Y },
    { "KEY_Z",                       (double) GLFW_KEY_Z },
    { "KEY_LEFT_BRACKET",            (double) GLFW_KEY_LEFT_BRACKET },
    { "KEY_BACKSLASH",               (double) GLFW_KEY_BACKSLASH },
    { "KEY_RIGHT_BRACKET",           (double) GLFW_KEY_RIGHT_BRACKET },
    { "KEY_GRAVE_ACCENT",            (double) GLFW_KEY_GRAVE_ACCENT },
    { "KEY_WORLD_1",                 (double) GLFW_KEY_WORLD_1 },
    { "KEY_WORLD_2",                 (double) GLFW_KEY_WORLD_2 },
    { "KEY_ESCAPE",                  (double) GLFW_KEY_ESCAPE },
    { "KEY_ENTER",                   (double) GLFW_KEY_ENTER },
    { "KEY_TAB",                     (double) GLFW_KEY_TAB },
    { "KEY_BACKSPACE",               (double) GLFW_KEY_BACKSPACE },
    { "KEY_INSERT",                  (double) GLFW_KEY_INSERT },
    { "KEY_DELETE",                  (double) GLFW_KEY_DELETE },
    { "KEY_RIGHT",                   (double) GLFW_KEY_RIGHT },
    { "KEY_LEFT",                    (double) GLFW_KEY_LEFT },
    { "KEY_DOWN",                    (double) GLFW_KEY_DOWN },
    { "KEY_UP",                      (double) GLFW_KEY_UP },
    { "KEY_PAGE_UP",                 (double) GLFW_KEY_PAGE_UP },
    { "KEY_PAGE_DOWN",               (double) GLFW_KEY_PAGE_DOWN },
    { "KEY_HOME",                    (double) GLFW_KEY_HOME },
    { "KEY_END",                     (double) GLFW_KEY_END },
    { "KEY_CAPS_LOCK",               (double) GLFW_KEY_CAPS_LOCK },
    { "KEY_SCROLL_LOCK",             (double) GLFW_KEY_SCROLL_LOCK },
    { "KEY_NUM_LOCK",                (double) GLFW_KEY_NUM_LOCK },
    { "KEY_PRINT_SCREEN",            (double) GLFW_KEY_PRINT_SCREEN },
    { "KEY_PAUSE",                   (double) GLFW_KEY_PAUSE },
    { "KEY_F1",                      (double) GLFW_KEY_F1 },
    { "KEY_F2",                      (double) GLFW_KEY_F2 },
    { "KEY_F3",                      (double) GLFW_KEY_F3 },
    { "KEY_F4",                      (double) GLFW_KEY_F4 },
    { "KEY_F5",                      (double) GLFW_KEY_F5 },
    { "KEY_F6",                      (double) GLFW_KEY_F6 },
    { "KEY_F7",                      (double) GLFW_KEY_F7 },
    { "KEY_F8",                      (double) GLFW_KEY_F8 },
    { "KEY_F9",                      (double) GLFW_KEY_F9 },
    { "KEY_F10",                     (double) GLFW_KEY_F10 },
    { "KEY_F11",                     (double) GLFW_KEY_F11 },
    { "KEY_F12",                     (double) GLFW_KEY_F12 },
    { "KEY_F13",                     (double) GLFW_KEY_F13 },
    { "KEY_F14",                     (double) GLFW_KEY_F14 },
    { "KEY_F15",                     (double) GLFW_KEY_F15 },
    { "KEY_F16",                     (double) GLFW_KEY_F16 },
    { "KEY_F17",                     (double) GLFW_KEY_F17 },
    { "KEY_F18",                     (double) GLFW_KEY_F18 },
    { "KEY_F19",                     (double) GLFW_KEY_F19 },
    { "KEY_F20",                     (double) GLFW_KEY_F20 },
    { "KEY_F21",                     (double) GLFW_KEY_F21 },
    { "KEY_F22",                     (double) GLFW_KEY_F22 },
    { "KEY_F23",                     (double) GLFW_KEY_F23 },
    { "KEY_F24",                     (double) GLFW_KEY_F24 },
    { "KEY_F25",                     (double) GLFW_KEY_F25 },
    { "KEY_KP_0",                    (double) GLFW_KEY_KP_0 },
    { "KEY_KP_1",                    (double) GLFW_KEY_KP_1 },
    { "KEY_KP_2",                    (double) GLFW_KEY_KP_2 },
    { "KEY_KP_3",                    (double) GLFW_KEY_KP_3 },
    { "KEY_KP_4",                    (double) GLFW_KEY_KP_4 },
    { "KEY_KP_5",                    (double) GLFW_KEY_KP_5 },
    { "KEY_KP_6",                    (double) GLFW_KEY_KP_6 },
    { "KEY_KP_7",                    (double) GLFW_KEY_KP_7 },
    { "KEY_KP_8",                    (double) GLFW_KEY_KP_8 },
    { "KEY_KP_9",                    (double) GLFW_KEY_KP_9 },
    { "KEY_KP_DECIMAL",              (double) GLFW_KEY_KP_DECIMAL },
    { "KEY_KP_DIVIDE",               (double) GLFW_KEY_KP_DIVIDE },
    { "KEY_KP_MULTIPLY",             (double) GLFW_KEY_KP_MULTIPLY },
    { "KEY_KP_SUBTRACT",             (double) GLFW_KEY_KP_SUBTRACT },
    { "KEY_KP_ADD",                  (double) GLFW_KEY_KP_ADD },
    { "KEY_KP_ENTER",                (double) GLFW_KEY_KP_ENTER },
    { "KEY_KP_EQUAL",                (double) GLFW_KEY_KP_EQUAL },
    { "KEY_LEFT_SHIFT",              (double) GLFW_KEY_LEFT_SHIFT },
    { "KEY_LEFT_CONTROL",            (double) GLFW_KEY_LEFT_CONTROL },
    { "KEY_LEFT_ALT",                (double) GLFW_KEY_LEFT_ALT },
    { "KEY_LEFT_SUPER",              (double) GLFW_KEY_LEFT_SUPER },
    { "KEY_RIGHT_SHIFT",             (double) GLFW_KEY_RIGHT_SHIFT },
    { "KEY_RIGHT_CONTROL",           (double) GLFW_KEY_RIGHT_CONTROL },
    { "KEY_RIGHT_ALT",               (double) GLFW_KEY_RIGHT_ALT },
    { "KEY_RIGHT_SUPER",             (double) GLFW_KEY_RIGHT_SUPER },
    { "KEY_MENU",                    (double) GLFW_KEY_MENU },
    { "KEY_LAST",                    (double) GLFW_KEY_LAST },
    /* Modifier key flags */
    { "MOD_SHIFT",                   (double) GLFW_MOD_SHIFT },
    { "MOD_CONTROL",                 (double) GLFW_MOD_CONTROL },
    { "MOD_ALT",                     (double) GLFW_MOD_ALT },
    { "MOD_SUPER",                   (double) GLFW_MOD_SUPER },
    #endif
    #if defined(DUK__GLFW_MOUSE_BIND) || defined(DUK__GLFW_MOUSE_CALLBACK_BIND)
    /* Mouse buttons */
    { "MOUSE_BUTTON_1",              (double) GLFW_MOUSE_BUTTON_1 },
    { "MOUSE_BUTTON_2",              (double) GLFW_MOUSE_BUTTON_2 },
    { "MOUSE_BUTTON_3",              (double) GLFW_MOUSE_BUTTON_3 },
    { "MOUSE_BUTTON_4",              (double) GLFW_MOUSE_BUTTON_4 },
    { "MOUSE_BUTTON_5",              (double) GLFW_MOUSE_BUTTON_5 },
    { "MOUSE_BUTTON_6",              (double) GLFW_MOUSE_BUTTON_6 },
    { "MOUSE_BUTTON_7",              (double) GLFW_MOUSE_BUTTON_7 },
    { "MOUSE_BUTTON_8",              (double) GLFW_MOUSE_BUTTON_8 },
    { "MOUSE_BUTTON_LAST",           (double) GLFW_MOUSE_BUTTON_LAST },
    { "MOUSE_BUTTON_LEFT",           (double) GLFW_MOUSE_BUTTON_LEFT },
    { "MOUSE_BUTTON_RIGHT",          (double) GLFW_MOUSE_BUTTON_RIGHT },
    { "MOUSE_BUTTON_MIDDLE",         (double) GLFW_MOUSE_BUTTON_MIDDLE },
    #endif
    #if defined(DUK__GLFW_JOYSTICK_BIND)
    /* Joysticks */
    { "JOYSTICK_1",                  (double) GLFW_JOYSTICK_1 },
    { "JOYSTICK_2",                  (double) GLFW_JOYSTICK_2 },
    { "JOYSTICK_3",                  (double) GLFW_JOYSTICK_3 },
    { "JOYSTICK_4",                  (double) GLFW_JOYSTICK_4 },
    { "JOYSTICK_5",                  (double) GLFW_JOYSTICK_5 },
    { "JOYSTICK_6",                  (double) GLFW_JOYSTICK_6 },
    { "JOYSTICK_7",                  (double) GLFW_JOYSTICK_7 },
    { "JOYSTICK_8",                  (double) GLFW_JOYSTICK_8 },
    { "JOYSTICK_9",                  (double) GLFW_JOYSTICK_9 },
    { "JOYSTICK_10",                 (double) GLFW_JOYSTICK_10 },
    { "JOYSTICK_11",                 (double) GLFW_JOYSTICK_11 },
    { "JOYSTICK_12",                 (double) GLFW_JOYSTICK_12 },
    { "JOYSTICK_13",                 (double) GLFW_JOYSTICK_13 },
    { "JOYSTICK_14",                 (double) GLFW_JOYSTICK_14 },
    { "JOYSTICK_15",                 (double) GLFW_JOYSTICK_15 },
    { "JOYSTICK_16",                 (double) GLFW_JOYSTICK_16 },
    { "JOYSTICK_LAST",               (double) GLFW_JOYSTICK_LAST },
    #endif
    #if defined(DUK__GLFW_ERROR_HANDLING_BIND)
    /* Error codes */
    { "NOT_INITIALIZED",             (double) GLFW_NOT_INITIALIZED },
    { "NO_CURRENT_CONTEXT",          (double) GLFW_NO_CURRENT_CONTEXT },
    { "INVALID_ENUM",                (double) GLFW_INVALID_ENUM },
    { "INVALID_VALUE",               (double) GLFW_INVALID_VALUE },
    { "OUT_OF_MEMORY",               (double) GLFW_OUT_OF_MEMORY },
    { "API_UNAVAILABLE",             (double) GLFW_API_UNAVAILABLE },
    { "VERSION_UNAVAILABLE",         (double) GLFW_VERSION_UNAVAILABLE },
    { "PLATFORM_ERROR",              (double) GLFW_PLATFORM_ERROR },
    { "FORMAT_UNAVAILABLE",          (double) GLFW_FORMAT_UNAVAILABLE },
    #endif
    /* End of Error codes */
    /* Window related attributes */
    { "FOCUSED",                     (double) GLFW_FOCUSED },
    { "ICONIFIED",                   (double) GLFW_ICONIFIED },
    { "RESIZABLE",                   (double) GLFW_RESIZABLE },
    { "VISIBLE",                     (double) GLFW_VISIBLE },
    { "DECORATED",                   (double) GLFW_DECORATED },
    { "AUTO_ICONIFY",                (double) GLFW_AUTO_ICONIFY },
    { "FLOATING",                    (double) GLFW_FLOATING },

    { "RED_BITS",                    (double) GLFW_RED_BITS },
    { "GREEN_BITS",                  (double) GLFW_GREEN_BITS },
    { "BLUE_BITS",                   (double) GLFW_BLUE_BITS },
    { "ALPHA_BITS",                  (double) GLFW_ALPHA_BITS },
    { "DEPTH_BITS",                  (double) GLFW_DEPTH_BITS },
    { "STENCIL_BITS",                (double) GLFW_STENCIL_BITS },
    { "ACCUM_RED_BITS",              (double) GLFW_ACCUM_RED_BITS },
    { "ACCUM_GREEN_BITS",            (double) GLFW_ACCUM_GREEN_BITS },
    { "ACCUM_BLUE_BITS",             (double) GLFW_ACCUM_BLUE_BITS },
    { "ACCUM_ALPHA_BITS",            (double) GLFW_ACCUM_ALPHA_BITS },
    { "AUX_BUFFERS",                 (double) GLFW_AUX_BUFFERS },
    { "STEREO",                      (double) GLFW_STEREO },
    { "SAMPLES",                     (double) GLFW_SAMPLES },
    { "SRGB_CAPABLE",                (double) GLFW_SRGB_CAPABLE },
    { "REFRESH_RATE",                (double) GLFW_REFRESH_RATE },
    { "DOUBLEBUFFER",                (double) GLFW_DOUBLEBUFFER },
    /* Context related attributes */
    { "CLIENT_API",                  (double) GLFW_CLIENT_API },
    { "CONTEXT_VERSION_MAJOR",       (double) GLFW_CONTEXT_VERSION_MAJOR },
    { "CONTEXT_VERSION_MINOR",       (double) GLFW_CONTEXT_VERSION_MINOR },
    { "CONTEXT_REVISION",            (double) GLFW_CONTEXT_REVISION },
    { "CONTEXT_ROBUSTNESS",          (double) GLFW_CONTEXT_ROBUSTNESS },
    { "OPENGL_FORWARD_COMPAT",       (double) GLFW_OPENGL_FORWARD_COMPAT },
    { "OPENGL_DEBUG_CONTEXT",        (double) GLFW_OPENGL_DEBUG_CONTEXT },
    { "OPENGL_PROFILE",              (double) GLFW_OPENGL_PROFILE },
    { "CONTEXT_RELEASE_BEHAVIOR",    (double) GLFW_CONTEXT_RELEASE_BEHAVIOR },
    { "OPENGL_API",                  (double) GLFW_OPENGL_API },
    { "OPENGL_ES_API",               (double) GLFW_OPENGL_ES_API },
    { "NO_ROBUSTNESS",               (double) GLFW_NO_ROBUSTNESS },
    { "NO_RESET_NOTIFICATION",       (double) GLFW_NO_RESET_NOTIFICATION },
    { "LOSE_CONTEXT_ON_RESET",       (double) GLFW_LOSE_CONTEXT_ON_RESET },
    { "OPENGL_ANY_PROFILE",          (double) GLFW_OPENGL_ANY_PROFILE },
    { "OPENGL_CORE_PROFILE",         (double) GLFW_OPENGL_CORE_PROFILE },
    { "OPENGL_COMPAT_PROFILE",       (double) GLFW_OPENGL_COMPAT_PROFILE },
    #if defined(DUK__GLFW_INPUT_MODE_BIND)
    /* Input modes (set/getInputMode) */
    { "CURSOR",                      (double) GLFW_CURSOR },
    { "STICKY_KEYS",                 (double) GLFW_STICKY_KEYS },
    { "STICKY_MOUSE_BUTTONS",        (double) GLFW_STICKY_MOUSE_BUTTONS },
    #endif
    #if defined(DUK__GLFW_INPUT_MODE_BIND)
    /* Cursor modes */
    { "CURSOR_NORMAL",               (double) GLFW_CURSOR_NORMAL },
    { "CURSOR_HIDDEN",               (double) GLFW_CURSOR_HIDDEN },
    { "CURSOR_DISABLED",             (double) GLFW_CURSOR_DISABLED },
    #endif
    { "ANY_RELEASE_BEHAVIOR",        (double) GLFW_ANY_RELEASE_BEHAVIOR },
    { "RELEASE_BEHAVIOR_FLUSH",      (double) GLFW_RELEASE_BEHAVIOR_FLUSH },
    { "RELEASE_BEHAVIOR_NONE",       (double) GLFW_RELEASE_BEHAVIOR_NONE },
    #if defined(DUK__GLFW_CREATE_CURSOR_BIND)
    /* Standard cursor shapes */
    { "ARROW_CURSOR",                (double) GLFW_ARROW_CURSOR },
    { "IBEAM_CURSOR",                (double) GLFW_IBEAM_CURSOR },
    { "CROSSHAIR_CURSOR",            (double) GLFW_CROSSHAIR_CURSOR },
    { "HAND_CURSOR",                 (double) GLFW_HAND_CURSOR },
    { "HRESIZE_CURSOR",              (double) GLFW_HRESIZE_CURSOR },
    { "VRESIZE_CURSOR",              (double) GLFW_VRESIZE_CURSOR },
    #endif
    #if defined(DUK__GLFW_MONITOR_BIND)
    /* Monitor events */
    { "CONNECTED",                   (double) GLFW_CONNECTED },
    { "DISCONNECTED",                (double) GLFW_DISCONNECTED },
    { "DONT_CARE",                   (double) GLFW_DONT_CARE },
    #endif
    { NULL, 0.0 }
  };

  duk_push_object(ctx);  /* module result */
  duk_put_function_list(ctx, -1, module_funcs);
  duk_put_number_list(ctx, -1, module_consts);

  /*  Global reference to the duktape context (used by glfwSetErrorCallback) */
#if defined(DUK__GLFW_ERROR_HANDLING_BIND)
  _ctx = ctx;
#endif

  return 1;  /* return module value */
}
