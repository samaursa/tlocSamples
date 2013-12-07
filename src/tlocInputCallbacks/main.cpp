#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>

#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocInput/tloc_input.h>

#include <tlocCore/memory/tlocLinkMe.cpp>

#include <samplesAssetsPath.h>

using namespace tloc;

class WindowCallback
{
public:
  WindowCallback()
    : m_endProgram(false)
  { }

  void OnWindowEvent(const gfx_win::WindowEvent& a_event)
  {
    if (a_event.m_type == gfx_win::WindowEvent::close)
    { m_endProgram = true; }
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

  //------------------------------------------------------------------------
  // Called when a key is pressed. Currently will printf tloc's representation
  // of the key.
  bool OnKeyPress(const tl_size a_caller,
    const input_hid::KeyboardEvent& a_event)
  {
    printf("\nCaller %i pressed a key. Key code is: %i",
           (tl_int)a_caller, a_event.m_keyCode);
    return false;
  }

  //------------------------------------------------------------------------
  // Called when a key is released. Currently will printf tloc's representation
  // of the key.
  bool OnKeyRelease(const tl_size a_caller,
    const input_hid::KeyboardEvent& a_event)
  {
    printf("\nCaller %i released a key. Key code is %i",
           (tl_int)a_caller, a_event.m_keyCode);
    return false;
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

  //------------------------------------------------------------------------
  // Called when a button is pressed. Currently will printf tloc's representation
  // of all buttons.
  bool OnButtonPress(const tl_size a_caller,
                     const input_hid::MouseEvent& a_event,
                     const input_hid::MouseEvent::button_code_type)
  {
    printf("\nCaller %i pushed a button. Button state is: %i",
           (tl_int)a_caller, a_event.m_buttonCode);
    return false;
  }

  //------------------------------------------------------------------------
  // Called when a button is released. Currently will printf tloc's representation
  // of all buttons.
  bool OnButtonRelease(const tl_size a_caller,
                       const input_hid::MouseEvent& a_event,
                       const input_hid::MouseEvent::button_code_type)
  {
    printf("\nCaller %i released a button. Button state is %i",
           (tl_int)a_caller, a_event.m_buttonCode);
    return false;
  }

  //------------------------------------------------------------------------
  // Called when mouse is moved. Currently will printf mouse's relative and
  // absolute position.
  bool OnMouseMove(const tl_size a_caller,
    const input_hid::MouseEvent& a_event)
  {
    printf("\nCaller %i moved the mouse by %i %i %i ", (tl_int)a_caller,
      a_event.m_X.m_rel(),
      a_event.m_Y.m_rel(),
      a_event.m_Z.m_rel());
    printf("to %i %i %i", a_event.m_X.m_abs().Value(),
      a_event.m_Y.m_abs().Value(),
      a_event.m_Z.m_abs().Value());
    return false;
  }
};
TLOC_DEF_TYPE(MouseCallback);

class TouchCallback
{
public:
  bool OnTouchPress(const tl_size a_caller,
                    const input::TouchSurfaceEvent& a_event)
  {
    printf("\nCaller %i surface touch #%li at %f %f",
           (tl_int)a_caller,
           a_event.m_touchHandle, a_event.m_X.m_abs(), a_event.m_Y.m_abs());
    return false;
  }
  bool OnTouchRelease(const tl_size a_caller,
                      const input::TouchSurfaceEvent& a_event)
  {
    printf("\nCaller %i surface touch release #%li at %f %f",
           (tl_int)a_caller,
           a_event.m_touchHandle, a_event.m_X.m_abs(), a_event.m_Y.m_abs());
    return false;
  }
  bool OnTouchMove(const tl_size a_caller,
                   const input::TouchSurfaceEvent& a_event)
  {
    printf("\nCaller %i surface touch move #%li at %f %f",
           (tl_int)a_caller,
           a_event.m_touchHandle, a_event.m_X.m_abs(), a_event.m_Y.m_abs());
    return false;
  }
};
TLOC_DEF_TYPE(TouchCallback);

class JoystickCallback
{
public:
  bool OnButtonPress(const tl_size a_caller,
                     const input_hid::JoystickEvent& ,
                     tl_int a_buttonIndex) const
  {
    printf("\nCaller %i joystick button(%i) press", a_caller, a_buttonIndex);
    return false;
  }

  bool OnButtonRelease(const tl_size a_caller,
                       const input_hid::JoystickEvent& ,
                       tl_int a_buttonIndex) const
  {
    printf("\nCaller %i joystick button(%i) release", a_caller, a_buttonIndex);
    return false;
  }

  bool OnAxisChange(const tl_size a_caller,
                    const input_hid::JoystickEvent& ,
                    tl_int a_axisIndex,
                    input_hid::JoystickEvent::axis_type a_axis) const
  {
    printf("\nCaller %i joystick axis(%i) change: %i, %i, %i", a_caller,
      a_axisIndex, a_axis[0], a_axis[1], a_axis[2]);
    return false;
  }

  bool OnSliderChange(const tl_size a_caller,
                      const input_hid::JoystickEvent& ,
                      tl_int a_sliderIndex,
                      input_hid::JoystickEvent::slider_type a_slider) const
  {
    printf("\nCaller %i joystick slider(%i) change: %i, %i",
            a_caller, a_sliderIndex, a_slider[0], a_slider[1]);
    return false;
  }

  bool OnPOVChange(const tl_size a_caller,
                   const input_hid::JoystickEvent& ,
                   tl_int a_povIndex,
                   input_hid::JoystickEvent::pov_type a_pov) const
  {
    printf("\nCaller %i joystick pov(%i) change: %s",
      a_caller, a_povIndex, a_pov.GetDirectionAsString(a_pov.GetDirection()));
    return false;
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
    gfx_win::WindowSettings("tlocInput") );

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> params;
  params.m_param1 = win.GetWindowHandle();

  input::input_mgr_b_ptr inputMgr =
    input::input_mgr_b_ptr(new input::InputManagerB(params));

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::KeyboardB* keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();
  input_hid::MouseB* mouse = inputMgr->CreateHID<input_hid::MouseB>();
  input_hid::TouchSurfaceB* touchSurface =
    inputMgr->CreateHID<input_hid::TouchSurfaceB>();
  input_hid::JoystickB* joystick = inputMgr->CreateHID<input_hid::JoystickB>();

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
        printf("\nCAN I HAZ CHEEZEBURGERZZ");
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
  printf("\nExiting normally");

}
