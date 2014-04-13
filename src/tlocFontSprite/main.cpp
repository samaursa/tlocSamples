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
                L"{}\\|;:'\",<.>/?`~";

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

class KeyboardCallback
{
public:
  KeyboardCallback(core_cs::entity_vptr a_spriteEnt)
    : m_spriteEnt(a_spriteEnt)
  { }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnKeyPress(const tl_size ,
                  const input_hid::KeyboardEvent& a_event)
  {
    gfx_cs::texture_animator_vptr ta =
      m_spriteEnt->GetComponent<gfx_cs::TextureAnimator>();

    TLOC_ASSERT(ta->GetSpriteSequence(0).GetNumSets() == g_symbols.length(),
                "Not enough sprites for all symbols");

    char8 keyChar = a_event.ToChar();

    for (tl_size i = 0; i < g_symbols.length(); ++i)
    {
      if (core_str::CharWideToAscii(g_symbols.at(i)) == keyChar)
      {
        ta->SetFrame(i);
        break;
      }
    }

    return false;
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnKeyRelease(const tl_size ,
                    const input_hid::KeyboardEvent& )
  { return false; }

private:
  core_cs::entity_vptr  m_spriteEnt;

};
TLOC_DEF_TYPE(KeyboardCallback);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
             gfx_win::WindowSettings("tlocFontSprite") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { printf("\nGraphics platform failed to initialize"); return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> kbParams;
  kbParams.m_param1 = win.GetWindowHandle();

  input::input_mgr_b_ptr inputMgr =
    input::input_mgr_b_ptr(new input::InputManagerB(kbParams));

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::KeyboardB* keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();

  // Check pointers
  TLOC_ASSERT_NOT_NULL(keyboard);

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
  core_cs::entity_manager_vso entityMgr(eventMgr.get());

  //------------------------------------------------------------------------
  // To render a quad, we need a quad render system - this is a specialized
  // system to render this primitive
  gfx_cs::QuadRenderSystem  quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  //------------------------------------------------------------------------
  // TextureAnimation system to animate sprites
  gfx_cs::TextureAnimatorSystem taSys(eventMgr.get(), entityMgr.get());

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The quad render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathVS("/shaders/tlocOneTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathVS("/tlocPassthroughVertexShader_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathFS("/shaders/tlocOneTextureFS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathFS("/tlocPassthroughFragmentShader_gl_es_2_0.glsl");
#endif

  //------------------------------------------------------------------------
  // Load the required resources

  core_io::Path fontPath( (core_str::String(GetAssetsPath()) +
    "Qlassik_TB.ttf" ).c_str() );

  core_io::FileIO_ReadB rb(fontPath);
  rb.Open();

  core_str::String fontContents;
  rb.GetContents(fontContents);

  gfx_med::Font f;
  f.Initialize(fontContents);

  gfx_med::Font::Params_Font fontParams(50);
  fontParams.BgColor(gfx_t::Color(0.1f, 0.1f, 0.1f, 0.1f))
            .PaddingColor(gfx_t::Color(0.0f, 0.5f, 0.0f, 0.2f))
            .PaddingDim(core_ds::MakeTuple(1, 1));

  gfx_med::sprite_sheet_ul_vso fontSs = 
    f.GenerateSpriteSheet(g_symbols.c_str(), fontParams);

  TLOC_LOG_CORE_INFO() 
    << "Char image size: " << fontSs->GetSpriteSheet()->GetWidth() 
    << ", "  << fontSs->GetSpriteSheet()->GetHeight();

  gfx_gl::texture_object_vso to;
  to->Initialize(*fontSs->GetSpriteSheet());
  to->Activate();

  gfx_gl::uniform_vso u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  gfx_gl::shader_operator_vso so;
  so->AddUniform(*u_to);

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  math_t::Rectf32_c rect(math_t::Rectf32_c::width(0.5f),
                         math_t::Rectf32_c::height(0.5f));
  core_cs::entity_vptr q =
    pref_gfx::Quad(entityMgr.get(), compMgr.get()).
                   TexCoords(true).Dimensions(rect).Create();

  pref_gfx::Material(entityMgr.get(), compMgr.get())
    .AddUniform(u_to.get())
    .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
            core_io::Path(GetAssetsPath() + shaderPathFS));

  // -----------------------------------------------------------------------
  // Prefab library also has a sprite sheet loader

  pref_gfx::SpriteAnimation(entityMgr.get(), compMgr.get())
    .Paused(true).Add(q, fontSs->begin(), fontSs->end());

  KeyboardCallback kb(q);
  keyboard->Register(&kb);

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  quadSys.Initialize();
  matSys.Initialize();
  taSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    inputMgr->Update();

    renderer->ApplyRenderSettings();
    taSys.ProcessActiveEntities(1);
    quadSys.ProcessActiveEntities();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  printf("\nExiting normally\n");

  return 0;
}
