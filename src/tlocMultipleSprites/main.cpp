#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocInput/tloc_input.h>
#include <tlocPrefab/tloc_prefab.h>

#include <tlocCore/memory/tlocLinkMe.cpp>

#include <samplesAssetsPath.h>

using namespace tloc;

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

  bool OnTouchPress(const tl_size,
                    const input::TouchSurfaceEvent&)
  {
    gfx_cs::texture_animator_vptr ta =
    m_spriteEnt->GetComponent<gfx_cs::TextureAnimator>();

    TLOC_ASSERT_NOT_NULL(ta);

    const tl_size numSprites = ta->GetNumSpriteSequences();
    tl_size currSpriteSet = ta->GetCurrentSpriteSeqIndex();

    ++currSpriteSet;

    if (currSpriteSet == numSprites)
    {
      currSpriteSet = 0;
    }

    ta->SetCurrentSpriteSequence(currSpriteSet);
    return false;
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnTouchRelease(const tl_size,
                      const input::TouchSurfaceEvent&)
  { return false;  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnTouchMove(const tl_size,
                   const input::TouchSurfaceEvent&)
  { return false;  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnKeyPress(const tl_size ,
                  const input_hid::KeyboardEvent& a_event)
  {
    gfx_cs::texture_animator_vptr ta =
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
      const tl_size numSprites = ta->GetNumSpriteSequences();
      tl_size currSpriteSet = ta->GetCurrentSpriteSeqIndex();

      ++currSpriteSet;

      if (currSpriteSet == numSprites)
      {
        currSpriteSet = 0;
      }

      ta->SetCurrentSpriteSequence(currSpriteSet);
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::left)
    {
      const tl_size numSprites = ta->GetNumSpriteSequences();
      tl_size currSpriteSet = ta->GetCurrentSpriteSeqIndex();

      if (currSpriteSet == 0)
      {
        currSpriteSet = numSprites;
      }

      --currSpriteSet;


      ta->SetCurrentSpriteSequence(currSpriteSet);
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::equals)
    {
      const tl_size fps = ta->GetFPS();

      // Note that FPS is converted to a float value by doing 1.0/FPS. This
      // produces rounding errors with the result that adding 1 to the current
      // FPS may not produce any change which is why we add 2
      ta->SetFPS(fps + 2);
      printf("\nNew FPS for SpriteSet #%u: %u",
        ta->GetCurrentSpriteSeqIndex(), ta->GetFPS());
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::minus_main)
    {
      const tl_size fps = ta->GetFPS();

      // See note above for why we -2
      if (fps > 0)
      { ta->SetFPS(fps - 2); }
      printf("\nNew FPS for SpriteSet #%u: %u",
        ta->GetCurrentSpriteSeqIndex(), ta->GetFPS());
    }

    return false;
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnKeyRelease(const tl_size ,
                    const input_hid::KeyboardEvent& )
  { return false; }

private:

  core_cs::entity_vptr m_spriteEnt;

};
TLOC_DEF_TYPE(KeyboardCallback);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
    gfx_win::WindowSettings("Sprite Animation") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { printf("\nGraphics platform failed to initialize"); return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = win.GetRenderer();
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
  ParamList<core_t::Any> kbParams;
  kbParams.m_param1 = win.GetWindowHandle();

  input::input_mgr_b_ptr inputMgr =
    input::input_mgr_b_ptr(new input::InputManagerB(kbParams));

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::KeyboardB* keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();
  input_hid::TouchSurfaceB* touchSurface =
    inputMgr->CreateHID<input_hid::TouchSurfaceB>();

  // Check pointers
  TLOC_ASSERT_NOT_NULL(keyboard);
  TLOC_ASSERT_NOT_NULL(touchSurface);

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  // See explanation in SimpleQuad sample on why it must be created first.
  core_cs::component_pool_mgr_vso cpoolMgr;

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso eventMgr;
  core_cs::entity_manager_vso entityMgr(eventMgr.get());

  //------------------------------------------------------------------------
  // To render a fan, we need a fan render system - this is a specialized
  // system to render this primitive
  gfx_cs::QuadRenderSystem  quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  //------------------------------------------------------------------------
  // TextureAnimation system to animate sprites
  gfx_cs::TextureAnimatorSystem taSys(eventMgr.get(), entityMgr.get());

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  math_t::Rectf32 rect(math_t::Rectf32::width(0.5f),
                        math_t::Rectf32::height(0.5f));
  core_cs::entity_vptr spriteEnt =
    pref_gfx::Quad(entityMgr.get(), cpoolMgr.get())
    .Dimensions(rect)
    .Create();

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The fan render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.

#if defined (TLOC_OS_WIN)
    core_str::String vsPath("/shaders/tlocOneTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String vsPath("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
    core_str::String fsPath("/shaders/tlocOneTextureFS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String fsPath("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
#endif

  // -----------------------------------------------------------------------
  // Load the required resources

  gfx_med::ImageLoaderPng png;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                      "/images/idle_and_spawn.png").c_str() );

  if (png.Load(path) != ErrorSuccess)
  { TLOC_ASSERT_FALSE("Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  gfx_gl::texture_object_vso to;
  to->Initialize(png.GetImage());
  to->Activate();

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(to.get());

  // -----------------------------------------------------------------------
  // create the material from prefab

  pref_gfx::Material matPrefab(entityMgr.get(), cpoolMgr.get());
  matPrefab.AddUniform(u_to.get()).AssetsPath(GetAssetsPath());
  matPrefab.Add(spriteEnt, core_io::Path(vsPath.c_str()),
                core_io::Path(fsPath.c_str()) );

  //------------------------------------------------------------------------
  // also has a sprite sheet loader

  core_str::String spriteSheetDataPath("/misc/idle_and_spawn.txt");
  spriteSheetDataPath = GetAssetsPath() + spriteSheetDataPath;

  core_io::FileIO_ReadA spriteData( core_io::Path(spriteSheetDataPath.c_str()) );

  if (spriteData.Open() != ErrorSuccess)
  { printf("\nUnable to open the sprite sheet"); }

  gfx_med::SpriteLoader_SpriteSheetPacker ssp;
  core_str::String sspContents;
  spriteData.GetContents(sspContents);
  ssp.Init(sspContents, png.GetImage().GetDimensions());

  pref_gfx::SpriteAnimation(entityMgr.get(), cpoolMgr.get()).
    Loop(true).Fps(24).
    Add(spriteEnt, ssp.begin("animation_idle"),
                   ssp.end("animation_idle"));

  pref_gfx::SpriteAnimation(entityMgr.get(), cpoolMgr.get())
    .Loop(true).Fps(24)
    .Add(spriteEnt, ssp.begin("animation_spawn_diffuse"),
                    ssp.end("animation_spawn_diffuse"));

  KeyboardCallback kb(spriteEnt);
  keyboard->Register(&kb);
  touchSurface->Register(&kb);

  spriteEnt.reset();

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  quadSys.Initialize();
  matSys.Initialize();
  taSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop


  printf("\nSprite sheet size: %i, %i",
          ssp.GetDimensions()[0], ssp.GetDimensions()[1]);
  printf("\nImage size: %i, %i",
          png.GetImage().GetWidth(), png.GetImage().GetHeight());

  printf("\n\nP - to toggle pause");
  printf("\nL - to toggle looping");
  printf("\nS - to toggle stop");
  printf("\n= - increase FPS");
  printf("\n- - decrease FPS");

  printf("\n\nRight Arrow - goto next animation sequence");
  printf("\nLeft Arrow  - goto previous animation sequence");

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
      renderer->ApplyRenderSettings();
      taSys.ProcessActiveEntities(deltaT);
      quadSys.ProcessActiveEntities();

      win.SwapBuffers();
      t.Reset();
    }
  }

  //------------------------------------------------------------------------
  // Exiting
  printf("\nExiting normally\n");

  return 0;

}
