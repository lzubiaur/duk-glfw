/*
 * test.c
 * Copyright (c) 2016 Laurent Zubiaur
 * MIT License (http://opensource.org/licenses/MIT)
 */

#include "duktape.h"
#include "duk_glfw.h"

#if defined(CPR_BUILD_WINDOWS)
/* Faster builds with smaller header files */
#define VC_EXTRALEAN 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <stdio.h>

DUK_LOCAL void fatal_handler(duk_context *ctx, duk_errcode_t code, const char *msg) {
  fprintf(stderr, "Fatal error: %s [code: %d]", msg, code);
  /* Fatal handler should not return. */
  exit(EXIT_FAILURE);
}

int start(int argc, char *argv[]) {
  duk_context *ctx = NULL;
  char *script_filename = "glfw.js";

  if (argc > 1) {
    script_filename = argv[1];
  }

  if ((ctx = duk_create_heap(NULL, NULL, NULL, NULL, fatal_handler)) == NULL) {
    fprintf(stderr, "FATAL: Failed to create the Duktape context.\n");
    goto finished;
  }

  /* Load the `GLFW` module */
  duk_push_c_function(ctx, dukopen_glfw, 0);
  duk_call(ctx, 0);
  duk_put_global_string(ctx, "glfw");

  if (duk_peval_file(ctx, script_filename) != 0) {
    fprintf(stderr, "Error loading file '%s' : %s\n", script_filename, duk_safe_to_string(ctx, -1));
    goto finished;
  }

finished:
  duk_destroy_heap(ctx); /* No-op if ctx is NULL */
  return EXIT_SUCCESS;
}

#if defined(__WIN32)
int CALLBACK WinMain(
  HINSTANCE hInstance,     /* handle to the current instance of the application. */
  HINSTANCE hPrevInstance, /* handle to the previous instance of the application */
  LPSTR     lpCmdLine,     /* command line for the application */
  int       nCmdShow) {    /* controls how the window is to be shown */
  /* Get command line count and string from global variable __argc and __argv */
  return start(__argc, __argv);
}
#else
/* Linux and OSX use standard entry point */
int main(int argc, char *argv[]) {
  return start(argc, argv);
}
#endif
