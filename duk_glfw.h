/*
 * duk_glfw.h
 * Copyright (c) 2015 Laurent Zubiaur
 * MIT License (http://opensource.org/licenses/MIT)
 */

#ifndef DUK_GLFW_H
#define DUK_GLFW_H

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

DUK_EXTERNAL_DECL duk_ret_t dukopen_glfw(duk_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* DUK_GLFW_H */
