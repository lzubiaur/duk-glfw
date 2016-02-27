/*
 * glfw.js
 * Copyright (c) 2016 Laurent Zubiaur
 * MIT License (http://opensource.org/licenses/MIT)
 */

var log = new Duktape.Logger('glfw.js');

/// Callback functions

function keyHandler(window, key, scancode, action, mods) {
  // log key code
  log.info('[keyHandler]:', key, scancode, action, mods);
  // close window if hit `ESC`
  if (key == glfw.KEY_ESCAPE) {
    glfw.setWindowShouldClose(window, true);
  }
  // remove this key callback if hit `SPACE`
  if (key == glfw.KEY_SPACE) {
    glfw.setKeyCallback(window, null);
  }
}

function errorHandler(code, message) { log.error(code, message); }
// XXX print unicode character instead of its code
function charHandler(window, character) { log.info('[charHandler]:', character); }
function charModsHandler(window, character) { log.info('[charModsHandler]:', character); }
function windowPosHandler(window, x, y) { log.info('[windowPosHandler]:', x, y); }
function windowSizeHandler(window, w, h) { log.info('[windowSizeHandler]:', w, h); }
function windowCloseHandler(window) { log.info('[windowCloseHandler]'); }
function windowRefreshHandler(window) { log.info('[windowRefreshHandler]'); }
function windowFocusHandler(window, focused) { log.info('[windowFocusHandler]:', focused); }
function windowIconifyHandler(window, iconified) { log.info('[windowIconifyHandler]:', iconified); }
function frameBufferSizeHandler(window, w, h) { log.info('[frameBufferSizeHandler]:', w, h); }
function mouseButtonHandler(window, button, action, mods) { log.info('[mouseButtonHandler]', button, action, mods); }
function cursorPosHandler(window, x, y) { log.info('[cursorPosHandler]', x, y); }
function cursorEnterHandler(window, entered) { log.info('[cursorEnterHandler]', entered); }
function scrollHandler(window, xoffset, yoffset) { log.info('[scrollHandler]', xoffset, yoffset); }
function dropHandler(window, paths) { log.info('[dropHandler]', paths); }

try {

  if (glfw == null) {
    throw new Error('GLFW module not loaded (null)');
  }

  glfw.setErrorCallBack(errorHandler);

  if (glfw.init() == null) {
    throw new Error('Cannot initialize GLFW library');
  }

  log.info('GFLW version', glfw.VERSION_MAJOR + '.' + glfw.VERSION_MINOR + '.' + glfw.VERSION_REVISION);

  /// Window handling
  // glfw.windowHint(glfw.RESIZABLE, 0);
  // glfwCreateWindow
  var window = glfw.createWindow(480, 320, 'my(window');
  // Full screen
  //(window = glfw.createWindow(640, 480, 'my(window', glfw.getPrimaryMonitor());
  if (window == null) {
    throw new Error('FATAL: Failed to create(window');
  }
  glfw.setWindowTitle(window, 'another title');
  glfw.setWindowSize(window, 640, 480);
  glfw.setWindowPos(window, 100, 200);
  log.info('Window position:', glfw.getWindowPos(window));
  log.info('Window size:', glfw.getWindowSize(window));
  log.info('Framebuffer size:', glfw.getFramebufferSize(window));
  log.info('Window Frame size', glfw.getWindowFrameSize(window));
  glfw.iconifyWindow(window);
  glfw.restoreWindow(window);
  glfw.hideWindow(window);
  glfw.showWindow(window);
  log.info('Window monitor (null if not fullscreen): ', glfw.getWindowMonitor(window));
  log.info('Window is resizable: ', glfw.getWindowAttrib(window, glfw.RESIZABLE));
  glfw.setWindowUserPointer(window, 'my user data');
  log.info(glfw.getWindowUserPointer(window));

  /// Callbacks
  glfw.setWindowPosCallback(window, windowPosHandler);
  glfw.setWindowSizeCallback(window, windowSizeHandler);
  glfw.setWindowCloseCallback(window, windowCloseHandler);
  glfw.setWindowRefreshCallback(window, windowRefreshHandler);
  glfw.setWindowFocusCallback(window, windowFocusHandler);
  glfw.setWindowIconifyCallback(window, windowIconifyHandler);
  glfw.setFramebufferSizeCallback(window, frameBufferSizeHandler);

  /// Monitor handling
  var monitor = glfw.getPrimaryMonitor()
  log.info('Monitors:', glfw.getMonitors());
  log.info('Monitor position:', glfw.getMonitorPos(monitor));
  log.info('Monitor physical size:', glfw.getMonitorPhysicalSize(monitor));
  log.info('Monitor name:', glfw.getMonitorName(monitor));
  // Not implemented
  // glfw.setMonitorCallback()
  for(var mode in glfw.getVideoModes(monitor)) {
    log.info('Monitor video mode:', mode);
  }
  log.info('Monitor current mode:', glfw.getVideoMode(monitor));
  log.info('Monitor gamma ramp:', glfw.getGammaRamp(monitor));
  // TODO test setGammaRamp with actual data
  // Testing with dummy data will crash on Linux (Ubuntu)
  // glfw.setGammaRamp(monitor, [red, green, blue]);

  /// Input handling
  glfw.setInputMode(window, glfw.CURSOR, glfw.CURSOR_NORMAL);
  glfw.setInputMode(window, glfw.STICKY_KEYS, 1);
  glfw.setInputMode(window, glfw.STICKY_MOUSE_BUTTONS, 1);
  log.info('[Input mode] Cursor:', glfw.getInputMode(window, glfw.CURSOR));
  log.info('[Input mode] Sticky keys:', glfw.getInputMode(window, glfw.STICKY_KEYS));
  log.info('[Input mode] Sticky mouse buttons:', glfw.getInputMode(window, glfw.STICKY_MOUSE_BUTTONS));
  log.info('[getKey] KEY_K:', glfw.getKey(window, glfw.KEY_K));
  log.info('[getMouseButton] MOUSE_BUTTON_1:', glfw.getMouseButton(window, glfw.MOUSE_BUTTON_1));
  // TODO setCursorPos doesn't seem to work
  glfw.setCursorPos(window, 100, 100);
  log.info('[getCursorPos]:', glfw.getCursorPos(window));
  var cursor = glfw.createStandardCursor(glfw.HAND_CURSOR);
  glfw.setCursor(window, cursor);

  // TODO test createCursor with actual 32bits pixel data
  // var buffer = Duktape.Buffer(256);
  // fill buffer...
  // var cursor = glfw.createCursor(buffer, 16, 16, 0, 0);
  // glfw.setCursor(window, cursor);

  /// Keyboard
  glfw.setKeyCallback(window, keyHandler);
  glfw.setCharCallback(window, charHandler);
  glfw.setCharModsCallback(window, charModsHandler);

  /// Mouse
  glfw.setMouseButtonCallback(window, mouseButtonHandler);
  glfw.setCursorPosCallback(window, cursorPosHandler);
  glfw.setCursorEnterCallback(window, cursorEnterHandler);
  glfw.setScrollCallback(window, scrollHandler);
  glfw.setDropCallback(window, dropHandler);

  /// Joystick
  log.info('[joystickPresent] 0:', glfw.joystickPresent(0));
  log.info('[joystickPresent] 1:', glfw.joystickPresent(1));
  log.info('[joystickPresent] 2:', glfw.joystickPresent(2));
  log.info('[getJoystickAxes]:', glfw.getJoystickAxes(0));
  log.info('[getJoystickButtons]:', glfw.getJoystickButtons(0));
  log.info('[getJoystickName]:', glfw.getJoystickName(0));

  /// clipboard
  glfw.setClipboardString(window, 'Hello world');
  log.info('[getClipboardString]:', glfw.getClipboardString(window));

  /// Time
  glfw.setTime(1);
  log.info('[getTime]:', glfw.getTime());

  /// Context handling
  glfw.makeContextCurrent(window);
  log.info('getCurrentContext:', glfw.getCurrentContext(window));

  glfw.swapInterval(1);
  // Test manual extension loading if enabled
  if (glfw.extensionSupported != null) {
    if (glfw.extensionSupported('WGL_EXT_swap_control_tear')) {
      log.info('swap control tear is supported');
    }
    log.info(glfw.getProcAddress('glGetDebugMessageLogARB'));
  }

  // Test version binding if enabled
  if (glfw.getVersion != null) {
    log.info(glfw.getVersionString());
    log.info(glfw.getVersion());
  }

  while (glfw.windowShouldClose(window) == false) {
    glfw.pollEvents();
    // Draw something...
    glfw.swapBuffers(window);
  }

} catch(error) {
  log.error(error.stack);
} finally {
  // clean up
  glfw.terminate()
}
