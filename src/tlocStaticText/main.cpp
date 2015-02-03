#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocMath/tloc_math.inl.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocInput/tloc_input.h>

#include <gameAssetsPath.h>

using namespace tloc;

namespace {

  const core_str::StringW 
    g_symbols = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ" 
                L"abcdefghijklmnopqrstuvwxyz" 
                L"1234567890!@#$%^&*()_+-=[]" 
                L"{}\\|;:'\",<.>/?`~\n ";

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
  win.Create( gfx_win::Window::graphics_mode::Properties(800, 300),
             gfx_win::WindowSettings("Static Text with Alignment") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  gfx_rend::Renderer::Params p(renderer->GetParams());
  p.Enable<enable_disable::Blend>()
   .SetClearColor(gfx_t::Color::COLOR_WHITE)
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
  gfx_cs::MeshRenderSystem  quadSys(eventMgr.get(), entityMgr.get());
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

  // -----------------------------------------------------------------------
  // Static text render system
  gfx_cs::StaticTextRenderSystem textSys(eventMgr.get(), entityMgr.get());
  textSys.SetRenderer(renderer);

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The quad render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.

  core_io::FileContents vsSource, fsSource;

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

      gfx_gl::uniform_vso u_to;
      u_to->SetName("s_texture").SetValueAs(*to);

      gfx_gl::shader_operator_vso so;
      so->AddUniform(*u_to);

      {
        math_t::Rectf32_c rect(math_t::Rectf32_c::width((f32)win.GetWidth()),
                               math_t::Rectf32_c::height(2.0f));
        
        core_cs::entity_vptr q =
          pref_gfx::Quad(entityMgr.get(), compMgr.get())
          .Dimensions(rect).Create();

        pref_gfx::Material(entityMgr.get(), compMgr.get())
          .AddUniform(u_to.get())
          .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
                  core_io::Path(GetAssetsPath() + shaderPathFS));
      }

      {
        math_t::Rectf32_c rect(math_t::Rectf32_c::width(2.0f),
                               math_t::Rectf32_c::height((f32)win.GetHeight()));
        
        core_cs::entity_vptr q =
          pref_gfx::Quad(entityMgr.get(), compMgr.get())
          .Dimensions(rect).Create();

        pref_gfx::Material(entityMgr.get(), compMgr.get())
          .AddUniform(u_to.get())
          .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
                  core_io::Path(GetAssetsPath() + shaderPathFS));
      }
    }

  //------------------------------------------------------------------------
  // Load the required font

  gfx_med::font_sptr font1 = core_sptr::MakeShared<gfx_med::Font>();
  {
    core_io::Path fontPath( (core_str::String(GetAssetsPath()) +
      "fonts/Qlassik_TB.ttf" ).c_str() );

    core_io::FileIO_ReadB rb(fontPath);
    rb.Open();

    core_str::String fontContents;
    rb.GetContents(fontContents);

    font1->Initialize(fontContents);

    using gfx_med::FontSize;
    FontSize fSize(FontSize::em(12),
                   FontSize::dpi(win.GetDPI()) );

    gfx_med::Font::Params fontParams(fSize);
    fontParams.FontColor(gfx_t::Color(0.0f, 0.0f, 0.0f, 1.0f))
              .BgColor(gfx_t::Color(0.0f, 0.0f, 0.0f, 0.0f))
              .PaddingColor(gfx_t::Color(0.0f, 0.0f, 0.0f, 0.0f))
              .PaddingDim(core_ds::MakeTuple(1, 1));

    font1->GenerateGlyphCache(g_symbols.c_str(), fontParams);
  }

  gfx_med::font_sptr font2 = core_sptr::MakeShared<gfx_med::Font>();
  {
    core_io::Path fontPath( (core_str::String(GetAssetsPath()) +
      "fonts/VeraMono-Bold.ttf" ).c_str() );

    core_io::FileIO_ReadB rb(fontPath);
    rb.Open();

    core_str::String fontContents;
    rb.GetContents(fontContents);

    font2->Initialize(fontContents);

    using gfx_med::FontSize;
    FontSize fSize(FontSize::em(12),
                   FontSize::dpi(win.GetDPI()) );

    gfx_med::Font::Params fontParams(fSize);
    fontParams.FontColor(gfx_t::Color(0.0f, 0.0f, 0.0f, 1.0f))
              .BgColor(gfx_t::Color(0.0f, 0.0f, 0.0f, 0.0f))
              .PaddingColor(gfx_t::Color(0.0f, 0.0f, 0.0f, 0.0f))
              .PaddingDim(core_ds::MakeTuple(1, 1));

    font2->GenerateGlyphCache(g_symbols.c_str(), fontParams);
  }

  // -----------------------------------------------------------------------
  // material will require the correct texture object

  gfx_gl::texture_object_vso toFont1;
  {
    // without specifying the nearest filter, the font will appear blurred in 
    // some cases (especially on smaller sizes)
    gfx_gl::TextureObject::Params toParams;
    toParams.MinFilter<gfx_gl::p_texture_object::filter::Nearest>();
    toParams.MagFilter<gfx_gl::p_texture_object::filter::Nearest>();
    toFont1->SetParams(toParams);

    toFont1->Initialize(*font1->GetSpriteSheetPtr()->GetSpriteSheet());
  }

  gfx_gl::uniform_vso u_toFont1;
  u_toFont1->SetName("s_texture").SetValueAs(*toFont1);

  gfx_gl::texture_object_vso toFont2;
  {
    // without specifying the nearest filter, the font will appear blurred in 
    // some cases (especially on smaller sizes)
    gfx_gl::TextureObject::Params toParams;
    toParams.MinFilter<gfx_gl::p_texture_object::filter::Nearest>();
    toParams.MagFilter<gfx_gl::p_texture_object::filter::Nearest>();
    toFont2->SetParams(toParams);

    toFont2->Initialize(*font2->GetSpriteSheetPtr()->GetSpriteSheet());
  }

  gfx_gl::uniform_vso u_toFont2;
  u_toFont2->SetName("s_texture").SetValueAs(*toFont2);

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  {
    core_cs::entity_vptr ent =
      pref_gfx::StaticText(entityMgr.get(), compMgr.get())
      .Alignment(gfx_cs::alignment::k_align_center)
      .HorizontalAlignment(gfx_cs::horizontal_alignment::k_align_middle)
      .Create(L"-- The quick brown fox jumps over the lazy dog. 1234567890 --", font1);
    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_toFont1.get())
      .Add(ent, vsSource, fsSource);
  }

  core_cs::entity_vptr textNodeAlignLeft =
    pref_gfx::StaticText(entityMgr.get(), compMgr.get())
    .Create(L"Align Left", font1);
  textNodeAlignLeft->GetComponent<math_cs::Transformf32>()->
    SetPosition(math_t::Vec3f32(0.0f, 90.0f, 0));
  pref_gfx::Material(entityMgr.get(), compMgr.get())
    .AddUniform(u_toFont1.get())
    .Add(textNodeAlignLeft, vsSource, fsSource);

  core_cs::entity_vptr textNodeAlignCenter =
    pref_gfx::StaticText(entityMgr.get(), compMgr.get())
    .Create(L"Align Center", font1);
  textNodeAlignCenter->GetComponent<math_cs::Transformf32>()->
    SetPosition(math_t::Vec3f32(0.0f, 60.0f, 0));
  pref_gfx::Material(entityMgr.get(), compMgr.get())
    .AddUniform(u_toFont1.get())
    .Add(textNodeAlignCenter, vsSource, fsSource);

  core_cs::entity_vptr textNodeAlignRight =
    pref_gfx::StaticText(entityMgr.get(), compMgr.get())
    .Create(L"Align Right", font1);
  textNodeAlignRight->GetComponent<math_cs::Transformf32>()->
    SetPosition(math_t::Vec3f32(0.0f, 30.0f, 0));
  pref_gfx::Material(entityMgr.get(), compMgr.get())
    .AddUniform(u_toFont1.get())
    .Add(textNodeAlignRight, vsSource, fsSource);

  {
    core_cs::entity_vptr ent =
      pref_gfx::StaticText(entityMgr.get(), compMgr.get())
        .Alignment(gfx_cs::alignment::k_align_center)
        .Create(L"SkopWorks Inc.", font2);
    ent->GetComponent<math_cs::Transformf32>()
       ->SetPosition(math_t::Vec3f32(0.0f, -30.0f, 0));
    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_toFont2.get())
      .Add(ent, vsSource, fsSource);
  }

  {
    core_cs::entity_vptr ent =
      pref_gfx::StaticText(entityMgr.get(), compMgr.get())
        .Alignment(gfx_cs::alignment::k_align_center)
        .Create(L"", font1);
    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_toFont1.get())
      .Add(ent, vsSource, fsSource);
  }

  {
    core_cs::entity_vptr ent =
      pref_gfx::StaticText(entityMgr.get(), compMgr.get())
        .Alignment(gfx_cs::alignment::k_align_center)
        .Create(L"A", font1);
    ent->GetComponent<math_cs::Transformf32>()
       ->SetPosition(math_t::Vec3f32(0.0f, -60.0f, 0));
    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_toFont1.get())
      .Add(ent, vsSource, fsSource);
  }

  {
    core_cs::entity_vptr ent =
      pref_gfx::StaticText(entityMgr.get(), compMgr.get())
        .Alignment(gfx_cs::alignment::k_align_center)
        .Create(L"Z!", font1);
    ent->GetComponent<math_cs::Transformf32>()
       ->SetPosition(math_t::Vec3f32(0.0f, -80.0f, 0));
    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_toFont1.get())
      .Add(ent, vsSource, fsSource);
  }

  {
    core_cs::entity_vptr ent =
      pref_gfx::StaticText(entityMgr.get(), compMgr.get())
        .Alignment(gfx_cs::alignment::k_align_center)
        .VerticalKerning(-2.5f)
        .Create(L"\nThis sentence\nis a single text\ncomponent split by newlines.\n@\n", font1);
    ent->GetComponent<math_cs::Transformf32>()
       ->SetPosition(math_t::Vec3f32(0.0f, -80.0f, 0));
    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_toFont1.get())
      .Add(ent, vsSource, fsSource);
  }

  // -----------------------------------------------------------------------
  // create a camera

  core_cs::entity_vptr camEnt = 
    pref_gfx::Camera(entityMgr.get(), compMgr.get())
    .Near(0.1f)
    .Far(100.0f)
    .Perspective(false)
    .Position(math_t::Vec3f(0, 0, 1.0f))
    .Create(win.GetDimensions()); 

  quadSys.SetCamera(camEnt);
  textSys.SetCamera(camEnt);

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  TLOC_LOG_CORE_INFO() << "Initializing Quad Render System"; 
  quadSys.Initialize();
  TLOC_LOG_CORE_INFO() << "Initializing Material System"; 
  matSys.Initialize();
  TLOC_LOG_CORE_INFO() << "Initializing Text Render System"; 
  textSys.Initialize();
  TLOC_LOG_CORE_INFO() << "Initializing SceneGraph System"; 
  sgSys.Initialize();
  TLOC_LOG_CORE_INFO() << "Initializing Camera System"; 
  camSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop

  TLOC_LOG_CORE_INFO() << "Setup complete... running main loop";

  core_time::Timer timer;

  bool textAligned = false;
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    renderer->ApplyRenderSettings();
    camSys.ProcessActiveEntities();
    sgSys.ProcessActiveEntities();
    quadSys.ProcessActiveEntities();
    textSys.ProcessActiveEntities();
    renderer->Render();

    if (timer.ElapsedSeconds() > 1.0f && !textAligned)
    {
      TLOC_LOG_CORE_INFO() << "Aligning text...";
      // we purposefully align after the systems are processed at least once
      // to see if alignment is updated during updates
      textNodeAlignLeft->GetComponent<gfx_cs::StaticText>()
        ->SetAlignment(gfx_cs::alignment::k_align_left);
      textNodeAlignRight->GetComponent<gfx_cs::StaticText>()
        ->SetAlignment(gfx_cs::alignment::k_align_right);
      textNodeAlignCenter->GetComponent<gfx_cs::StaticText>()
        ->SetAlignment(gfx_cs::alignment::k_align_center);
      TLOC_LOG_CORE_INFO() << "Aligning text... COMPLETE";

      textAligned = true;
    }

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
