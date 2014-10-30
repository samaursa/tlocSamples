#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocMath/tloc_math.inl.h>
#include <tlocPrefab/tloc_prefab.h>

#include <gameAssetsPath.h>

#include <tlocCore/smart_ptr/tloc_smart_ptr.inl.h>
#include <tlocCore/containers/tlocArray.inl.h>

using namespace tloc;

namespace {

  const tl_int g_totalSystems = 10;

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

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
             gfx_win::WindowSettings("Multiple Systems (Simple)") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  using namespace gfx_rend::p_renderer;
  gfx_rend::Renderer::Params p(renderer->GetParams());
  p.AddClearBit<clear::ColorBufferBit>() 
   .SetClearColor(gfx_t::Color(0.5f, 0.5f, 0.5f, 1.0f));
  renderer->SetParams(p);

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.

  core_cs::component_pool_mgr_vso compMgr;

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso  eventMgr;
  core_cs::entity_manager_vso entityMgr( MakeArgs(eventMgr.get()) );

  //------------------------------------------------------------------------
  // To render a quad, we need a quad render system - this is a specialized
  // system to render this primitive

  // we can have multiple systems of the same type - to avoid both systems
  // rendering the same quad, we will use selective dispatch

  typedef core_conts::Array<gfx_cs::quad_render_system_sptr> quad_sys_cont;

  quad_sys_cont quadRenderSystems;

  for (tl_int i = 0; i < g_totalSystems; ++i)
  {
    quadRenderSystems.push_back(core_sptr::MakeShared<gfx_cs::QuadRenderSystem> 
                                (eventMgr.get(), entityMgr.get()));
    quadRenderSystems.back()->SetRenderer(renderer);
  }

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The quad render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.

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

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  for (quad_sys_cont::iterator itr = quadRenderSystems.begin(), 
       itrEnd = quadRenderSystems.end(); itr != itrEnd; ++itr)
  {

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(0.5f),
                           math_t::Rectf32_c::height(0.5f));
    core_cs::entity_vptr q =
      pref_gfx::Quad(entityMgr.get(), compMgr.get())
      .TexCoords(false)
      .Dimensions(rect)
      .DispatchTo((*itr).get())
      .Create();

    tl_float x = core_rng::g_defaultRNG.GetRandomFloat(-0.5f, 0.5f);
    tl_float y = core_rng::g_defaultRNG.GetRandomFloat(-0.5f, 0.5f);
    q->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(x, y, 0));

    pref_gfx::Material(entityMgr.get(), compMgr.get()).
      Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
             core_io::Path(GetAssetsPath() + shaderPathFS));
  }

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  for (quad_sys_cont::iterator itr = quadRenderSystems.begin(), 
       itrEnd = quadRenderSystems.end(); itr != itrEnd; ++itr)
  {
    (*itr)->Initialize();
  }
  matSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop

  TLOC_LOG_DEFAULT_DEBUG() << "Each of the " << g_totalSystems
    << " QuadRenderSystem is disabled for 1 second - thus only 1 quad should "
    << "show per second";

  core_time::Timer t;

  tl_int prevIndex = 0;
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    renderer->ApplyRenderSettings();

    core_time::Timer::sec_type seconds = t.ElapsedSeconds();

    if (seconds > quadRenderSystems.size())
    { t.Reset(); }
    else
    {
      tl_int index = (tl_int)seconds;
      if (index != prevIndex)
      {
        TLOC_LOG_DEFAULT_INFO() << "QuadRenderSystem #" << index 
          << " processing...";
        prevIndex = index;
      }
      quadRenderSystems[index]->ProcessActiveEntities();
    }

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
