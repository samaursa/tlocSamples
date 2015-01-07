#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>

#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocInput/tloc_input.h>

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

int TLOC_MAIN(int, char**)
{
  gfx_win::Window win;
  WindowCallback  winCallback;
  win.Register(&winCallback);

  const tl_int winWidth = 800;
  const tl_int winHeight = 600;

  win.Create( gfx_win::Window::graphics_mode::Properties(winWidth, winHeight),
    gfx_win::WindowSettings("FPS Camera") );

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
          .Disable<enable_disable::DepthTest>() 
          .AddClearBit<clear::DepthBufferBit>();

  gfx_rend::renderer_sptr linesRenderer = 
    core_sptr::MakeShared<gfx_rend::Renderer>(pNoDepth);

  renderer->SetParams(p);

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> params;
  params.m_param1 = win.GetWindowHandle();

  input::input_mgr_i_ptr inputMgr =
    core_sptr::MakeShared<input::InputManagerI>(params);

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::keyboard_i_vptr      keyboard = 
    inputMgr->CreateHID<input_hid::KeyboardI>();
  input_hid::mouse_i_vptr         mouse = 
    inputMgr->CreateHID<input_hid::MouseI>();

  // Check pointers
  TLOC_LOG_CORE_WARN_IF(keyboard == nullptr) << "No keyboard detected";
  TLOC_LOG_CORE_WARN_IF(mouse == nullptr) << "No mouse detected";

  // -----------------------------------------------------------------------
  // ECS

  core_cs::ECS ecs;

  ecs.AddSystem<gfx_cs::MaterialSystem>();
  ecs.AddSystem<gfx_cs::CameraSystem>();

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
  to->Initialize(png.GetImage());

  // -----------------------------------------------------------------------
  // Add a texture to the material. We need:
  //  * an image
  //  * a TextureObject (preparing the image for OpenGL)
  //  * a Uniform (all textures are uniforms in shaders)
  //  * a ShaderOperator (this is what the material will take)

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
      a_ecs.CreatePrefab<pref_gfx::Mesh>().Create(a_vertices);

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
    matPtr->SetEnableUniform<gfx_cs::p_material::Uniforms::k_viewMatrix>();

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

  dtrSys.SetCamera(m_cameraEnt);
  meshSys->SetCamera(m_cameraEnt);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  ecs.Initialize();
  dtrSys.Initialize();

  //------------------------------------------------------------------------
  // In order to update at a pre-defined time interval, a timer must be created
  core_time::Timer32 inputFrameTime;
  inputFrameTime.Reset();

  win.SetMouseVisibility(false);
  win.ConfineMouseToWindow(true);

  f32 pitch = 0.0f;
  f32 yaw = 0.0f;
  bool quit = false;

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram && !quit)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    //------------------------------------------------------------------------
    // Update Input
    if (win.IsValid() && inputFrameTime.ElapsedMilliSeconds() > 16)
    {
      inputFrameTime.Reset();

      // This updates all HIDs attached to the InputManager. An InputManager can
      // be updated at a different interval than the main render update.
      inputMgr->Update();

      // process input
      {
        auto camTrans = m_cameraEnt->GetComponent<math_cs::Transform>();
        auto camDir = camTrans->GetOrientation().GetCol(2).ConvertTo<math_t::Vec3f32>();
        auto camLeft = camTrans->GetOrientation().GetCol(0).ConvertTo<math_t::Vec3f32>();

        camDir *= 0.1f;
        camLeft *= 0.1f;

        // translation
        if (keyboard->IsKeyDown(input_hid::KeyboardEvent::w))
        { camTrans->SetPosition(camTrans->GetPosition() - camDir); }
        if (keyboard->IsKeyDown(input_hid::KeyboardEvent::s))
        { camTrans->SetPosition(camTrans->GetPosition() + camDir); }
        if (keyboard->IsKeyDown(input_hid::KeyboardEvent::a))
        { camTrans->SetPosition(camTrans->GetPosition() - camLeft); }
        if (keyboard->IsKeyDown(input_hid::KeyboardEvent::d))
        { camTrans->SetPosition(camTrans->GetPosition() + camLeft); }
        if (keyboard->IsKeyDown(input_hid::KeyboardEvent::q))
        { quit = true; }

        auto mouseState = mouse->GetState();
        f32 absX = (f32)mouseState.m_X.m_rel();
        f32 absY = (f32)mouseState.m_Y.m_rel();

        pitch += absY * 0.1f;
        yaw += absX * 0.1f;

        auto rotX = math_t::Mat3f32::IDENTITY;
        rotX.MakeRotationX(math_t::Degree(-pitch));

        auto rotY = math_t::Mat3f32::IDENTITY;
        rotY.MakeRotationY(math_t::Degree(-yaw));

        auto finalRot = rotY * rotX;
        camTrans->SetOrientation(finalRot);
      }

      renderer->ApplyRenderSettings();
      ecs.Process(0.016f);
      renderer->Render();

      linesRenderer->ApplyRenderSettings();
      dtrSys.ProcessActiveEntities();

      ecs.Update();

      win.SwapBuffers();
      inputMgr->Reset();
    }
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
