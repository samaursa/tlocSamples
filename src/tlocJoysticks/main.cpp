#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>

#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocInput/tloc_input.h>

#include <gameAssetsPath.h>

using namespace tloc;

namespace {
  // We need a material to attach to our entity (which we have not yet created).

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathVS("/shaders/tlocOneTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathFS("/shaders/tlocOneTextureFS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathFS("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
#endif

  gfx_win::Window g_win;
}

// ///////////////////////////////////////////////////////////////////////
// WindowCallback

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

// ///////////////////////////////////////////////////////////////////////
// JoystickCallback

class JoystickCallback
{
public:
  JoystickCallback(core_cs::entity_vptr a_axis1, core_cs::entity_vptr a_axis2)
    : m_axis1(a_axis1)
    , m_axis2(a_axis2)
  { }

public:
  core_dispatch::Event 
    OnJoystickButtonPress(const tl_size a_caller, 
                          const input_hid::JoystickEvent& , 
                          tl_int a_buttonIndex) const
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %lu joystick button(%i) press", a_caller, a_buttonIndex);

    return core_dispatch::f_event::Continue();
  }

  core_dispatch::Event 
    OnJoystickButtonRelease(const tl_size a_caller,
                            const input_hid::JoystickEvent& , 
                            tl_int a_buttonIndex) const
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %lu joystick button(%i) release", a_caller, a_buttonIndex);

    return core_dispatch::f_event::Continue();
  }

  core_dispatch::Event 
    OnJoystickAxisChange(const tl_size a_caller, 
                         const input_hid::JoystickEvent& , 
                         tl_int a_axisIndex, 
                         input_hid::JoystickEvent::axis_type ,
                         input_hid::JoystickEvent::axis_type_norm a_axisNorm) const
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %lu joystick axis(%i) change: %f, %f, %f", 
      a_caller, a_axisIndex, a_axisNorm[0], a_axisNorm[1], a_axisNorm[2]);

    if (a_axisIndex == 0)
    { 
      m_axis1->GetComponent<math_cs::Transform>()->
        SetPosition(math_t::Vec3f32(a_axisNorm) * 0.5f * (f32)g_win.GetWidth()); 
    }
    else if (a_axisIndex == 1)
    { 
      m_axis2->GetComponent<math_cs::Transform>()->
        SetPosition(math_t::Vec3f32(a_axisNorm) * 0.5f * (f32)g_win.GetWidth()); 
    }

    return core_dispatch::f_event::Continue();
  }

  core_dispatch::Event 
    OnJoystickSliderChange(const tl_size a_caller, 
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
    OnJoystickPOVChange(const tl_size a_caller, 
                        const input_hid::JoystickEvent& , 
                        tl_int a_povIndex, 
                        input_hid::JoystickEvent::pov_type a_pov) const
  {
    TLOC_LOG_CORE_INFO() << 
      core_str::Format("Caller %lu joystick pov(%i) change: %s", 
      a_caller, a_povIndex, a_pov.GetDirectionAsString(a_pov.GetDirection()));

    return core_dispatch::f_event::Continue();
  }

  core_cs::entity_vptr m_axis1;
  core_cs::entity_vptr m_axis2;
};
TLOC_DEF_TYPE(JoystickCallback);

int TLOC_MAIN(int, char**)
{
  WindowCallback  winCallback;
  g_win.Register(&winCallback);

  const tl_int winWidth = 500;
  const tl_int winHeight = 500;

  g_win.Create( gfx_win::Window::graphics_mode::Properties(winWidth, winHeight),
    gfx_win::WindowSettings("Input Callbacks") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; return -1; }

  using namespace gfx_rend::p_renderer;
  auto renderer = g_win.GetRenderer();
  {
    gfx_rend::Renderer::Params p(renderer->GetParams());
    p.SetClearColor(gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f))
      .SetBlendFunction<blend_function::SourceAlpha,
                        blend_function::OneMinusSourceAlpha>()
      .Enable<enable_disable::Blend>()
      .AddClearBit<clear::ColorBufferBit>();

    renderer->SetParams(p);
  }

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> params;
  params.m_param1 = g_win.GetWindowHandle();

  input::input_mgr_b_ptr inputMgr =
    core_sptr::MakeShared<input::InputManagerB>(params);

  core_conts::ArrayFixed<input_hid::joystick_b_vptr, 4> joysticks;

  for (tl_int i = 0; i < 4; ++i)
  {
    auto joystick = inputMgr->CreateHID<input_hid::JoystickB>();
    if (joystick)
    { joysticks.push_back(joystick); }
    else
    { break; }
  }

  if (joysticks.size() == 0)
  { TLOC_LOG_CORE_WARN() << "No joysticks detected"; }
  else
  { TLOC_LOG_CORE_DEBUG() << joysticks.size() << " joysticks detected"; }

  // -----------------------------------------------------------------------

  core_cs::ECS ecs;
  ecs.AddSystem<gfx_cs::CameraSystem>();
  auto quadSys = ecs.AddSystem<gfx_cs::QuadRenderSystem>();
  quadSys->SetRenderer(g_win.GetRenderer());

  ecs.AddSystem<gfx_cs::MaterialSystem>();

  gfx_med::ImageLoaderPng png;
  {
    auto path = core_io::Path( (core_str::String(GetAssetsPath()) + 
                                "/images/target.png").c_str());
    png.Load(path);
  }

  gfx_gl::texture_object_vso to;
  {
    to->Initialize(png.GetImage());
  }

  gfx_gl::uniform_vso u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  // -----------------------------------------------------------------------

  math_t::Rectf32_c rect(math_t::Rectf32_c::width((tl_float)g_win.GetWidth() * 0.05f), 
                         math_t::Rectf32_c::height((tl_float)g_win.GetHeight() * 0.05f));
  auto axis1 = ecs.CreatePrefab<pref_gfx::Quad>().Dimensions(rect).Create();
  auto axis2 = ecs.CreatePrefab<pref_gfx::Quad>().Dimensions(rect).Create();

  ecs.CreatePrefab<pref_gfx::Material>()
     .AddUniform(u_to.get())
     .Add(axis1, core_io::Path(GetAssetsPath() + shaderPathVS),
                 core_io::Path(GetAssetsPath() + shaderPathFS));

  // ideally, we should share the material
  ecs.CreatePrefab<pref_gfx::Material>()
     .AddUniform(u_to.get())
     .Add(axis2, core_io::Path(GetAssetsPath() + shaderPathVS),
                 core_io::Path(GetAssetsPath() + shaderPathFS));

  //------------------------------------------------------------------------
  // Creating Keyboard and mouse callbacks and registering them with their
  // respective HIDs

  JoystickCallback  joystickCallback(axis1, axis2);
  core::for_each_all(joysticks, [&joystickCallback]
                     (input_hid::joystick_b_vptr& joyPtr) 
  {
    joyPtr->Register(&joystickCallback);
  });

  // -----------------------------------------------------------------------
  // create a camera

  auto camEnt = ecs.CreatePrefab<pref_gfx::Camera>() 
                   .Near(0.1f)
                   .Far(100.0f)
                   .Position(math_t::Vec3f(0, 0, 1.0f))
                   .Perspective(false)
                   .Create(g_win.GetDimensions()); 

  quadSys->SetCamera(camEnt);

  // -----------------------------------------------------------------------

  ecs.Initialize();

  //------------------------------------------------------------------------
  // In order to update at a pre-defined time interval, a timer must be created
  core_time::Timer32 inputFrameTime;
  inputFrameTime.Reset();

  //------------------------------------------------------------------------
  // Main loop
  while (g_win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (g_win.GetEvent(evt))
    { }

    //------------------------------------------------------------------------
    // Update Input
    if (g_win.IsValid() && inputFrameTime.ElapsedMilliSeconds() > 16)
    {
      inputFrameTime.Reset();
      inputMgr->Update();

      renderer->ApplyRenderSettings();
      ecs.Process(0.016f);

      g_win.SwapBuffers();
    }
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
