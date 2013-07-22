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
  KeyboardCallback(core_cs::Entity* a_spriteEnt)
    : m_spriteEnt(a_spriteEnt)
  { }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnKeyPress(const tl_size ,
                  const input_hid::KeyboardEvent& a_event)
  {
    gfx_cs::TextureAnimator* ta =
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
      const tl_size numSprites = ta->GetNumSpriteSets();
      tl_size currSpriteSet = ta->GetCurrentSpriteSetIndex();

      ++currSpriteSet;

      if (currSpriteSet == numSprites)
      {
        currSpriteSet = 0;
      }

      ta->SetCurrentSpriteSet(currSpriteSet);
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::left)
    {
      const tl_size numSprites = ta->GetNumSpriteSets();
      tl_size currSpriteSet = ta->GetCurrentSpriteSetIndex();

      if (currSpriteSet == 0)
      {
        currSpriteSet = numSprites;
      }

      --currSpriteSet;


      ta->SetCurrentSpriteSet(currSpriteSet);
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::equals)
    {
      const tl_size fps = ta->GetFPS();

      // Note that FPS is converted to a float value by doing 1.0/FPS. This
      // produces rounding errors with the result that adding 1 to the current
      // FPS may not produce any change which is why we add 2
      ta->SetFPS(fps + 2);
      printf("\nNew FPS for SpriteSet #%u: %u",
        ta->GetCurrentSpriteSetIndex(), ta->GetFPS());
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::minus_main)
    {
      const tl_size fps = ta->GetFPS();

      // See note above for why we -2
      if (fps > 0)
      { ta->SetFPS(fps - 2); }
      printf("\nNew FPS for SpriteSet #%u: %u",
        ta->GetCurrentSpriteSetIndex(), ta->GetFPS());
    }

    return false;
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnKeyRelease(const tl_size ,
                    const input_hid::KeyboardEvent& )
  { return false; }

private:

  core_cs::Entity* m_spriteEnt;

};
TLOC_DEF_TYPE(KeyboardCallback);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
    gfx_win::WindowSettings("tlocTexturedFan") );

  //------------------------------------------------------------------------
  // Initialize renderer
  gfx_rend::Renderer  renderer;
  if (renderer.Initialize() != ErrorSuccess)
  { printf("\nRenderer failed to initialize"); return 1; }

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
  input_hid::MouseB* mouse = inputMgr->CreateHID<input_hid::MouseB>();

  // Check pointers
  TLOC_ASSERT_NOT_NULL(keyboard);
  TLOC_ASSERT_NOT_NULL(mouse);

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_sptr  eventMgr(new core_cs::EventManager());
  core_cs::entity_manager_sptr entityMgr(new core_cs::EntityManager(eventMgr));

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  core_cs::ComponentPoolManager cpoolMgr;

  //------------------------------------------------------------------------
  // To render a fan, we need a fan render system - this is a specialized
  // system to render this primitive
  gfx_cs::QuadRenderSystem  quadSys(eventMgr, entityMgr);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr, entityMgr);

  //------------------------------------------------------------------------
  // TextureAnimation system to animate sprites
  gfx_cs::TextureAnimatorSystem taSys(eventMgr, entityMgr);

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The fan render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.
  gfx_cs::Material  mat;
  {
#if defined (TLOC_OS_WIN)
    core_str::String shaderPath("/shaders/tlocOneTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPath("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

    shaderPath = GetAssetsPath() + shaderPath;
    core_io::FileIO_ReadA shaderFile(shaderPath.c_str());

    if (shaderFile.Open() != ErrorSuccess)
    { printf("\nUnable to open the vertex shader"); return 1;}

    core_str::String code;
    shaderFile.GetContents(code);
    mat.SetVertexSource(code);
  }
  {
#if defined (TLOC_OS_WIN)
    core_str::String shaderPath("/shaders/tlocOneTextureFS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPath("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
#endif

    shaderPath = GetAssetsPath() + shaderPath;
    core_io::FileIO_ReadA shaderFile(shaderPath.c_str());

    if (shaderFile.Open() != ErrorSuccess)
    { printf("\nUnable to open the fragment shader"); return 1;}

    core_str::String code;
    shaderFile.GetContents(code);
    mat.SetFragmentSource(code);
  }

  //------------------------------------------------------------------------
  // Add a texture to the material. We need:
  //  * an image
  //  * a TextureObject (preparing the image for OpenGL)
  //  * a Uniform (all textures are uniforms in shaders)
  //  * a ShaderOperator (this is what the material will take)

  gfx_med::ImageLoaderPng png;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                      "/images/idle_and_spawn.png").c_str() );

  if (png.Load(path) != ErrorSuccess)
  { TLOC_ASSERT(false, "Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  gfx_gl::texture_object_sptr to(new gfx_gl::TextureObject());
  to->Initialize(png.GetImage());

  gfx_gl::uniform_sptr  u_to(new gfx_gl::Uniform());
  u_to->SetName("s_texture").SetValueAs(to);

  gfx_gl::shader_operator_sptr so =
    gfx_gl::shader_operator_sptr(new gfx_gl::ShaderOperator());
  so->AddUniform(u_to);

  // Finally, set this shader operator as the master operator (aka user operator)
  // in our material.
  mat.AddShaderOperator(so);

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  math_t::Rectf32 rect(math_t::Rectf32::width(0.5f),
                        math_t::Rectf32::height(0.5f));
  core_cs::Entity* spriteEnt =
    prefab_gfx::Quad(entityMgr.get(), &cpoolMgr).Dimensions(rect).Create();
  entityMgr->InsertComponent(spriteEnt, &mat);

  //------------------------------------------------------------------------
  // also has a sprite sheet loader

  core_str::String shaderPath("/misc/idle_and_spawn.txt");
  shaderPath = GetAssetsPath() + shaderPath;

  core_io::FileIO_ReadA shaderFile(shaderPath.c_str());

  if (shaderFile.Open() != ErrorSuccess)
  { printf("\nUnable to open the sprite sheet"); }

  gfx_med::SpriteLoader_SpriteSheetPacker ssp;
  core_str::String sspContents;
  shaderFile.GetContents(sspContents);
  ssp.Init(sspContents, gfx_t::Dimension2i(png.GetImage().GetWidth(),
                                           png.GetImage().GetHeight()));

  prefab_gfx::SpriteAnimation(entityMgr.get(), &cpoolMgr).
    Loop(true).Append(true).Fps(24).
    Add(spriteEnt, ssp.begin("animation_idle"),
                   ssp.end("animation_idle"));

  prefab_gfx::SpriteAnimation(entityMgr.get(), &cpoolMgr).
    Loop(true).Append(true).Fps(24).
    Add(spriteEnt, ssp.begin("animation_spawn_diffuse"),
                   ssp.end("animation_spawn_diffuse"));

  KeyboardCallback kb(spriteEnt);
  keyboard->Register(&kb);

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  quadSys.Initialize();
  matSys.Initialize();
  taSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop

  printf("\nP - to toggle pause");
  printf("\nL - to toggle looping");
  printf("\nS - to toggle stop");
  printf("\n= - increase FPS");
  printf("\n- - decrease FPS");

  printf("\n\nRight Aroow - goto next animation sequence");
  printf("\nLeft Aroow  - goto previous animation sequence");

  core_time::Timer64 t;

  glClearColor(0.5f, 0.5f, 1.0f, 1.0f);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
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
      // Finally, process the fan
      taSys.ProcessActiveEntities(deltaT);
      quadSys.ProcessActiveEntities();

      win.SwapBuffers();
      t.Reset();
    }
  }

  //------------------------------------------------------------------------
  // Exiting
  printf("\nExiting normally");

  return 0;

}