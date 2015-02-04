#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocInput/tloc_input.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <gameAssetsPath.h>

using namespace tloc;

namespace {

  // -----------------------------------------------------------------------
  // We need a material to attach to our entity (which we have not yet created).

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

class RaypickCallback
  : public input_hid::KeyboardListener
{
public:
  core_dispatch::Event 
    OnRaypickEvent(const gfx_cs::RaypickEvent& a_event)
  {
    TLOC_LOG_DEFAULT_DEBUG() << "Picked Entity: " << *a_event.m_ent;

    return core_dispatch::f_event::Continue();
  }

  core_dispatch::Event 
    OnRayUnpickEvent(const gfx_cs::RaypickEvent& a_event)
  {
    TLOC_LOG_DEFAULT_DEBUG() << "Unpicked Entity: " << *a_event.m_ent;

    return core_dispatch::f_event::Continue();
  }

  core_dispatch::Event 
    OnKeyPress(const tl_size, const input_hid::KeyboardEvent& a_event)
  {
    if (a_event.m_keyCode == input_hid::KeyboardEvent::n1)
    { 
      m_raypickSystem->SetPickingMode(gfx_cs::p_raypick_system::k_on_click);
      TLOC_LOG_DEFAULT_DEBUG() << "Raypicking Mode: On click";
    }
    if (a_event.m_keyCode == input_hid::KeyboardEvent::n2)
    { 
      m_raypickSystem->SetPickingMode(gfx_cs::p_raypick_system::k_continuous_on_click);
      TLOC_LOG_DEFAULT_DEBUG() << "Raypicking Mode: Continuous On Click";
    }
    if (a_event.m_keyCode == input_hid::KeyboardEvent::n3)
    { 
      m_raypickSystem->SetPickingMode(gfx_cs::p_raypick_system::k_continuous);
      TLOC_LOG_DEFAULT_DEBUG() << "Raypicking Mode: Continuous";
    }

    return core_dispatch::f_event::Continue();
  }

  gfx_cs::raypick_system_vptr   m_raypickSystem;
};
TLOC_DEF_TYPE(RaypickCallback);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;
  RaypickCallback rayCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(800, 600),
    gfx_win::WindowSettings("Object File Loader") );

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
   .Enable<enable_disable::DepthTest>()
   .AddClearBit<clear::ColorBufferBit>()
   .AddClearBit<clear::DepthBufferBit>();

  gfx_rend::Renderer::Params pNoDepth(renderer->GetParams());
  pNoDepth.SetClearColor(gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f))
          .Disable<enable_disable::DepthTest>();

  auto linesRenderer = core_sptr::MakeShared<gfx_rend::Renderer>(pNoDepth);

  gfx_rend::Renderer::Params pNoFill(renderer->GetParams());
  pNoFill.Disable<enable_disable::DepthTest>() 
         .SetPointSize(3.0f)
         .PolygonMode<polygon_mode::Point>();

  auto bbRenderer = core_sptr::MakeShared<gfx_rend::Renderer>(pNoFill);

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
  input_hid::keyboard_b_vptr
    keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();
  input_hid::mouse_b_vptr
    mouse = inputMgr->CreateHID<input_hid::MouseB>();
  input_hid::touch_surface_b_vptr
    touchSurface = inputMgr->CreateHID<input_hid::TouchSurfaceB>();

  // Check pointers
  TLOC_ASSERT_NOT_NULL(keyboard);
  TLOC_ASSERT_NOT_NULL(mouse);
  TLOC_ASSERT_NOT_NULL(touchSurface);

  core_cs::ECS ecs;

  // -----------------------------------------------------------------------
  // To render a mesh, we need a mesh render system - this is a specialized
  // system to render this primitive
  ecs.AddSystem<gfx_cs::MaterialSystem>();
  ecs.AddSystem<gfx_cs::CameraSystem>();
  ecs.AddSystem<gfx_cs::ArcBallSystem>();
  auto arcBallControlSystem = ecs.AddSystem<input_cs::ArcBallControlSystem>();

  auto raypickSys = ecs.AddSystem<gfx_cs::RaypickSystem>(1.0/5.0f);
  raypickSys->SetWindowDimensions(win.GetDimensions());
  raypickSys->SetPickingMode(gfx_cs::p_raypick_system::k_on_click);
  raypickSys->Register(&rayCallback);
  rayCallback.m_raypickSystem = raypickSys;

  ecs.AddSystem<gfx_cs::BoundingBoxSystem>();
  auto bbRenderSys = ecs.AddSystem<gfx_cs::BoundingBoxRenderSystem>();
  bbRenderSys->SetRenderer(bbRenderer);

  auto meshSys = ecs.AddSystem<gfx_cs::MeshRenderSystem>();
  meshSys->SetRenderer(renderer);

  // -----------------------------------------------------------------------
  // Transformation debug rendering

  gfx_cs::DebugTransformRenderSystem dtrSys(ecs.GetEventManager(), 
                                            ecs.GetEntityManager());
  dtrSys.SetScale(1.0f);
  dtrSys.SetRenderer(linesRenderer);

  // -----------------------------------------------------------------------
  // Load the required resources

  gfx_med::ImageLoaderPng png;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                       "/images/crateTexture.png").c_str() );

  if (png.Load(path) != ErrorSuccess)
  { TLOC_ASSERT_FALSE("Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  gfx_gl::texture_object_vso to;
  to->Initialize(*png.GetImage());

  // -----------------------------------------------------------------------
  // ObjLoader can load (basic) .obj files

  path = core_io::Path( (core_str::String(GetAssetsPath()) +
                         "/models/Crate.obj").c_str() );

  core_io::FileIO_ReadA objFile(path);
  if (objFile.Open() != ErrorSuccess)
  { 
    TLOC_LOG_GFX_ERR() << "Unable to open the .obj file."; 
    return 1;
  }

  core_str::String objFileContents;
  objFile.GetContents(objFileContents);

  gfx_med::ObjLoader objLoader;
  if (objLoader.Init(objFileContents) != ErrorSuccess)
  { 
    TLOC_LOG_GFX_ERR() << "Parsing errors in .obj file.";
    return 1;
  }

  if (objLoader.GetNumGroups() == 0)
  { 
    TLOC_LOG_GFX_ERR() << "Obj file does not have any objects.";
    return 1;
  }

  gfx_med::ObjLoader::vert_cont_type vertices;
  objLoader.GetUnpacked(vertices, 0);

  // -----------------------------------------------------------------------
  // Create the mesh and add the material

  TL_NESTED_FUNC_BEGIN(CreateMesh) 
    core_cs::entity_vptr 
    CreateMesh( core_cs::ECS& a_ecs,
               const gfx_med::ObjLoader::vert_cont_type& a_vertices,
               const gfx_gl::texture_object_vptr& a_to)

  {
    core_cs::entity_vptr ent =
      a_ecs.CreatePrefab<pref_gfx::Mesh>().Raypick(true).Create(a_vertices);

    gfx_gl::uniform_vso  u_to;
    u_to->SetName("s_texture").SetValueAs(*a_to);

    gfx_gl::uniform_vso  u_lightDir;
    u_lightDir->SetName("u_lightDir").SetValueAs(math_t::Vec3f32(0.2f, 0.5f, 3.0f));

    a_ecs.CreatePrefab<pref_gfx::Material>()
         .AddUniform(u_to.get())
         .AddUniform(u_lightDir.get())
         .Add(ent, core_io::Path(GetAssetsPath() + shaderPathVS),
                   core_io::Path(GetAssetsPath() + shaderPathFS));

    auto matPtr = ent->GetComponent<gfx_cs::Material>();
    matPtr->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewMatrix>();

    auto meshPtr = ent->GetComponent<gfx_cs::Mesh>();
    meshPtr->SetEnableUniform<gfx_cs::p_renderable::uniforms::k_normalMatrix>();

    return ent;
  }
  TL_NESTED_FUNC_END();

  core_cs::entity_ptr_array meshes;
  {
    auto newMesh = TL_NESTED_CALL(CreateMesh)(ecs, vertices, to.get());
    meshes.push_back(newMesh);
  }

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library

  core_cs::entity_vptr m_cameraEnt =
    ecs.CreatePrefab<pref_gfx::Camera>()
    .Perspective(true)
    .Near(1.0f)
    .Far(100.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(0.0f, 0.0f, 5.0f))
    .Create(win.GetDimensions());

  ecs.CreatePrefab<pref_gfx::ArcBall>().Add(m_cameraEnt);
  ecs.CreatePrefab<pref_input::ArcBallControl>()
    .GlobalMultiplier(math_t::Vec2f(0.01f, 0.01f))
    .Add(m_cameraEnt);

  dtrSys.SetCamera(m_cameraEnt);
  meshSys->SetCamera(m_cameraEnt);
  bbRenderSys->SetCamera(m_cameraEnt);
  raypickSys->SetCamera(m_cameraEnt);

  keyboard->Register(&*arcBallControlSystem);
  keyboard->Register(&rayCallback);
  mouse->Register(&*arcBallControlSystem);
  mouse->Register(&*raypickSys);
  touchSurface->Register(&*arcBallControlSystem);
  touchSurface->Register(&*raypickSys);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  ecs.Initialize();
  dtrSys.Initialize();

  // -----------------------------------------------------------------------
  // Main loop

  TLOC_LOG_CORE_DEBUG() << 
    "Press ALT and Left, Middle and Right mouse buttons to manipulate the camera";
  TLOC_LOG_CORE_DEBUG() << "Press L to LookAt() the crate";
  TLOC_LOG_CORE_DEBUG() << "Press F to focus on the crate";
  TLOC_LOG_CORE_DEBUG() << "Press E to enable an INVALID uniform";
  TLOC_LOG_CORE_DEBUG() << "Press D to disable the INVALID uniform";
  TLOC_LOG_CORE_DEBUG() << "Press B to disable bounding box rendering";
  TLOC_LOG_CORE_DEBUG() << "Press C to create more crates";
  TLOC_LOG_CORE_DEBUG() << "Press U to destroy (uncreate) more crates";
  TLOC_LOG_CORE_DEBUG() << "Press 1 to switch to picking with left click";
  TLOC_LOG_CORE_DEBUG() << "Press 2 to switch to continuous picking with left click";
  TLOC_LOG_CORE_DEBUG() << "Press 3 to switch to continuous picking";

  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    // -----------------------------------------------------------------------
    // keyevents

    if (keyboard->IsKeyDown(input_hid::KeyboardEvent::l))
    {
      auto cam = m_cameraEnt->GetComponent<gfx_cs::Camera>();
      cam->LookAt(math_t::Vec3f32(0, 0, 0));
    }
    if (keyboard->IsKeyDown(input_hid::KeyboardEvent::f))
    {
      auto arcball = m_cameraEnt->GetComponent<gfx_cs::ArcBall>();
      arcball->SetFocus(math_t::Vec3f32(0, 0, 0));
    }

    // create and destroy the meshes on a keypress
    if (keyboard->IsKeyDown(input_hid::KeyboardEvent::c))
    {
      auto newMesh = TL_NESTED_CALL(CreateMesh)(ecs, vertices, to.get());
      meshes.push_back(newMesh);

      auto xPos = core_rng::g_defaultRNG.GetRandomFloat(-10.0f, 10.0f);
      auto yPos = core_rng::g_defaultRNG.GetRandomFloat(-10.0f, 10.0f);
      auto zPos = core_rng::g_defaultRNG.GetRandomFloat(-10.0f, 10.0f);

      newMesh->GetComponent<math_cs::Transform>()->
        SetPosition(math_t::Vec3f32(xPos, yPos, zPos));
    }
    if (keyboard->IsKeyDown(input_hid::KeyboardEvent::u))
    {
      if (meshes.size() > 0)
      {
        ecs.GetEntityManager()->DestroyEntity(meshes.front());
        meshes.erase(meshes.begin());
      }

      ecs.GetComponentPoolManager()->RecycleAllUnused();
    }

    // force shader warnings by enabling a uniform that the shader can't handle
    // this is mainly to test whether the render system responds to uniforms
    // being enabled/disabled
    if (meshes.size() > 0)
    {
      auto matPtr = meshes[0]->GetComponent<gfx_cs::Material>();
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::e))
      { matPtr->SetEnableUniform<gfx_cs::p_material::uniforms::k_projectionMatrix>(true); }
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::d))
      { matPtr->SetEnableUniform<gfx_cs::p_material::uniforms::k_projectionMatrix>(false); }
    }

    // -----------------------------------------------------------------------
    // update code

    inputMgr->Update();

    ecs.Update();
    ecs.Process();

    renderer->ApplyRenderSettings();
    renderer->Render();

    linesRenderer->ApplyRenderSettings();
    dtrSys.ProcessActiveEntities();

    if (keyboard->IsKeyDown(input_hid::KeyboardEvent::b) == false)
    {
      bbRenderer->ApplyRenderSettings();
      bbRenderer->Render();
    }

    win.SwapBuffers();
  }

  // -----------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
