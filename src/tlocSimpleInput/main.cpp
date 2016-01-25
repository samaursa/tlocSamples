#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>

#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocInput/tloc_input.h>

#include <gameAssetsPath.h>

using namespace tloc;

namespace {

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathVS("/tlocPassthroughVertexShader.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathVS("/tlocPassthroughVertexShader_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathFS("/tlocPassthroughFragmentShader.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathFS("/tlocPassthroughFragmentShader_gl_es_2_0.glsl");
#endif

};

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

void MoveEntity(const core_cs::entity_vptr a_ent, const math_t::Vec2f& a_deltaPos)
{
  math_cs::transform_sptr transform = a_ent->GetComponent<math_cs::Transform>();

  math_cs::Transform::position_type position(transform->GetPosition());
  position[0] += a_deltaPos[0];
  position[1] += a_deltaPos[1];

  transform->SetPosition(position);
}

void MoveEntityToPosition(const core_cs::entity_vptr a_ent, const math_t::Vec2f& a_position)
{
  math_cs::transform_sptr transform = a_ent->GetComponent<math_cs::Transform>();
  transform->SetPosition(a_position.ConvertTo<math_t::Vec3f>());
}

int TLOC_MAIN(int , char *[])
{
  gfx_win::Window win;
  WindowCallback  winCallback;
  win.Register(&winCallback);

  win.Create( gfx_win::Window::graphics_mode::Properties(800, 600),
    gfx_win::WindowSettings("Simple Input") );

  const tl_size winWidth = win.GetWidth();
  const tl_size winHeight = win.GetHeight();

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  gfx_rend::Renderer::Params p(renderer->GetParams());
  p.SetClearColor(gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f))
   .AddClearBit<clear::ColorBufferBit>();

  renderer->SetParams(p);

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> params;
  params.m_param1 = win.GetWindowHandle();

  input::input_mgr_i_ptr inputMgr =
    core_sptr::MakeShared<input::InputManagerI>(params);

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::keyboard_i_vptr keyboard = 
    inputMgr->CreateHID<input_hid::KeyboardI>();
  input_hid::mouse_i_vptr mouse = 
    inputMgr->CreateHID<input_hid::MouseI>();
  input_hid::touch_surface_i_vptr touchSurface =
    inputMgr->CreateHID<input_hid::TouchSurfaceI>();

  // Create a container that we will save our touches in
  input_hid::TouchSurfaceI::touch_container_type currentTouches;

  // Check pointers
  TLOC_ASSERT_NOT_NULL(keyboard);
  TLOC_ASSERT_NOT_NULL(mouse);
  TLOC_ASSERT_NOT_NULL(touchSurface);

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  // See explanation in SimpleQuad sample on why it must be created first.
  core_cs::component_pool_mgr_vso compMgr;

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso  eventMgr;
  core_cs::entity_manager_vso entityMgr(eventMgr.get());

  //------------------------------------------------------------------------
  // To render a quad, we need a quad render system - this is a specialized
  // system to render this primitive
  gfx_cs::MeshRenderSystem quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem matSys(eventMgr.get(), entityMgr.get());

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us
  math_t::Rectf32_c rect(math_t::Rectf32_c::width(0.5f),
                         math_t::Rectf32_c::height(0.5f));
  core_cs::entity_vptr ent = 
    pref_gfx::QuadNoTexCoords(entityMgr.get(), compMgr.get())
    .Dimensions(rect).Create();

  pref_gfx::Material(entityMgr.get(), compMgr.get())
    .Add(ent, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  quadSys.Initialize();
  matSys.Initialize();

  //------------------------------------------------------------------------
  // In order to update at a pre-defined time interval, a timer must be created
  core_time::Timer32 renderFrameTime;
  renderFrameTime.Reset();
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

      // Polling if arrow keys are down and moving the entity with a helper
      // function
      const tl_float deltaPos = 0.01f;
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::left))
      {
        MoveEntity(ent, math_t::Vec2f(-deltaPos, 0.0f));
      }
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::right))
      {
        MoveEntity(ent, math_t::Vec2f(deltaPos, 0.0f));
      }
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::up))
      {
        MoveEntity(ent, math_t::Vec2f(0.0f, deltaPos));
      }
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::down))
      {
        MoveEntity(ent, math_t::Vec2f(0.0f, -deltaPos));
      }
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::escape))
      {
        exit(0);
      }
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::h))
      {
        TLOC_LOG_CORE_DEBUG() << "h key pressed in immediate mode";
      }

      // Polling the mouse's absolute position, using that value to move the quad
      if (mouse->IsButtonDown(input_hid::MouseEvent::left) &&
          mouse->IsButtonDown(input_hid::MouseEvent::right))
      {
        MoveEntityToPosition(ent, math_t::Vec2f(0.0f, 0.0f));
      }
      else if (mouse->IsButtonDown(input_hid::MouseEvent::left))
      {
        input_hid::MouseEvent mouseState = mouse->GetState();

        tl_float xScaled = core_utils::CastNumber<tl_float>
        (mouseState.m_X.m_abs);
        tl_float yScaled = core_utils::CastNumber<tl_float>
        (mouseState.m_Y.m_abs);

        xScaled /= core_utils::CastNumber<tl_float>(winWidth);
        yScaled /= core_utils::CastNumber<tl_float>(winHeight);

        xScaled = (xScaled * 2) - 1;
        yScaled = (yScaled * 2) - 1;

        // mouse co-ords start from top-left, OpenGL from bottom-left
        yScaled *= -1;

        MoveEntityToPosition(ent, math_t::Vec2f(xScaled, yScaled));
      }

      currentTouches = touchSurface->GetCurrentTouches();

      if (currentTouches.size() == 1)
      {
        tl_float xScaled = currentTouches[0].m_X.m_abs;
        tl_float yScaled = currentTouches[0].m_Y.m_abs;

        xScaled /= core_utils::CastNumber<tl_float>(winWidth);
        yScaled /= core_utils::CastNumber<tl_float>(winHeight);

        xScaled = (xScaled * 2.0f) - 1.0f;
        yScaled = (yScaled * 2.0f) - 1.0f;

        yScaled *= -1;

        math_t::Vec2f scaledPosition(xScaled, yScaled);

        MoveEntityToPosition(ent, scaledPosition);
      }
      else if (currentTouches.size() == 2)
      {
        MoveEntityToPosition(ent, math_t::Vec2f(0.0f, 0.0f));
      }


      // The Immediate mode InputManager needs to be reset to reset all
      // key/button states to their default. E.g. If user pushes a key down the
      // key will remain "down" until the user resets the InputManager.
      inputMgr->Reset();
    }

    //------------------------------------------------------------------------
    // Update the QuadRenderSystem and Window
    if (win.IsValid() && renderFrameTime.ElapsedMilliSeconds() > 16)
    {
      renderFrameTime.Reset();

      renderer->ApplyRenderSettings();
      quadSys.ProcessActiveEntities();
      renderer->Render();

      win.SwapBuffers();
    }
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
