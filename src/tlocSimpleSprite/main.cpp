#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocInput/tloc_input.h>
#include <tlocPrefab/tloc_prefab.h>

#include <gameAssetsPath.h>

TLOC_DEFINE_THIS_FILE_NAME()

using namespace tloc;

gfx_gl::texture_object_vptr
  GetTextureObjectPtr()
{
  static gfx_gl::texture_object_vso g_to;
  return g_to.get();
}

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
  KeyboardCallback(core_cs::entity_vptr a_spriteEnt)
    : m_spriteEnt(a_spriteEnt)
    , m_filterNearest(false)
  { }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyPress(const tl_size , const input_hid::KeyboardEvent& a_event)
  {
    gfx_cs::texture_animator_sptr ta =
      m_spriteEnt->GetComponent<gfx_cs::TextureAnimator>();

    TLOC_ASSERT_NOT_NULL(ta);

    if (a_event.m_keyCode == input_hid::KeyboardEvent::s)
    {
      if (ta->IsStopped())
      { ta->SetStopped(false); }
      else
      { ta->SetStopped(true); }
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::l)
    {
      if (ta->IsLooping())
      { ta->SetLooping(false); }
      else
      { ta->SetLooping(true); }
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::p)
    {
      if (ta->IsPaused())
      { ta->SetPaused(false); }
      else
      { ta->SetPaused(true); }
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::right)
    {
      ta->NextFrame();
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::left)
    {
      ta->PrevFrame();
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::period_main)
    {
      const tl_size fps = ta->GetFPS();

      // Note that FPS is converted to a float value by doing 1.0/FPS. This
      // produces rounding errors with the result that adding 1 to the current
      // FPS may not produce any change which is why we add 2
      ta->SetFPS(fps + 2);
      TLOC_LOG_CORE_INFO() << core_str::Format("New FPS: %lu", ta->GetFPS());
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::comma)
    {
      const tl_size fps = ta->GetFPS();

      // See note above for why we -2
      if (fps > 0)
      { ta->SetFPS(fps - 2); }
      TLOC_LOG_CORE_INFO() << core_str::Format("New FPS: %lu", ta->GetFPS());
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::f)
    {
      gfx_gl::TextureObject::Params texParams = GetTextureObjectPtr()->GetParams();

      if (m_filterNearest)
      {
        m_filterNearest = false;
        texParams.MinFilter<gfx_gl::p_texture_object::filter::Linear>();
        texParams.MagFilter<gfx_gl::p_texture_object::filter::Linear>();
      }
      else
      {
        m_filterNearest = true;
        texParams.MinFilter<gfx_gl::p_texture_object::filter::Nearest>();
        texParams.MagFilter<gfx_gl::p_texture_object::filter::Nearest>();
      }

      GetTextureObjectPtr()->SetParams(texParams);
      GetTextureObjectPtr()->UpdateParameters();
    }

    return core_dispatch::f_event::Continue();
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyRelease(const tl_size , const input_hid::KeyboardEvent& )
  { return core_dispatch::f_event::Continue(); }

private:

  core_cs::entity_vptr  m_spriteEnt;
  bool                  m_filterNearest;

};
TLOC_DEF_TYPE(KeyboardCallback);

int TLOC_MAIN(int argc, char *argv[])
{
  core_str::String imageAndXmlName("prince");

  if (argc == 2)
  { imageAndXmlName = argv[1]; }

  // create correct paths
  core_str::String imagePath("/images/");
  imagePath += imageAndXmlName;
  imagePath += ".png";
  core_str::String xmlPath("/misc/");
  xmlPath += imageAndXmlName;
  xmlPath += ".txt";

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
    gfx_win::WindowSettings("Simple Sprite") );

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
    .SetClearColor(gfx_t::Color(0.5f, 0.5f, 0.5f, 1.0f))
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

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  // See explanation in SimpleQuad sample on why it must be created first.
  core_cs::component_pool_mgr_vso cpoolMgr;

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso eventMgr;
  core_cs::entity_manager_vso entityMgr( MakeArgs(eventMgr.get()) );

  //------------------------------------------------------------------------
  // To render a quad, we need a quad render system - this is a specialized
  // system to render this primitive
  gfx_cs::MeshRenderSystem  quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  //------------------------------------------------------------------------
  // TextureAnimation system to animate sprites
  gfx_cs::TextureAnimatorSystem taSys(eventMgr.get(), entityMgr.get());

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  math_t::Rectf32_c rect(math_t::Rectf32_c::width(1.0f),
                         math_t::Rectf32_c::height(1.0f));
  core_cs::entity_vptr spriteEnt =
    pref_gfx::QuadNoTexCoords(entityMgr.get(), cpoolMgr.get())
    .Sprite(true).Dimensions(rect).Create();

  auto tComp = spriteEnt->GetComponent<math_cs::Transform>();

  // We need a material to attach to our entity (which we have not yet created).
  core_str::String vsPath("/shaders/tlocOneTextureVS.glsl");
  core_str::String fsPath("/shaders/tlocOneTextureFS.glsl");

  gfx_med::ImageLoaderPng png;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                      imagePath).c_str() );

  if (png.Load(path) != ErrorSuccess)
  { TLOC_ASSERT_FALSE("Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  GetTextureObjectPtr()->Initialize(*png.GetImage());

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(*GetTextureObjectPtr());

  pref_gfx::Material matPrefab(entityMgr.get(), cpoolMgr.get());
  matPrefab.AddUniform(u_to.get()).AssetsPath(GetAssetsPath());
  matPrefab.Add(spriteEnt, core_io::Path(vsPath.c_str()),
                           core_io::Path(fsPath.c_str()) );

  //------------------------------------------------------------------------
  // Prefab library also has a sprite sheet loader

  xmlPath = GetAssetsPath() + xmlPath;

  core_io::FileIO_ReadA file( (core_io::Path(xmlPath)) );

  if (file.Open() != ErrorSuccess)
  { 
    TLOC_LOG_GFX_ERR() << "Unable to open the sprite sheet";
  }

  gfx_med::SpriteLoader_SpriteSheetPacker ssp;
  core_str::String                        sspContents;

  file.GetContents(sspContents);
  ssp.Init(sspContents, png.GetImage()->GetDimensions());

  pref_gfx::SpriteAnimation(entityMgr.get(), cpoolMgr.get())
    .Loop(true).Fps(24).Add(spriteEnt, ssp.begin("run"), ssp.end("run"));

  KeyboardCallback kb(spriteEnt);
  keyboard->Register(&kb);

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  quadSys.Initialize();
  matSys.Initialize();
  taSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop

  TLOC_LOG_CORE_DEBUG() << 
    core_str::Format("Sprite sheet size: %li, %li", 
                      ssp.GetDimensions()[0], ssp.GetDimensions()[1]); 
  TLOC_LOG_CORE_DEBUG() << 
    core_str::Format("Image size: %li, %li",
                     png.GetImage()->GetWidth(), png.GetImage()->GetHeight());

  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "P - to toggle pause";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "L - to toggle looping";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "S - to toggle stop";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << ", - increase FPS";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << ". - decrease FPS";

  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "f - toggle MinFilter between Linear and Nearest";

  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "Right Arrow - goto next animation sequence";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "Left Arrow  - goto previous animation sequence";

  core_time::Timer64 t;

  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    inputMgr->Update();

    f64 deltaT = t.ElapsedSeconds();

    if (deltaT > 1.0f/60.0f)
    {
      // Finally, process the fan
      taSys.ProcessActiveEntities(deltaT);
      quadSys.ProcessActiveEntities();

      renderer->ApplyRenderSettings();
      renderer->Render();

      win.SwapBuffers();
      t.Reset();
    }
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
