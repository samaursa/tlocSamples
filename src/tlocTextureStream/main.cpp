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

  const tl_int g_imgRows = 100;
  const tl_int g_imgCols = 100;

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The quad render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathVS("/shaders/tlocOneTextureVS_2D.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathFS("/shaders/tlocOneTextureFS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathFS("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
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

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
             gfx_win::WindowSettings("Texture Streaming") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  //
  // NOTES:
  // It MUST be destroyed AFTER the EntityManager(s) that use its components.
  // This is because upon destruction of EntityManagers, all entities are
  // destroyed which trigger events for removal of all their components. If
  // the components are destroyed, the smart pointers will not let us dereference
  // them and will trigger an assertion.
  core_cs::component_pool_mgr_vso compMgr;

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso  eventMgr;
  core_cs::entity_manager_vso entityMgr( MakeArgs(eventMgr.get()) );

  //------------------------------------------------------------------------
  // To render a quad, we need a quad render system - this is a specialized
  // system to render this primitive
  gfx_cs::MeshRenderSystem  quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  // -----------------------------------------------------------------------
  // add noise

  TL_NESTED_FUNC_BEGIN(AddNoise) 
    void AddNoise(gfx_med::image_rgba_vptr a_ptr)
  {
    for (tl_int row = 0; row < g_imgRows; ++row)
    {
      for (tl_int col = 0; col < g_imgCols; ++col)
      {
        tl_float r = core_rng::g_defaultRNG.GetRandomFloat(0.0f, 1.0f);
        tl_float g = core_rng::g_defaultRNG.GetRandomFloat(0.0f, 1.0f);
        tl_float b = core_rng::g_defaultRNG.GetRandomFloat(0.0f, 1.0f);
        tl_float a = core_rng::g_defaultRNG.GetRandomFloat(0.0f, 1.0f);
        a_ptr->SetPixel(row, col, gfx_med::image_rgba::color_type(r, g, b, a));
      }
    }
  }
  TL_NESTED_FUNC_END();

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  typedef gfx_med::image_vso  image_vso;
  image_vso rgba;
  rgba->Create(core_ds::MakeTuple(g_imgRows, g_imgCols),
               gfx_med::Image::color_type::COLOR_BLACK);

  gfx_gl::texture_object_vso to;
  to->Initialize(*rgba);

  gfx_gl::uniform_vso u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  math_t::Rectf32_c rect(math_t::Rectf32_c::width(1.5f),
                         math_t::Rectf32_c::height(1.5f));
  core_cs::entity_vptr q =
    pref_gfx::Quad(entityMgr.get(), compMgr.get())
    .Dimensions(rect).Create();

  pref_gfx::Material(entityMgr.get(), compMgr.get())
    .AddUniform(u_to.get())
    .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
            core_io::Path(GetAssetsPath() + shaderPathFS));

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  quadSys.Initialize();
  matSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    TL_NESTED_CALL(AddNoise)( rgba.get() );
    to->Update(*rgba);

    renderer->ApplyRenderSettings();
    quadSys.ProcessActiveEntities();
    renderer->Render();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
