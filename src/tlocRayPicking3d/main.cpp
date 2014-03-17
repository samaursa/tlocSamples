#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocInput/tloc_input.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <tlocCore/memory/tlocLinkMe.cpp>

#include <samplesAssetsPath.h>

using namespace tloc;

// -----------------------------------------------------------------------
//

math_t::Cuboidf32 g_cuboid;

// -----------------------------------------------------------------------
// WindowCallback

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

//------------------------------------------------------------------------
// KeyboardCallback

class MayaCam
{
  enum
  {
    k_altPressed = 0,
    k_rotating,
    k_panning,
    k_dolly,
    k_count
  };

public:
  MayaCam(core_cs::entity_vptr a_camera, core_cs::entity_vptr a_cube)
    : m_camera(a_camera)
    , m_flags(k_count)
    , m_cube(a_cube)
  {
    TLOC_ASSERT(a_camera->HasComponent(gfx_cs::components::arcball),
      "Camera does not have ArcBall component");
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnButtonPress(const tl_size ,
                     const input_hid::MouseEvent&,
                     const input_hid::MouseEvent::button_code_type a_button)
  {
    if (a_button == input_hid::MouseEvent::left)
    {
      if (m_flags.IsMarked(k_altPressed))
      { m_flags.Mark(k_rotating); }
      else
      { m_flags.Unmark(k_rotating); }
    }

    if (a_button == input_hid::MouseEvent::middle)
    {
      if (m_flags.IsMarked(k_altPressed))
      { m_flags.Mark(k_panning); }
      else
      { m_flags.Unmark(k_panning); }
    }

    if (a_button == input_hid::MouseEvent::right)
    {
      if (m_flags.IsMarked(k_altPressed))
      { m_flags.Mark(k_dolly); }
      else
      { m_flags.Unmark(k_dolly); }
    }

    return false;
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnButtonRelease(const tl_size ,
                       const input_hid::MouseEvent&,
                       const input_hid::MouseEvent::button_code_type a_button)
  {
    if (a_button == input_hid::MouseEvent::left)
    {
      m_flags.Unmark(k_rotating);
    }

    if (a_button == input_hid::MouseEvent::middle)
    {
      m_flags.Unmark(k_panning);
    }

    if (a_button == input_hid::MouseEvent::right)
    {
      m_flags.Unmark(k_dolly);
    }

    return false;
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnMouseMove(const tl_size ,
                   const input_hid::MouseEvent& a_event)
  {
    f32 xRel = core_utils::CastNumber<f32>(a_event.m_X.m_rel());
    f32 yRel = core_utils::CastNumber<f32>(a_event.m_Y.m_rel());

    if (m_flags.IsMarked(k_rotating))
    {
      gfx_cs::arcball_vptr arcBall = m_camera->GetComponent<gfx_cs::ArcBall>();

      arcBall->MoveVertical(yRel * 0.01f );
      arcBall->MoveHorizontal(xRel * 0.01f );

      return true;
    }
    else if (m_flags.IsMarked(k_panning))
    {
      math_cs::transform_vptr t = m_camera->GetComponent<math_cs::Transform>();
      gfx_cs::arcball_vptr arcBall = m_camera->GetComponent<gfx_cs::ArcBall>();

      math_t::Vec3f32 leftVec; t->GetOrientation().GetCol(0, leftVec);
      math_t::Vec3f32 upVec; t->GetOrientation().GetCol(1, upVec);

      leftVec *= xRel * 0.01f;
      upVec *= yRel * 0.01f;

      t->SetPosition(t->GetPosition() - leftVec + upVec);
      arcBall->SetFocus(arcBall->GetFocus() - leftVec + upVec);

      return true;
    }
    else if (m_flags.IsMarked(k_dolly))
    {
      math_cs::transform_vptr t = m_camera->GetComponent<math_cs::Transform>();

      math_t::Vec3f32 dirVec; t->GetOrientation().GetCol(2, dirVec);

      dirVec *= xRel * 0.01f;

      t->SetPosition(t->GetPosition() - dirVec);

      return true;
    }

    // otherwise, start picking

    CheckCollisionWithRay((f32)a_event.m_X.m_abs().Get(),
                          (f32)a_event.m_Y.m_abs().Get());

    return false;
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnKeyPress(const tl_size ,
                  const input_hid::KeyboardEvent& a_event)
  {
    if (a_event.m_keyCode == input_hid::KeyboardEvent::left_alt)
    {
      m_flags.Mark(k_altPressed);
    }
    return false;
  }

  //------------------------------------------------------------------------
  // Called when a key is released. Currently will printf tloc's representation
  // of the key.
  bool OnKeyRelease(const tl_size ,
                    const input_hid::KeyboardEvent& a_event)
  {
    if (a_event.m_keyCode == input_hid::KeyboardEvent::left_alt)
    {
      m_flags.Unmark(k_altPressed);
    }
    return false;
  }

  void CheckCollisionWithRay(f32 absX, f32 absY)
  {
    using math_utils::scale_f32_f32;
    typedef math_utils::scale_f32_f32::range_small range_small;
    typedef math_utils::scale_f32_f32::range_large range_large;
    using namespace core::component_system;

    //printf("\n%i, %i", a_event.m_X.m_abs(), a_event.m_Y.m_abs());

    range_small smallR(-1.0f, 1.1f);
    range_large largeRX(0.0f, 1024.0f);
    range_large largeRY(0.0f, 768.0f);
    scale_f32_f32 scx(smallR, largeRX);
    scale_f32_f32 scy(smallR, largeRY);

    math_t::Vec3f32 xyz(scx.ScaleDown((f32)(absX) ),
      scy.ScaleDown((f32)(768.0f - absY - 1 )), -1.0f);
    math_t::Ray3f ray =
      m_camera->GetComponent<gfx_cs::Camera>()->GetFrustumRef().GetRay(xyz);

    // Transform with inverse of camera
    math_cs::Transformf32 camTrans =
      *m_camera->GetComponent<math_cs::Transformf32>();
    math_t::Mat4f32 camTransMatInv = camTrans.GetTransformation();

    ray = ray * camTransMatInv;

    // Now start transform the ray from world to fan entity co-ordinates for
    // the intersection test

    ray = ray * m_cube->GetComponent
      <math_cs::Transform>()->Invert().GetTransformation();

    static tl_int intersectionCounter = 0;
    static tl_int nonIntersectionCounter = 0;

    if (g_cuboid.Intersects(ray))
    {
      nonIntersectionCounter = 0;
      ++intersectionCounter;
      if (intersectionCounter == 1)
      {
        printf("\nIntersecting with cube!");
      }
    }
    else
    {
      intersectionCounter = 0;
      ++nonIntersectionCounter;

      if (nonIntersectionCounter == 1)
      {
        printf("\nNOT intersecting with cube!");
      }
    }
  }

  core_cs::entity_vptr    m_camera;
  core_cs::entity_vptr    m_cube;
  core_utils::Checkpoints m_flags;
};
TLOC_DEF_TYPE(MayaCam);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(1024, 768),
    gfx_win::WindowSettings("tlocTexturedFan") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { printf("\nGraphics platform failed to initialize"); return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  gfx_rend::Renderer::Params p(renderer->GetParams());
  p.SetClearColor(gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f))
   .Enable<enable_disable::DepthTest>()
   .AddClearBit<clear::ColorBufferBit>()
   .AddClearBit<clear::DepthBufferBit>();

  renderer->SetParams(p);

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
  // A component pool manager manages all the components in a particular
  // session/level/section.
  // See explanation in SimpleQuad sample on why it must be created first.
  core_cs::component_pool_mgr_vso cpoolMgr;

  // -----------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso eventMgr;
  core_cs::entity_manager_vso entityMgr(eventMgr.get());

  // -----------------------------------------------------------------------
  // To render a mesh, we need a mesh render system - this is a specialized
  // system to render this primitive
  gfx_cs::MeshRenderSystem  meshSys(eventMgr.get(), entityMgr.get());
  meshSys.SetRenderer(renderer);

  // -----------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  // -----------------------------------------------------------------------
  // The camera's view transformations are calculated by the camera system
  gfx_cs::CameraSystem      camSys(eventMgr.get(), entityMgr.get());
  gfx_cs::ArcBallSystem     arcBallSys(eventMgr.get(), entityMgr.get());

  // -----------------------------------------------------------------------
  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The fan render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathVS("/shaders/tlocTexturedMeshVS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathVS("/shaders/tlocTexturedMeshVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathFS("/shaders/tlocTexturedMeshFS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathFS("/shaders/tlocTexturedMeshFS_gl_es_2_0.glsl");
#endif

  // -----------------------------------------------------------------------
  // Load the required resources

  gfx_med::ImageLoaderPng png;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                       "/images/uv_grid_col.png").c_str() );

  if (png.Load(path) != ErrorSuccess)
  { TLOC_ASSERT(false, "Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  gfx_gl::texture_object_vso to;
  to->Initialize(png.GetImage());
  to->Activate();

  // -----------------------------------------------------------------------
  // Add a texture to the material. We need:
  //  * an image
  //  * a TextureObject (preparing the image for OpenGL)
  //  * a Uniform (all textures are uniforms in shaders)
  //  * a ShaderOperator (this is what the material will take)

  // -----------------------------------------------------------------------
  // Create the mesh and add the material

  using namespace tloc::core;

  tl_float posX = rng::g_defaultRNG.GetRandomFloat(-2.0f, 2.0f);
  tl_float posY = rng::g_defaultRNG.GetRandomFloat(-2.0f, 2.0f);
  tl_float posZ = rng::g_defaultRNG.GetRandomFloat(-2.0f, 2.0f);

  printf("\nCube position: %f, %f, %f", posX, posY, posZ);

  using math_t::Cuboidf32;
  g_cuboid = Cuboidf32
    ( (Cuboidf32::width(posX + 2.0f)), (Cuboidf32::height(posY + 2.0f)),
      (Cuboidf32::depth(posZ + 2.0f)) );
  g_cuboid.SetPosition(Cuboidf32::point_type(posX, posY, posZ));

  core_cs::entity_vptr ent =
    prefab_gfx::Cuboid(entityMgr.get(), cpoolMgr.get())
    .Dimensions(g_cuboid)
    .Create();
  ent->GetComponent<math_cs::Transform>()->
    SetPosition(math_t::Vec3f32(posX, posY, posZ));

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  prefab_gfx::Material(entityMgr.get(), cpoolMgr.get()).AddUniform(u_to.get())
    .Add(ent, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library

  math_t::AspectRatio ar(math_t::AspectRatio::width( (tl_float)win.GetWidth()),
    math_t::AspectRatio::height( (tl_float)win.GetHeight()) );
  math_t::FOV fov(math_t::Degree(60.0f), ar, math_t::p_FOV::vertical());

  math_proj::FrustumPersp::Params params(fov);
  params.SetFar(100.0f).SetNear(1.0f);

  math_proj::FrustumPersp fr(params);
  fr.BuildFrustum();

  core_cs::entity_vptr m_cameraEnt =
    prefab_gfx::Camera(entityMgr.get(), cpoolMgr.get()).
    Create(fr, math_t::Vec3f(0.0f, 0.0f, 5.0f));

  prefab_gfx::ArcBall(entityMgr.get(), cpoolMgr.get()).Add(m_cameraEnt);

  meshSys.SetCamera(m_cameraEnt);

  MayaCam mayaCam(m_cameraEnt, ent);
  keyboard->Register(&mayaCam);
  mouse->Register(&mayaCam);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  meshSys.Initialize();
  matSys.Initialize();
  camSys.Initialize();
  arcBallSys.Initialize();

  // -----------------------------------------------------------------------
  // Main loop

  printf("\nPress ALT and Left, Middle and Right mouse buttons to manipulate the camera");

  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    inputMgr->Update();

    arcBallSys.ProcessActiveEntities();
    camSys.ProcessActiveEntities();

    renderer->ApplyRenderSettings();
    meshSys.ProcessActiveEntities();

    win.SwapBuffers();
  }

  // -----------------------------------------------------------------------
  // Exiting
  printf("\nExiting normally\n");

  return 0;

}
