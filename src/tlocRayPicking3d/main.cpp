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

//------------------------------------------------------------------------
// KeyboardCallback

class Raypicker
{
public:
  Raypicker(core_cs::entity_vptr a_camera, core_cs::entity_vptr a_cube)
    : m_camera(a_camera)
    , m_cube(a_cube)
  {
    TLOC_ASSERT(a_camera->HasComponent(gfx_cs::components::arcball),
      "Camera does not have ArcBall component");
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnButtonPress(const tl_size , 
                  const input_hid::MouseEvent&, 
                  const input_hid::MouseEvent::button_code_type )
  { return core_dispatch::f_event::Continue(); }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnButtonRelease(const tl_size , 
                    const input_hid::MouseEvent&, 
                    const input_hid::MouseEvent::button_code_type )
  { return core_dispatch::f_event::Continue(); }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnMouseMove(const tl_size , const input_hid::MouseEvent& a_event)
  {
    CheckCollisionWithRay((f32)a_event.m_X.m_abs().Get(),
                          (f32)a_event.m_Y.m_abs().Get());

    return core_dispatch::f_event::Continue();
  }

  void CheckCollisionWithRay(f32 absX, f32 absY)
  {
    using math_utils::scale_f32_f32;
    typedef math_utils::scale_f32_f32::range_small range_small;
    typedef math_utils::scale_f32_f32::range_large range_large;
    using namespace core::component_system;

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
        TLOC_LOG_CORE_INFO() << "Intersecting with cube!";
      }
    }
    else
    {
      intersectionCounter = 0;
      ++nonIntersectionCounter;

      if (nonIntersectionCounter == 1)
      {
        TLOC_LOG_CORE_INFO() << "NOT intersecting with cube!";
      }
    }
  }

  core_cs::entity_vptr    m_camera;
  core_cs::entity_vptr    m_cube;
};
TLOC_DEF_TYPE(Raypicker);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(1024, 768),
    gfx_win::WindowSettings("Raypicking 3D") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { 
    TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; 
    return -1;
  }

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
    core_sptr::MakeShared<input::InputManagerB>(kbParams);

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::keyboard_b_vptr keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();
  input_hid::mouse_b_vptr    mouse = inputMgr->CreateHID<input_hid::MouseB>();

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
  core_cs::entity_manager_vso entityMgr( MakeArgs(eventMgr.get()) );

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
  input_cs::ArcBallControlSystem arcBallControlSys(eventMgr.get(), entityMgr.get());

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
  { TLOC_ASSERT_FALSE("Image did not load!"); }

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

  TLOC_LOG_CORE_INFO() << 
    core_str::Format("Cube position: %f, %f, %f", posX, posY, posZ);

  using math_t::Cuboidf32;
  g_cuboid = Cuboidf32
    ( (Cuboidf32::width(posX + 2.0f)), (Cuboidf32::height(posY + 2.0f)),
      (Cuboidf32::depth(posZ + 2.0f)) );
  g_cuboid.SetPosition(Cuboidf32::point_type(posX, posY, posZ));

  core_cs::entity_vptr ent =
    pref_gfx::Cuboid(entityMgr.get(), cpoolMgr.get())
    .Dimensions(g_cuboid)
    .Create();
  ent->GetComponent<math_cs::Transform>()->
    SetPosition(math_t::Vec3f32(posX, posY, posZ));

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  pref_gfx::Material(entityMgr.get(), cpoolMgr.get()).AddUniform(u_to.get())
    .Add(ent, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library

  core_cs::entity_vptr m_cameraEnt =
    pref_gfx::Camera(entityMgr.get(), cpoolMgr.get())
    .Near(1.0f)
    .Far(100.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(0.0f, 0.0f, 10.0f))
    .Create(win.GetDimensions());

  pref_gfx::ArcBall(entityMgr.get(), cpoolMgr.get()).Add(m_cameraEnt);
  pref_input::ArcBallControl(entityMgr.get(), cpoolMgr.get())
    .GlobalMultiplier(math_t::Vec2f(0.01f, 0.01f))
    .Add(m_cameraEnt);

  meshSys.SetCamera(m_cameraEnt);

  keyboard->Register(&arcBallControlSys);
  mouse->Register(&arcBallControlSys);

  // Raypicker MUST be registered AFTER ArcBallControlSys to ensure that
  // camera movement does not also result in picking
  Raypicker rayPicker(m_cameraEnt, ent);
  mouse->Register(&rayPicker);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  meshSys.Initialize();
  matSys.Initialize();
  camSys.Initialize();
  arcBallSys.Initialize();
  arcBallControlSys.Initialize();

  // -----------------------------------------------------------------------
  // Main loop

  TLOC_LOG_CORE_DEBUG() << 
    "Press ALT and Left, Middle and Right mouse buttons to manipulate the camera";

  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    inputMgr->Update();

    arcBallControlSys.ProcessActiveEntities();
    arcBallSys.ProcessActiveEntities();
    camSys.ProcessActiveEntities();

    renderer->ApplyRenderSettings();
    meshSys.ProcessActiveEntities();

    win.SwapBuffers();
  }

  // -----------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
