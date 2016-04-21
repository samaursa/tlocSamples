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

  core_str::String shaderPathVS("/shaders/tlocTexturedMeshVS.glsl");
  core_str::String shaderPathFS("/shaders/tlocTexturedMeshBloomFS.glsl");

  core_str::String shaderPathBloomVS("/shaders/tlocBloomVS.glsl");
  core_str::String shaderPathBloomFS("/shaders/tlocBloomFS.glsl");

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

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

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
   .Enable<gfx_rend::p_renderer::enable_disable::CullFace>()
   .Cull<gfx_rend::p_renderer::cull_face::Back>()
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

  core_cs::ECS mainScene;
  core_cs::ECS rttScene;

  // -----------------------------------------------------------------------

  gfx::Rtt rtt(win.GetDimensions());
  auto rttColTo = rtt.AddColorAttachment<0, gfx_t::color_u16_rgba>();
  auto brightTo = rtt.AddColorAttachment<1, gfx_t::color_u16_rgba>();
  rtt.AddDepthAttachment();

  auto rttRenderer = rtt.GetRenderer();
  {
    auto params = rttRenderer->GetParams();
    params.SetClearColor(gfx_t::Color(0, 0, 0, 0));
    rttRenderer->SetParams(params);
  }

  // -----------------------------------------------------------------------
  // To render a mesh, we need a mesh render system - this is a specialized
  // system to render this primitive
  mainScene.AddSystem<gfx_cs::MaterialSystem>("Render");
  mainScene.AddSystem<gfx_cs::CameraSystem>("Render");
  mainScene.AddSystem<gfx_cs::ArcBallSystem>("Update");
  auto arcBallControlSystem = mainScene.AddSystem<input_cs::ArcBallControlSystem>("Update");

  auto meshSys = mainScene.AddSystem<gfx_cs::MeshRenderSystem>("Render");
  meshSys->SetRenderer(rtt.GetRenderer());

  rttScene.AddSystem<gfx_cs::MaterialSystem>("Render");
  auto rttMeshSys = rttScene.AddSystem<gfx_cs::MeshRenderSystem>("Render");
  rttMeshSys->SetRenderer(renderer);

  // -----------------------------------------------------------------------
  // Load the required resources

  gfx_med::ImageLoaderJpeg img;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                       "/images/crateTexture.jpg").c_str() );

  if (img.Load(path) != ErrorSuccess)
  { TLOC_ASSERT_FALSE("Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  gfx_gl::texture_object_vso to;
  to->Initialize(*img.GetImage());

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

  gfx_gl::uniform_vptr u_lightCol;

  TL_NESTED_FUNC_BEGIN(CreateMesh) 
    core_cs::entity_vptr 
    CreateMesh( core_cs::ECS& a_ecs,
               const gfx_med::ObjLoader::vert_cont_type& a_vertices,
               const gfx_gl::texture_object_vptr& a_to,
               gfx_gl::uniform_vptr& a_lightCol)

  {
    static gfx_cs::material_sptr mat;

    core_cs::entity_vptr ent =
      a_ecs.CreatePrefab<pref_gfx::Mesh>().Create(a_vertices);

    gfx_gl::uniform_vso  u_to;
    u_to->SetName("s_texture").SetValueAs(*a_to);

    gfx_gl::uniform_vso  u_lightDir;
    u_lightDir->SetName("u_lightDir").SetValueAs(math_t::Vec3f32(0.2f, 0.5f, 3.0f));

    gfx_gl::uniform_vso  u_lightColor;
    u_lightColor->SetName("u_lightColor").SetValueAs(math_t::Vec3f32(3.5f, 3.5f, 3.5f));

    if (mat == nullptr)
    {
      a_ecs.CreatePrefab<pref_gfx::Material>()
           .AddUniform(u_to.get())
           .AddUniform(u_lightDir.get())
           .AddUniform(u_lightColor.get())
           .Add(ent, core_io::Path(GetAssetsPath() + shaderPathVS),
                     core_io::Path(GetAssetsPath() + shaderPathFS));

      mat = ent->GetComponent<gfx_cs::Material>();

      a_lightCol = gfx_gl::f_shader_operator::GetUniform
        (*mat->GetShaderOperator(), "u_lightColor");
    }
    else
    {
      a_ecs.GetEntityManager()->
        InsertComponent(core_cs::EntityManager::Params(ent, mat));
    }

    auto matPtr = ent->GetComponent<gfx_cs::Material>();
    matPtr->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewMatrix>();

    auto meshPtr = ent->GetComponent<gfx_cs::Mesh>();
    meshPtr->SetEnableUniform<gfx_cs::p_renderable::uniforms::k_normalMatrix>();

    return ent;
  }
  TL_NESTED_FUNC_END();

  core_cs::entity_ptr_array meshes;
  {
    auto newMesh = TL_NESTED_CALL(CreateMesh)(mainScene, vertices, to.get(), u_lightCol);
    meshes.push_back(newMesh);
  }

  // -----------------------------------------------------------------------
  // Quad for rendering the texture

  using math_t::Rectf32_c;
  Rectf32_c rect(Rectf32_c::width(2.0f), Rectf32_c::height(2.0f));

  auto q = rttScene.CreatePrefab<pref_gfx::Quad>().Dimensions(rect).Create();
  {
    gfx_gl::uniform_vso u_rttColTo;
    u_rttColTo->SetName("s_texture").SetValueAs(*rttColTo);

    gfx_gl::uniform_vso u_rttBrightTo;
    u_rttBrightTo->SetName("s_bright").SetValueAs(*brightTo);

    gfx_gl::uniform_vso u_exposure;
    u_exposure->SetName("u_exposure").SetValueAs(0.5f);

    rttScene.CreatePrefab<pref_gfx::Material>()
      .AddUniform(u_rttColTo.get())
      .AddUniform(u_rttBrightTo.get())
      .AddUniform(u_exposure.get())
      .Add(q, core_io::Path(GetAssetsPath() + shaderPathBloomVS),
              core_io::Path(GetAssetsPath() + shaderPathBloomFS));
  }

  auto u_exposure = gfx_gl::f_shader_operator::GetUniform
    (*q->GetComponent<gfx_cs::Material>()->GetShaderOperator(), "u_exposure");

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library

  core_cs::entity_vptr m_cameraEnt =
    mainScene.CreatePrefab<pref_gfx::Camera>()
    .Perspective(true)
    .Near(1.0f)
    .Far(100.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(0.0f, 0.0f, 5.0f))
    .Create(win.GetDimensions());

  mainScene.CreatePrefab<pref_gfx::ArcBall>().Add(m_cameraEnt);
  mainScene.CreatePrefab<pref_input::ArcBallControl>()
    .GlobalMultiplier(math_t::Vec2f(0.01f, 0.01f))
    .Add(m_cameraEnt);

  meshSys->SetCamera(m_cameraEnt);

  keyboard->Register(&*arcBallControlSystem);
  mouse->Register(&*arcBallControlSystem);
  touchSurface->Register(&*arcBallControlSystem);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  mainScene.Initialize();
  rttScene.Initialize();

  // -----------------------------------------------------------------------
  // Main loop

  TLOC_LOG_CORE_DEBUG() << 
    "Press ALT and Left, Middle and Right mouse buttons to manipulate the camera";
  TLOC_LOG_CORE_DEBUG() << "Press L to LookAt() the crate";
  TLOC_LOG_CORE_DEBUG() << "Press F to focus on the crate";
  TLOC_LOG_CORE_DEBUG() << "Press left and right arrow keys to change exposure";

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
    if (keyboard->IsKeyDown(input_hid::KeyboardEvent::left))
    {
      u_exposure->SetValueAs(core::Clamp(u_exposure->GetValueAs<float>() - 0.001f, 0.0f, 10.0f));
    }
    if (keyboard->IsKeyDown(input_hid::KeyboardEvent::right))
    {
      u_exposure->SetValueAs(core::Clamp(u_exposure->GetValueAs<float>() + 0.001f, 0.0f, 10.0f));
    }
    if (keyboard->IsKeyDown(input_hid::KeyboardEvent::up))
    {
      const auto lightVal = u_lightCol->GetValueAs<math_t::Vec3f32>()[0] + 0.001f;
      u_lightCol->SetValueAs(math_t::Vec3f32(lightVal, lightVal, lightVal));
    }
    if (keyboard->IsKeyDown(input_hid::KeyboardEvent::down))
    {
      const auto lightVal = u_lightCol->GetValueAs<math_t::Vec3f32>()[0] - 0.001f;
      u_lightCol->SetValueAs(math_t::Vec3f32(lightVal, lightVal, lightVal));
    }

    // -----------------------------------------------------------------------
    // update code

    inputMgr->Update();

    mainScene.Update(1.0/60.0);
    mainScene.Process(1.0/60.0);

    rttRenderer->ApplyRenderSettings();
    rttRenderer->Render();

    rttScene.Update(1.0/60.0);
    rttScene.Process(1.0/60.0);

    renderer->ApplyRenderSettings();
    renderer->Render();

    win.SwapBuffers();
  }

  // -----------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
