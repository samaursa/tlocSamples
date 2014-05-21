#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>

#include <tlocCore/memory/tlocLinkMe.cpp>

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

    if (a_event.m_type == gfx_win::WindowEvent::gained_focus)
    { TLOC_LOG_CORE_INFO() << "Window just gained focus"; }

    if (a_event.m_type == gfx_win::WindowEvent::lost_focus)
    { TLOC_LOG_CORE_INFO() << "Window just lost focus"; }

    if (a_event.m_type == gfx_win::WindowEvent::resized)
    { TLOC_LOG_CORE_INFO() << "Window just resized"; }

    return core_dispatch::f_event::Continue();
  }

  bool  m_endProgram;
};
TLOC_DEF_TYPE(WindowCallback);

int main()
{
  gfx_win::Window win;
  WindowCallback  winCallback;
  win.Register(&winCallback);

  win.Create( gfx_win::Window::graphics_mode::Properties(800, 600),
              gfx_win::WindowSettings("Window Only") );

  while (win.IsValid() && !winCallback.m_endProgram)
  {
    // We MUST pool for window events, otherwise it will appear to be frozen
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    win.SwapBuffers();
  }
}
