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
                L"{}\\|;:'\",<.>/?`~";

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

class KeyboardCallback
{
public:
  KeyboardCallback(core_cs::entity_vptr a_spriteEnt, 
                   core_cs::ecs_vptr a_ecs,
                   gfx_med::const_font_vptr a_font)
    : m_spriteEnt(a_spriteEnt)
    , m_ecs(a_ecs)
    , m_font(a_font)
  { }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyPress(const tl_size , const input_hid::KeyboardEvent& a_event)
  {
    gfx_cs::texture_animator_sptr ta =
      m_spriteEnt->GetComponent<gfx_cs::TextureAnimator>();

    TLOC_ASSERT(ta->GetSpriteSequence(0).GetNumSets() == g_symbols.length(),
                "Not enough sprites for all symbols");

    char8 keyChar = a_event.ToChar();

    for (tl_size i = 0; i < g_symbols.length(); ++i)
    {
      if (core_str::CharWideToAscii(g_symbols.at(i)) == keyChar)
      {
        gfx_med::Font::const_glyph_metrics_iterator 
          itr = m_font->GetGlyphMetric(keyChar);

        math_cs::transform_f32_sptr trans = m_spriteEnt->GetComponent<math_cs::Transformf32>();

        math_t::Rectf32_bl rect(math_t::Rectf32_bl::width((f32)itr->m_dim[0] * 0.01f),
                                math_t::Rectf32_bl::height((f32)itr->m_dim[1] * 0.01f));

        trans->SetPosition(math_t::Vec3f32((f32)itr->m_horizontalBearing[0] * 0.01f,
                                           (f32)itr->m_horizontalBearing[1] * 0.01f - rect.GetHeight(),
                                           0));

        auto spriteMesh = m_spriteEnt->GetComponent<gfx_cs::Mesh>();

        *spriteMesh = *m_ecs->CreatePrefab<pref_gfx::QuadNoTexCoords>()
          .Dimensions(rect).Construct();

        ta->SetFrame(i);
        break;
      }
    }

    return core_dispatch::f_event::Continue();
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyRelease(const tl_size , const input_hid::KeyboardEvent& )
  { return core_dispatch::f_event::Continue(); }

private:
  core_cs::entity_vptr                  m_spriteEnt;
  core_cs::ecs_vptr                     m_ecs;
  gfx_med::const_font_vptr              m_font;

};
TLOC_DEF_TYPE(KeyboardCallback);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
             gfx_win::WindowSettings("Font Sprite") );

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
   .AddClearBit<clear::ColorBufferBit>()
   .SetBlendFunction<blend_function::SourceAlpha,
                     blend_function::OneMinusSourceAlpha>();
  renderer->SetParams(p);

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> kbParams;
  kbParams.m_param1 = win.GetWindowHandle();

  input::input_mgr_b_ptr inputMgr =
    core_sptr::MakeShared<input::InputManagerB>(kbParams);

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::keyboard_b_vptr keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();

  // Check pointers
  TLOC_ASSERT_NOT_NULL(keyboard);

  core_cs::ecs_vso textECS;

  textECS->AddSystem<gfx_cs::MaterialSystem>();
  textECS->AddSystem<gfx_cs::TextureAnimatorSystem>();
  auto  meshSys = textECS->AddSystem<gfx_cs::MeshRenderSystem>();
  meshSys->SetRenderer(renderer);

  // -----------------------------------------------------------------------
  // A thin rectangle signifying the baseline

    gfx_med::Image redPng;
    redPng.Create(core_ds::MakeTuple(2, 2), gfx_t::Color(1.0f, 0.0f, 0.0f, 1.0f));

    {
      gfx_gl::texture_object_vso to;
      to->Initialize(redPng);

      gfx_gl::uniform_vso u_to;
      u_to->SetName("s_texture").SetValueAs(*to);

      gfx_gl::shader_operator_vso so;
      so->AddUniform(*u_to);

      {
        math_t::Rectf32_c rect(math_t::Rectf32_c::width(1.0f),
                               math_t::Rectf32_c::height(0.01f));
        
        core_cs::entity_vptr q = textECS->CreatePrefab<pref_gfx::Quad>()
          .Dimensions(rect).Create();

        textECS->CreatePrefab<pref_gfx::Material>()
          .AddUniform(u_to.get())
          .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
                  core_io::Path(GetAssetsPath() + shaderPathFS));
      }

      {
        math_t::Rectf32_c rect(math_t::Rectf32_c::width(0.01f),
                               math_t::Rectf32_c::height(1.0f));
        
        core_cs::entity_vptr q = textECS->CreatePrefab<pref_gfx::Quad>()
          .Dimensions(rect).Create();

        textECS->CreatePrefab<pref_gfx::Material>()
          .AddUniform(u_to.get())
          .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
                  core_io::Path(GetAssetsPath() + shaderPathFS));
      }
    }

  //------------------------------------------------------------------------
  // Load the required resources

  core_io::Path fontPath( (core_str::String(GetAssetsPath()) +
    "fonts/VeraMono-Bold.ttf" ).c_str() );

  core_io::FileIO_ReadB rb(fontPath);
  rb.Open();

  core_str::String fontContents;
  rb.GetContents(fontContents);

  gfx_med::font_vso f;
  f->Initialize(fontContents);

  using gfx_med::FontSize;
  FontSize fSize(FontSize::pixels(50));

  gfx_med::Font::Params fontParams(fSize);
  fontParams.BgColor(gfx_t::Color(0.1f, 0.1f, 0.1f, 0.7f))
            .PaddingColor(gfx_t::Color(0.0f, 0.5f, 0.0f, 0.7f))
            .PaddingDim(core_ds::MakeTuple(1, 1));

  gfx_med::const_sprite_sheet_ul_vptr fontSs = 
    f->GenerateGlyphCache(g_symbols.c_str(), fontParams);

  TLOC_LOG_CORE_INFO() 
    << "Char image size: " << fontSs->GetSpriteSheet()->GetWidth() 
    << ", "  << fontSs->GetSpriteSheet()->GetHeight();

  gfx_gl::texture_object_vso to;
  to->Initialize(*fontSs->GetSpriteSheet());

  gfx_gl::uniform_vso u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  gfx_gl::shader_operator_vso so;
  so->AddUniform(*u_to);

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  math_t::Rectf32_bl rect(math_t::Rectf32_bl::width(0.25f),
                         math_t::Rectf32_bl::height(0.25f));
  core_cs::entity_vptr q =
    textECS->CreatePrefab<pref_gfx::QuadNoTexCoords>()
    .Sprite(true).Dimensions(rect).Create();

  textECS->CreatePrefab<pref_gfx::Material>()
    .AddUniform(u_to.get())
    .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
            core_io::Path(GetAssetsPath() + shaderPathFS));

  // -----------------------------------------------------------------------
  // Prefab library also has a sprite sheet loader

  using gfx_med::algos::compare::sprite_info::MakeName;

  textECS->CreatePrefab<pref_gfx::SpriteAnimation>()
    .Paused(true).Add(q, fontSs->begin(), fontSs->end() );

  KeyboardCallback kb(q, textECS.get(), f.get());
  keyboard->Register(&kb);

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  textECS->Initialize();

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    inputMgr->Update();

    textECS->Process(0.0);

    renderer->ApplyRenderSettings();
    renderer->Render();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
