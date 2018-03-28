## GLFW binding for Duktape

[![Build Status](https://travis-ci.org/lzubiaur/duk-glfw.svg?branch=master)](https://travis-ci.org/lzubiaur/duk-glfw)

Binding of the OpenGL [GLFW](http://www.glfw.org/) API for the [Duktape](http://duktape.org/) JavaScript engine.

To use the binding just include `duk_glfw.c` and `duk_glfw.h` into your project and call `dukopen_glfw` to load the module.

```C
#include "duk_glfw.h"

int main() {

  // Create Duktape heap...

  // Load the module
  duk_push_c_function(ctx, dukopen_glfw, 0);
  duk_call(ctx, 0);
  duk_put_global_string(ctx, "glfw");

}
```

Now you can call GLFW functions from JavaScript using the `glfw` global object.

```Javascript
  glfw.setErrorCallBack(myErrorHandler);
  glfw.init();
  var window = glfw.createWindow(480, 320, 'my(window');
```

See `tests/glfw.js` for more examples.

### Build the tests

The CMake script will download both GLFW and Duktape before building the tests.

```Bash
mkdir build && cd build
cmake ..
make
./duk_glfw
```

### License

[MIT License](http://opensource.org/licenses/MIT)
