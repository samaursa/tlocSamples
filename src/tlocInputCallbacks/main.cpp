#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>

#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocInput/tloc_input.h>

#include <samplesAssetsPath.h>

using namespace tloc;

class WindowCallback
{
public:
  WindowCallback()
    : m_endProgram(false)
  { }

  core_dispatch::Event 
    OnWindowEvent(const gfx_win::WindowEvent& a_event)
  {
    if (a_event.m_type == gfx_win::WindowEvent::close)
    { m_endProgram = true; }

    return core_dispatch::f_event::Continue();
  }

  bool  m_endProgram;
};
TLOC_DEF_TYPE(WindowCallback);

//------------------------------------------------------------------------
// KeyboardCallback

class KeyboardCallback
{
public:
  KeyboardCallback()
  { }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyPress(const tl_size a_caller,
    const input_hid::KeyboardEvent& a_event)
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %i pressed a key. Key code is: %i", 
                      (tl_int)a_caller, a_event.m_keyCode);

    return core_dispatch::f_event::Continue();
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyRelease(const tl_size a_caller,
    const input_hid::KeyboardEvent& a_event)
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %i released a key. Key code is %i", 
                      (tl_int)a_caller, a_event.m_keyCode);

    return core_dispatch::f_event::Continue();
  }
};
TLOC_DEF_TYPE(KeyboardCallback);

//------------------------------------------------------------------------
// MouseCallback

class MouseCallback
{
public:
  MouseCallback()
  { }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnButtonPress(const tl_size a_caller, 
                  const input_hid::MouseEvent& a_event, 
                  const input_hid::MouseEvent::button_code_type)
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %i pushed a button. Button state is: %i", 
                      (tl_int)a_caller, a_event.m_buttonCode);

    return core_dispatch::f_event::Continue();
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnButtonRelease(const tl_size a_caller, 
                    const input_hid::MouseEvent& a_event, 
                    const input_hid::MouseEvent::button_code_type)
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %i released a button. Button state is %i", 
                      (tl_int)a_caller, a_event.m_buttonCode);

    return core_dispatch::f_event::Continue();
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnMouseMove(const tl_size a_caller, const input_hid::MouseEvent& a_event)
  {
    TLOC_LOG_CORE_INFO() <<
      core_str::Format("Caller %i moved the mouse by %i %i %i ", (tl_int)a_caller, 
                       a_event.m_X.m_rel(), 
                       a_event.m_Y.m_rel(), 
                       a_event.m_Z.m_rel())
                       <<
      core_str::Format(" to %i %i %i", a_event.m_X.m_abs().Value(), 
                       a_event.m_Y.m_abs().Value(), 
                       a_event.m_Z.m_abs().Value());

    return core_dispatch::f_event::Continue();
  }
};
TLOC_DEF_TYPE(MouseCallback);

class TouchCallback
{
public:
  core_dispatch::Event 
    OnTouchPress(const tl_size a_caller, 
                 const input::TouchSurfaceEvent& a_event)
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %i surface touch #%li at %f %f", 
                      (tl_int)a_caller, 
                      a_event.m_touchHandle, 
                      a_event.m_X.m_abs(), a_event.m_Y.m_abs());

    return core_dispatch::f_event::Continue();
  }
  core_dispatch::Event 
    OnTouchRelease(const tl_size a_caller, 
                   const input::TouchSurfaceEvent& a_event)
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %i surface touch release #%li at %f %f", 
                      (tl_int)a_caller, 
                      a_event.m_touchHandle, 
                      a_event.m_X.m_abs(), a_event.m_Y.m_abs());

    return core_dispatch::f_event::Continue();
  }
  core_dispatch::Event 
    OnTouchMove(const tl_size a_caller,
                const input::TouchSurfaceEvent& a_event)
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %i surface touch move #%li at %f %f", 
                      (tl_int)a_caller, 
                      a_event.m_touchHandle, 
                      a_event.m_X.m_abs(), a_event.m_Y.m_abs());

    return core_dispatch::f_event::Continue();
  }
};
TLOC_DEF_TYPE(TouchCallback);

class JoystickCallback
{
public:
  core_dispatch::Event 
    OnButtonPress(const tl_size a_caller, 
                  const input_hid::JoystickEvent& , 
                  tl_int a_buttonIndex) const
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %lu joystick button(%i) press", a_caller, a_buttonIndex);

    return core_dispatch::f_event::Continue();
  }

  core_dispatch::Event 
    OnButtonRelease(const tl_size a_caller,
                    const input_hid::JoystickEvent& , 
                    tl_int a_buttonIndex) const
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %lu joystick button(%i) release", a_caller, a_buttonIndex);

    return core_dispatch::f_event::Continue();
  }

  core_dispatch::Event 
    OnAxisChange(const tl_size a_caller,
                 const input_hid::JoystickEvent& , 
                 tl_int a_axisIndex, 
                 input_hid::JoystickEvent::axis_type a_axis) const
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %lu joystick axis(%i) change: %i, %i, %i", 
      a_caller, a_axisIndex, a_axis[0], a_axis[1], a_axis[2]);

    return core_dispatch::f_event::Continue();
  }

  core_dispatch::Event 
    OnSliderChange(const tl_size a_caller, 
                   const input_hid::JoystickEvent& , 
                   tl_int a_sliderIndex, 
                   input_hid::JoystickEvent::slider_type a_slider) const
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %lu joystick slider(%i) change: %i, %i", 
      a_caller, a_sliderIndex, a_slider[0], a_slider[1]);

    return core_dispatch::f_event::Continue();
  }

  core_dispatch::Event 
    OnPOVChange(const tl_size a_caller, 
                const input_hid::JoystickEvent& , 
                tl_int a_povIndex, 
                input_hid::JoystickEvent::pov_type a_pov) const
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %lu joystick pov(%i) change: %s", 
      a_caller, a_povIndex, a_pov.GetDirectionAsString(a_pov.GetDirection()));

    return core_dispatch::f_event::Continue();
  }
};
TLOC_DEF_TYPE(JoystickCallback);

int TLOC_MAIN(int, char**)
{
  gfx_win::Window win;
  WindowCallback  winCallback;
  win.Register(&winCallback);

  const tl_int winWidth = 800;
  const tl_int winHeight = 600;

  win.Create( gfx_win::Window::graphics_mode::Properties(winWidth, winHeight),
    gfx_win::WindowSettings("Input Callbacks") );

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> params;
  params.m_param1 = win.GetWindowHandle();

  input::input_mgr_b_ptr inputMgr =
    core_sptr::MakeShared<input::InputManagerB>(params);

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::keyboard_b_vptr      keyboard = 
    inputMgr->CreateHID<input_hid::KeyboardB>();
  input_hid::mouse_b_vptr         mouse = 
    inputMgr->CreateHID<input_hid::MouseB>();
  input_hid::touch_surface_b_vptr touchSurface =
    inputMgr->CreateHID<input_hid::TouchSurfaceB>();
  input_hid::joystick_b_vptr      joystick = 
    inputMgr->CreateHID<input_hid::JoystickB>();

  // Check pointers
  TLOC_LOG_CORE_WARN_IF(keyboard == nullptr) << "No keyboard detected";
  TLOC_LOG_CORE_WARN_IF(mouse == nullptr) << "No mouse detected";
  TLOC_LOG_CORE_WARN_IF(touchSurface == nullptr) << "No touchSurface detected";
  TLOC_LOG_CORE_WARN_IF(joystick == nullptr) << "No joystick detected";

  //------------------------------------------------------------------------
  // Creating Keyboard and mouse callbacks and registering them with their
  // respective HIDs

  KeyboardCallback keyboardCallback;
  if (keyboard)
  { keyboard->Register(&keyboardCallback); }

  MouseCallback mouseCallback;
  if (mouse)
  { mouse->Register(&mouseCallback); }

  TouchCallback touchCallback;
  if (touchSurface)
  { touchSurface->Register(&touchCallback); }

  JoystickCallback  joystickCallback;
  if (joystick)
  { joystick->Register(&joystickCallback); }

  //------------------------------------------------------------------------
  // In order to update at a pre-defined time interval, a timer must be created
  core_time::Timer32 inputFrameTime;
  inputFrameTime.Reset();

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    //------------------------------------------------------------------------
    // Update Input
    if (win.IsValid() && inputFrameTime.ElapsedMilliSeconds() > 16)
    {
      inputFrameTime.Reset();

      // This updates all HIDs attached to the InputManager. An InputManager can
      // be updated at a different interval than the main render update.
      inputMgr->Update();

      // Polling if esc key is down, to exit program
      if (keyboard && keyboard->IsKeyDown(input_hid::KeyboardEvent::escape))
      {
        exit(0);
      }
      if (keyboard && keyboard->IsKeyDown(input_hid::KeyboardEvent::h))
      {
        TLOC_LOG_CORE_INFO() << "h key pressed in immediate mode";
      }
      if (keyboard && keyboard->IsKeyDown(input_hid::KeyboardEvent::c))
      {
        if (mouse)
        { mouse->SetClamped(!mouse->IsClamped()); }
      }

      // The InputManager does not need to be reset while in buffered mode as
      // all input is recorded. E.g If user pushes a key down the key will have
      // a state of "down" until the user releases the key, automatically
      // setting it to "up"
      //inputMgr->Reset();

      win.SwapBuffers();
    }
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
