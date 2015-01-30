#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocInput/tloc_input.h>
#include <tlocPrefab/tloc_prefab.h>

#include <gameAssetsPath.h>

TLOC_DEFINE_THIS_FILE_NAME();

using namespace tloc;

gfx_t::Color red(255, 65, 65, 255);
gfx_t::Color green(92, 255, 54, 255);
gfx_t::Color blue(37, 201, 255, 255);
gfx_t::Color yellow(255, 201, 37, 255);
gfx_t::Color violet(186, 37, 255, 255);
gfx_t::Color orange(255, 114, 37, 255);

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

  KeyboardCallback(core_cs::entity_vptr a_spriteEnt, gfx_gl::uniform_vptr a_spriteColor)
    : m_spriteEnt(a_spriteEnt)
    , m_spriteColor(a_spriteColor)
  { }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnTouchPress(const tl_size, const input::TouchSurfaceEvent&)
  {
    gfx_cs::texture_animator_sptr ta =
      m_spriteEnt->GetComponent<gfx_cs::TextureAnimator>();
    
    gfx_cs::texture_animator_sptr ta2 =
      m_spriteEnt->GetComponent<gfx_cs::TextureAnimator>(1);

    TLOC_ASSERT_NOT_NULL(ta);

    const tl_size numSprites = ta->GetNumSpriteSequences();
    tl_size currSpriteSet = ta->GetCurrentSpriteSeqIndex();

    ++currSpriteSet;

    if (currSpriteSet == numSprites)
    {
      currSpriteSet = 0;
    }

    ta->SetCurrentSpriteSequence(currSpriteSet);
    ta2->SetCurrentSpriteSequence(currSpriteSet);

    return core_dispatch::f_event::Continue();
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnTouchRelease(const tl_size, const input::TouchSurfaceEvent&)
  { return core_dispatch::f_event::Continue();  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnTouchMove(const tl_size, const input::TouchSurfaceEvent&)
  { return core_dispatch::f_event::Continue();  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyPress(const tl_size , const input_hid::KeyboardEvent& a_event)
  {
    gfx_cs::texture_animator_sptr ta =
      m_spriteEnt->GetComponent<gfx_cs::TextureAnimator>(0);

    gfx_cs::texture_animator_sptr ta2 =
      m_spriteEnt->GetComponent<gfx_cs::TextureAnimator>(1);

    TLOC_ASSERT_NOT_NULL(ta);

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
      ta2->SetCurrentSpriteSequence(currSpriteSet);
    }

    else if (a_event.m_keyCode == input_hid::KeyboardEvent::left)
    {
      const tl_size numSprites = ta->GetNumSpriteSequences();
      tl_size currSpriteSet = ta->GetCurrentSpriteSeqIndex();

      if (currSpriteSet == 0)
      {
        currSpriteSet = numSprites;
      }

      --currSpriteSet;


      ta->SetCurrentSpriteSequence(currSpriteSet);
      ta2->SetCurrentSpriteSequence(currSpriteSet);
    }

    else if (a_event.m_keyCode == input_hid::KeyboardEvent::n1)
    {
      m_spriteColor->SetValueAs(red);
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::n2)
    {
      m_spriteColor->SetValueAs(green);
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::n3)
    {
      m_spriteColor->SetValueAs(yellow);
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::n4)
    {
      m_spriteColor->SetValueAs(violet);
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::n5)
    {
      m_spriteColor->SetValueAs(orange);
    }

    return core_dispatch::f_event::Continue();
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyRelease(const tl_size , const input_hid::KeyboardEvent& )
  { return core_dispatch::f_event::Continue(); }

private:

  core_cs::entity_vptr m_spriteEnt;
  gfx_gl::uniform_vptr  m_spriteColor;

};
TLOC_DEF_TYPE(KeyboardCallback);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
    gfx_win::WindowSettings("Multiple Texture Coordinates") );

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
   .SetBlendFunction<blend_function::SourceAlpha,
                  blend_function::OneMinusSourceAlpha>()
   .Enable<enable_disable::Blend>()
   .AddClearBit<clear::ColorBufferBit>();

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
  input_hid::keyboard_b_vptr      keyboard = 
    inputMgr->CreateHID<input_hid::KeyboardB>();
  input_hid::touch_surface_b_vptr touchSurface =
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
  core_cs::event_manager_vso  eventMgr;
  core_cs::entity_manager_vso entityMgr( MakeArgs(eventMgr.get()) );

  //------------------------------------------------------------------------
  // To render a fan, we need a fan render system - this is a specialized
  // system to render this primitive
  gfx_cs::MeshRenderSystem  quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);
  //quadSys.SetEnabledUseVBOs(false);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  //------------------------------------------------------------------------
  // TextureAnimation system to animate sprites
  gfx_cs::TextureAnimatorSystem taSys(eventMgr.get(), entityMgr.get());

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  auto winRatio = (f32)win.GetWidth() / (f32)win.GetHeight();
  math_t::Rectf32_c rect(math_t::Rectf32_c::width(0.5f),
                         math_t::Rectf32_c::height(winRatio * 0.5f));
  core_cs::entity_vptr spriteEnt =
    pref_gfx::QuadNoTexCoords(entityMgr.get(), cpoolMgr.get())
    .Sprite(true).Dimensions(rect).Create();

  gfx_cs::texture_coords_sptr tcoord2 =
    core_sptr::MakeShared<gfx_cs::TextureCoords>();
  entityMgr->InsertComponent(core_cs::EntityManager::Params(spriteEnt, tcoord2)
                             .Orphan(true));

  // We need a material to attach to our entity (which we have not yet created).

#if defined (TLOC_OS_WIN)
  core_str::String vsPath("/shaders/tlocOneTextureMultipleTCoordsVS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String vsPath("/shaders/tlocOneTextureMultipleTCoordsVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String fsPath("/shaders/tlocOneTextureMultipleTCoordsFS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String fsPath("/shaders/tlocOneTextureMultipleTCoordsFS_gl_es_2_0.glsl");
#endif

  // -----------------------------------------------------------------------
  // Load the required resources

  gfx_med::ImageLoaderPng png;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                      "/images/blocksprites.png").c_str() );

  if (png.Load(path) != ErrorSuccess)
  { TLOC_ASSERT_FALSE("Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  gfx_gl::texture_object_vso to;
  to->Initialize(*png.GetImage());

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  gfx_gl::uniform_vso u_blockColor;

  u_blockColor->SetName("u_blockColor").SetValueAs(red);

  // -----------------------------------------------------------------------
  // create the material from prefab

  pref_gfx::Material matPrefab(entityMgr.get(), cpoolMgr.get());
  matPrefab.AddUniform(u_to.get()).AssetsPath(GetAssetsPath());
  matPrefab.AddUniform(u_blockColor.get());
  matPrefab.Add(spriteEnt, core_io::Path(vsPath.c_str()),
                core_io::Path(fsPath.c_str()) );

  //------------------------------------------------------------------------
  // also has a sprite sheet loader

  core_str::String spriteSheetDataPath("/misc/blocksprites.xml");
  spriteSheetDataPath = GetAssetsPath() + spriteSheetDataPath;

  core_io::FileIO_ReadA spriteData( (core_io::Path(spriteSheetDataPath)) );

  if (spriteData.Open() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Unable to open the sprite sheet"; }

  gfx_med::SpriteLoader_TexturePacker ssp;
  core_str::String sspContents;
  spriteData.GetContents(sspContents);
  ssp.Init(sspContents, png.GetImage()->GetDimensions());

  const char* spriteNames [] =
  {
    "block_bombcatchblock.png",
    "block_bombcatchnoblock.png",
    "block_bombpiece.png",
    "block_glass.png",
    "block_glassshatter.png",
    "block_glasswall.png",
    "block_glasswall_shatter.png",
    "block_regular.png",
    "block_spawner.png",
    "block_starblock.png",
    "block_starblockcatch.png",
    "block_starpiece.png",
  };

  const char* spriteNamesAlpha [] =
  {
    "block_bombcatchblock_alpha.png",
    "block_none.png",
    "block_none.png",
    "block_glass_alpha.png",
    "block_glassshatter_alpha.png",
    "block_none.png",
    "block_none.png",
    "block_regular_alpha.png",
    "block_none.png",
    "block_starblock_alpha.png",
    "block_starblockcatch_alpha.png",
    "block_none.png",
  };

  TLOC_ASSERT(core_utils::ArraySize(spriteNames) == core_utils::ArraySize(spriteNamesAlpha),
    "Mismatched sprites and alphas");

  for (tl_size i = 0; i < core_utils::ArraySize(spriteNames); ++i)
  {
    pref_gfx::SpriteAnimation(entityMgr.get(), cpoolMgr.get())
      .Fps(24).Paused(true).SetIndex(0) /* 0 is the default index */
      .Add(spriteEnt, ssp.begin(spriteNames[i]), ssp.end(spriteNames[i]));

    pref_gfx::SpriteAnimation(entityMgr.get(), cpoolMgr.get())
      .Fps(24).Paused(true).SetIndex(1)
      .Add(spriteEnt, ssp.begin(spriteNamesAlpha[i]), ssp.end(spriteNamesAlpha[i]));
  }

  // -----------------------------------------------------------------------
  // our uniforms/attributes are always copied - in order to change the
  // material's uniform/attributes we must first get pointers to the
  // uniform/attribute we are looking for

  auto spriteEntMat = spriteEnt->GetComponent<gfx_cs::Material>();
  TLOC_ASSERT(spriteEntMat->size_uniforms() == 2,
    "Unexpected number of shader operators");

  auto uniformItr = spriteEntMat->begin_uniforms();
  ++uniformItr; // block color is the second uniform
  gfx_gl::uniform_vptr blockColorPtr = uniformItr->first.get();
  TLOC_ASSERT(blockColorPtr->GetName().compare("u_blockColor") == 0, 
              "Wrong uniform.");

  KeyboardCallback kb(spriteEnt, blockColorPtr);
  keyboard->Register(&kb);
  touchSurface->Register(&kb);

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  quadSys.Initialize();
  matSys.Initialize();
  taSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop

  TLOC_LOG_CORE_DEBUG() << 
    core_str::Format("Sprite sheet size: %lu, %lu", 
                     ssp.GetDimensions()[0], ssp.GetDimensions()[1]);

  TLOC_LOG_CORE_DEBUG() << 
    core_str::Format("Image size: %lu, %lu", 
                     png.GetImage()->GetWidth(), png.GetImage()->GetHeight());

  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "Right Arrow - goto next animation sequence";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "Left Arrow  - goto previous animation sequence";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "1 to 5 - change sprite color";

  core_time::Timer64 t;

  glClearColor(0.5f, 0.5f, 1.0f, 1.0f);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    inputMgr->Update();

    f64 deltaT = t.ElapsedSeconds();

    if (deltaT > 1.0f/60.0f)
    {
      glClear(GL_COLOR_BUFFER_BIT);
      taSys.ProcessActiveEntities(deltaT);

      renderer->ApplyRenderSettings();
      quadSys.ProcessActiveEntities();
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
