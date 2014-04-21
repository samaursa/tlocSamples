#include <tlocCore/tlocNoOpt.h>

#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocMath/tloc_math.inl.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocInput/tloc_input.h>

#include <samplesAssetsPath.h>

using namespace tloc;

namespace {

  const core_str::StringW 
    g_symbols = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ" 
                L"abcdefghijklmnopqrstuvwxyz" 
                L"1234567890!@#$%^&*()_+-=[]" 
                L"{}\\|;:'\",<.>/?`~ ";

};

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

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(800, 300),
             gfx_win::WindowSettings("Dynamic Text with alignment") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { printf("\nGraphics platform failed to initialize"); return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  gfx_rend::Renderer::Params p(renderer->GetParams());
  p.Enable<enable_disable::Blend>()
   .AddClearBit<clear::ColorBufferBit>()
   .SetBlendFunction<blend_function::SourceAlpha,
                     blend_function::OneMinusSourceAlpha>();
  renderer->SetParams(p);

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
  gfx_cs::QuadRenderSystem  quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  // -----------------------------------------------------------------------
  // SceneNodes require the SceneGraphSystem
  gfx_cs::SceneGraphSystem  sgSys(eventMgr.get(), entityMgr.get());

  // -----------------------------------------------------------------------
  // Camera system
  gfx_cs::CameraSystem      camSys(eventMgr.get(), entityMgr.get());

  //------------------------------------------------------------------------
  // Load the required font

  core_io::Path fontPath( (core_str::String(GetAssetsPath()) +
    "fonts/Qlassik_TB.ttf" ).c_str() );

  core_io::FileIO_ReadB rb(fontPath);
  rb.Open();

  core_str::String fontContents;
  rb.GetContents(fontContents);

  gfx_med::font_sptr f = core_sptr::MakeShared<gfx_med::Font>();
  f->Initialize(fontContents);

  gfx_med::Font::Params fontParams(32);
  fontParams.BgColor(gfx_t::Color(0.0f, 0.0f, 0.0f, 0.0f))
            .PaddingColor(gfx_t::Color(0.0f, 0.0f, 0.0f, 0.0f))
            .PaddingDim(core_ds::MakeTuple(0, 0));

  f->GenerateGlyphCache(g_symbols.c_str(), fontParams);

  // -----------------------------------------------------------------------
  // Text render system
  gfx_cs::dyn_text_render_system_vso 
    textSys( MakeArgs(eventMgr.get(), entityMgr.get(), f) );
  textSys->SetRenderer(renderer);

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The quad render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.

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

  core_str::String vsSource, fsSource;

  {
    core_io::Path vsPath( (GetAssetsPath() + shaderPathVS) );
    core_io::FileIO_ReadA f(vsPath);
    f.Open();
    f.GetContents(vsSource);
  }

  {
    core_io::Path fsPath ( (GetAssetsPath() + shaderPathFS) );
    core_io::FileIO_ReadA f(fsPath);
    f.Open();
    f.GetContents(fsSource);
  }

  // -----------------------------------------------------------------------
  // A thin rectangle signifying the baseline

  gfx_med::Image redPng;
  redPng.Create(core_ds::MakeTuple(2, 2), gfx_t::Color(1.0f, 0.0f, 0.0f, 0.3f));

  {
    gfx_gl::texture_object_vso to;
    to->Initialize(redPng);
    to->Activate();

    gfx_gl::uniform_vso u_to;
    u_to->SetName("s_texture").SetValueAs(*to);

    gfx_gl::shader_operator_vso so;
    so->AddUniform(*u_to);

    {
      math_t::Rectf32_c rect(math_t::Rectf32_c::width((f32)win.GetWidth()),
                             math_t::Rectf32_c::height(2.0f));
      
      core_cs::entity_vptr q =
        pref_gfx::Quad(entityMgr.get(), compMgr.get()).
        TexCoords(true).Dimensions(rect).Create();

      pref_gfx::Material(entityMgr.get(), compMgr.get())
        .AddUniform(u_to.get())
        .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
                core_io::Path(GetAssetsPath() + shaderPathFS));
    }

    {
      math_t::Rectf32_c rect(math_t::Rectf32_c::width(2.0f),
                             math_t::Rectf32_c::height((f32)win.GetHeight()));
      
      core_cs::entity_vptr q =
        pref_gfx::Quad(entityMgr.get(), compMgr.get()).
        TexCoords(true).Dimensions(rect).Create();

      pref_gfx::Material(entityMgr.get(), compMgr.get())
        .AddUniform(u_to.get())
        .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
                core_io::Path(GetAssetsPath() + shaderPathFS));
    }
  }

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  core_cs::entity_vptr dText = 
    pref_gfx::DynamicText(entityMgr.get(), compMgr.get())
    .Alignment(gfx_cs::alignment::k_align_right)
    .Create(L"High Score");

  textSys->SetShaders(vsSource, fsSource);

  // -----------------------------------------------------------------------
  // create a camera

  core_cs::entity_vptr camEnt = 
    pref_gfx::Camera(entityMgr.get(), compMgr.get())
    .Near(0.1f)
    .Far(100.0f)
    .Position(math_t::Vec3f(0, 0, 1.0f))
    .Perspective(false)
    .Create(win.GetDimensions()); 

  quadSys.SetCamera(camEnt);
  textSys->SetCamera(camEnt);

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  TLOC_LOG_CORE_DEBUG() << "Initializing Quad Render System"; 
  quadSys.Initialize();
  TLOC_LOG_CORE_DEBUG() << "Initializing Material System"; 
  matSys.Initialize();
  TLOC_LOG_CORE_DEBUG() << "Initializing Text Render System"; 
  textSys->Initialize();
  TLOC_LOG_CORE_DEBUG() << "Initializing SceneGraph System"; 
  sgSys.Initialize();
  TLOC_LOG_CORE_DEBUG() << "Initializing Camera System"; 
  camSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop

  TLOC_LOG_CORE_DEBUG() << "Setup complete... running main loop";

  core_time::Timer t, tAlign;
  
  //tl_int counter = 0;
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    //if (t.ElapsedSeconds() > 0.01f)
    //{
    //  counter++;

    //  core_str::String numStr = core_str::Format("%i", counter);
    //  core_str::StringW numStrW = core_str::CharAsciiToWide(numStr);

    //  dText->GetComponent<gfx_cs::DynamicText>()->Set(numStrW);
    //  t.Reset();
    //}
    //
    //if (tAlign.ElapsedSeconds() > 1.0f)
    //{
    //  gfx_cs::dynamic_text_sptr dt = 
    //    dText->GetComponent<gfx_cs::DynamicText>(); 

    //  if (dt->GetAlignment() == gfx_cs::alignment::k_align_center)
    //  { dt->SetAlignment(gfx_cs::alignment::k_align_right); }
    //  else 
    //  { dt->SetAlignment(gfx_cs::alignment::k_align_center); }

    //  tAlign.Reset();
    //}

    renderer->ApplyRenderSettings();
    camSys.ProcessActiveEntities();
    sgSys.ProcessActiveEntities();
    quadSys.ProcessActiveEntities();
    textSys->ProcessActiveEntities();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  printf("\nExiting normally\n");

  return 0;
}
