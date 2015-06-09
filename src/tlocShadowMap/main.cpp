#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocInput/tloc_input.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <gameAssetsPath.h>

using namespace tloc;

namespace {

  tl_size g_rttImgWidth   = 1024;
  tl_size g_rttImgHeight  = 1024;

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
    gfx_win::WindowSettings("Shadow Mapping") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  auto_cref renderer = win.GetRenderer();
  {
    using namespace gfx_rend::p_renderer;
    auto_cref commonParams = GetParamsCommon
      (renderer->GetParams().GetFBO(), renderer->GetParams().GetDimensions(), 
       gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f));
    renderer->SetParams(commonParams);
  }

  gfx_rend::renderer_sptr linesRenderer;
  {
    using namespace gfx_rend::p_renderer;
    auto_cref noDepthParams = GetParamsCommonNoDepthNoColorClear
      (renderer->GetParams().GetFBO(), renderer->GetParams().GetDimensions());
    linesRenderer =
      core_sptr::MakeShared<gfx_rend::Renderer>(noDepthParams);
  }

  // -----------------------------------------------------------------------
  // Prepare RTT

  gfx::Rtt  rtt(core_ds::MakeTuple(g_rttImgWidth, g_rttImgHeight));
  auto depthTo      = rtt.AddShadowDepthAttachment();
  auto rttTo        = rtt.AddColorAttachment(0);
  auto rttRenderer  = rtt.GetRenderer();
  {
    using namespace gfx_rend::p_renderer;
    auto_cref shadowParams = GetParamsShadow
      (rttRenderer->GetParams().GetFBO(), rttRenderer->GetParams().GetDimensions());
    rttRenderer->SetParams(shadowParams);
  }

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

  // -----------------------------------------------------------------------
  // Scene

  core_cs::ECS ecsMainScene;
  core_cs::ECS ecsRttQuad;

  auto arcBallControlSystem = ecsMainScene.AddSystem<input_cs::ArcBallControlSystem>();
  ecsMainScene.AddSystem<gfx_cs::ArcBallSystem>();
  auto camSys = ecsMainScene.AddSystem<gfx_cs::CameraSystem>(1.0 / 60.0, true);
  auto matSys = ecsMainScene.AddSystem<gfx_cs::MaterialSystem>();

  auto  meshSys = ecsMainScene.AddSystem<gfx_cs::MeshRenderSystem>();
  meshSys->SetRenderer(renderer);

  ecsRttQuad.AddSystem<gfx_cs::MaterialSystem>();
  auto  rttMeshSys = ecsRttQuad.AddSystem<gfx_cs::MeshRenderSystem>();
  rttMeshSys->SetRenderer(renderer);

  // -----------------------------------------------------------------------
  // Transformation debug rendering

  auto dtrSys = ecsMainScene.AddSystem<gfx_cs::DebugTransformRenderSystem>(1.0 / 60.0, true);
  dtrSys->SetScale(1.0f);
  dtrSys->SetRenderer(linesRenderer);

  // -----------------------------------------------------------------------
  // We need a material to attach to our entity (which we have not yet created).

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathMeshVS("/shaders/tlocTexturedMeshShadowVS.glsl");
    core_str::String shaderPathQuadVS("/shaders/tlocOneTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathMeshVS("/shaders/tlocTexturedMeshShadowVS_gl_es_2_0.glsl");
    core_str::String shaderPathQuadVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathMeshFS("/shaders/tlocTexturedMeshShadowFS.glsl");
    core_str::String shaderPathQuadFS("/shaders/tlocOneTextureNoFlipFS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathMeshFS("/shaders/tlocTexturedMeshShadowFS_gl_es_2_0.glsl");
    core_str::String shaderPathQuadFS("/shaders/tlocOneTextureNoFlipFS_gl_es_2_0.glsl");
#endif

  // -----------------------------------------------------------------------
  // Create the quad and add the material

  using math_t::Rectf32_c;
  Rectf32_c rectDim(Rectf32_c::width(0.5f), Rectf32_c::height(0.5f));

  core_cs::entity_vptr rttQuad = 
    ecsRttQuad.CreatePrefab<pref_gfx::Quad>()
    .Dimensions(rectDim)
    .Create();

  rttQuad->GetComponent<math_cs::Transform>()->
    SetPosition(math_t::Vec3f32(1.0f - rectDim.GetWidth() * 0.5f, 
                                1.0f - rectDim.GetHeight() * 0.5f, 
                                0.0f));

  gfx_gl::uniform_vso u_toRtt;
  u_toRtt->SetName("s_texture").SetValueAs(*rttTo);

  ecsRttQuad.CreatePrefab<pref_gfx::Material>()
    .AddUniform(u_toRtt.get())
    .Add(rttQuad, core_io::Path(GetAssetsPath() + shaderPathQuadVS),
                  core_io::Path(GetAssetsPath() + shaderPathQuadFS));

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library - this will be our scene camera

  core_cs::entity_vptr m_cameraEnt =
    ecsMainScene.CreatePrefab<pref_gfx::Camera>()
    .Perspective(true)
    .Near(1.0f)
    .Far(100.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(0.0f, 0.0f, 5.0f))
    .Create(win.GetDimensions());

  // -----------------------------------------------------------------------
  // Create another camera, this will be what the light sees

  core_cs::entity_vptr m_lightCamera =
    ecsMainScene.CreatePrefab<pref_gfx::Camera>()
    .Perspective(true)
    .Near(1.0f)
    .Far(20.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(4.0f, 4.0f, 4.0f))
    .Create(win.GetDimensions());

  m_lightCamera->GetComponent<gfx_cs::Camera>()->
    LookAt(math_t::Vec3f32::ZERO);

  ecsMainScene.CreatePrefab<pref_gfx::ArcBall>().Add(m_cameraEnt);
  ecsMainScene.CreatePrefab<pref_input::ArcBallControl>()
    .GlobalMultiplier(math_t::Vec2f(0.01f, 0.01f))
    .Add(m_cameraEnt);

  dtrSys->SetCamera(m_cameraEnt);
  meshSys->SetCamera(m_cameraEnt);

  keyboard->Register(arcBallControlSystem.get());
  mouse->Register(arcBallControlSystem.get());
  touchSurface->Register(arcBallControlSystem.get());

  // -----------------------------------------------------------------------
  // Load a grayscale texture

  gfx_gl::texture_object_vso crateTextureTo;
  {
    gfx_med::ImageLoaderPng png;
    core_io::Path path(( core_str::String(GetAssetsPath()) +
      "/images/crateTexture.png" ).c_str());

    if (png.Load(path) != ErrorSuccess)
    { TLOC_ASSERT_FALSE("Image did not load!"); }

    // gl::Uniform supports quite a few types, including a TextureObject
    crateTextureTo->Initialize(*png.GetImage());
  }

  gfx_gl::texture_object_vso to;
  {
    gfx_med::Image grayCol;
    grayCol.Create(core_ds::MakeTuple(2, 2),
                   gfx_med::Image::color_type::COLOR_GREY);

    // gl::Uniform supports quite a few types, including a TextureObject
    to->Initialize(grayCol);
  }

  // -----------------------------------------------------------------------
  // ObjLoader can load (basic) .obj files

  core_str::String objFileContents;
  {
    core_io::Path path = core_io::Path(( core_str::String(GetAssetsPath()) +
      "/models/Crate.obj" ).c_str());

    core_io::FileIO_ReadA objFile(path);
    if (objFile.Open() != ErrorSuccess)
    { TLOC_LOG_GFX_ERR() << "Unable to open the .obj file."; return 1; }

    objFile.GetContents(objFileContents);
  }

  gfx_med::ObjLoader::vert_cont_type vertices;
  {
    gfx_med::ObjLoader objLoader;
    if (objLoader.Init(objFileContents) != ErrorSuccess)
    { TLOC_LOG_GFX_ERR() << "Parsing errors in .obj file."; return 1; }

    if (objLoader.GetNumGroups() == 0)
    { TLOC_LOG_GFX_ERR() << "Obj file does not have any objects."; return 1; }

    objLoader.GetUnpacked(vertices, 0);
  }

  // -----------------------------------------------------------------------
  // For shadow maps, we need the MVP matrix of the light with a scaling 
  // matrix

  camSys->Initialize();
  camSys->ProcessActiveEntities();

  math_cs::Transform::transform_type lightCamTrans = 
    m_lightCamera->GetComponent<math_cs::Transform>()->GetTransformation();

  const math_t::Vec3f32 lightDir = 
    lightCamTrans.GetCol(2).ConvertTo<math_t::Vec3f32>();

  // the bias matrix - this is need to convert the NDC coordinates to texture
  // space coordinates (i.e. from -1,1 to 0,1).
  // to put it another way: v_screen = v_NDC * 0.5 + 0.5;
  const math_t::Mat4f32 lightMVPBias(0.5f, 0.0f, 0.0f, 0.5f,
                                     0.0f, 0.5f, 0.0f, 0.5f,
                                     0.0f, 0.0f, 0.5f, 0.5f,
                                     0.0f, 0.0f, 0.0f, 1.0f);

  gfx_cs::Camera::matrix_type lightMVP = 
    m_lightCamera->GetComponent<gfx_cs::Camera>()->GetViewProjRef();

  lightMVP = lightMVPBias * lightMVP;

  // -----------------------------------------------------------------------
  // Create the mesh and add the material

  core_cs::entity_vptr crateEnt =
    ecsMainScene.CreatePrefab<pref_gfx::Mesh>().Create(vertices);

  crateEnt->GetComponent<gfx_cs::Mesh>()->
    SetEnableUniform<gfx_cs::p_renderable::uniforms::k_normalMatrix>();

  using math_t::Cuboidf32;
  Cuboidf32 cubSize(Cuboidf32::width(10.0f),
                    Cuboidf32::height(0.1f), 
                    Cuboidf32::depth(10.0f));

  core_cs::entity_vptr floorMesh = 
    ecsMainScene.CreatePrefab<pref_gfx::Cuboid>()
    .Dimensions(cubSize)
    .Create();

  floorMesh->GetComponent<gfx_cs::Mesh>()->
    SetEnableUniform<gfx_cs::p_renderable::uniforms::k_normalMatrix>();

  floorMesh->GetComponent<math_cs::Transform>()->
    SetPosition(math_t::Vec3f32(0, -2.0f, 0));

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(*crateTextureTo);

  gfx_gl::uniform_vso  u_toShadowMap;
  u_toShadowMap->SetName("s_shadowMap").SetValueAs(*depthTo);

  gfx_gl::uniform_vso  u_lightDir;
  u_lightDir->SetName("u_lightDir").SetValueAs(lightDir);

  gfx_gl::uniform_vso  u_lightMVP;
  u_lightMVP->SetName("u_lightMVP").SetValueAs(lightMVP);

  gfx_gl::uniform_vso u_imgDim;
  u_imgDim->SetName("u_imgDim")
    .SetValueAs(math_t::Vec2f32((f32)g_rttImgWidth, (f32)g_rttImgHeight));

  auto meshMat = ecsMainScene.CreatePrefab<pref_gfx::Material>();
  meshMat.AddUniform(u_to.get())
         .AddUniform(u_lightDir.get())
         .AddUniform(u_toShadowMap.get()) 
         .AddUniform(u_lightMVP.get())
         .AddUniform(u_imgDim.get());

  meshMat.Add(crateEnt, core_io::Path(GetAssetsPath() + shaderPathMeshVS), 
                   core_io::Path(GetAssetsPath() + shaderPathMeshFS));

  auto crateMat = crateEnt->GetComponent<gfx_cs::Material>();
  crateMat->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewMatrix>(true);

  // change the texture to grayscale - over-riding does not change the previously
  // applied material because the uniforms are copied
  u_to->SetValueAs(*to);

  meshMat.Add(floorMesh, core_io::Path(GetAssetsPath() + shaderPathMeshVS), 
                         core_io::Path(GetAssetsPath() + shaderPathMeshFS));

  auto floorMat = floorMesh->GetComponent<gfx_cs::Material>();
  floorMat->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewMatrix>(true);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  ecsMainScene.Initialize();
  ecsRttQuad.Initialize();
  dtrSys->Initialize();

  // -----------------------------------------------------------------------
  // Main loop

  TLOC_LOG_CORE_DEBUG() << 
    "Press ALT and Left, Middle and Right mouse buttons to manipulate the camera";

  math_t::Degree d(0.0f);

  core_time::Timer rotTimer;
  core_time::Timer matSysTimer;
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    inputMgr->Update();

    // rotate the crate
    if (rotTimer.ElapsedMilliSeconds() > 16)
    {
      math_t::Mat3f32 newRot;
      newRot.MakeRotationY(d);
      d += 1.0f;
      crateEnt->GetComponent<math_cs::Transform>()->SetOrientation(newRot);

      rotTimer.Reset();
    }

    if (matSysTimer.ElapsedSeconds() > 1.0f)
    { matSys->ReInitialize(); matSysTimer.Reset(); }

    camSys->ProcessActiveEntities();

    // render to RTT first
    meshSys->SetCamera(m_lightCamera);
    meshSys->SetRenderer(rttRenderer);
    ecsMainScene.Process();

    rttRenderer->ApplyRenderSettings();
    rttRenderer->Render();

    // render the scene normally
    meshSys->SetCamera(m_cameraEnt);
    meshSys->SetRenderer(renderer);
    ecsMainScene.Process();

    renderer->ApplyRenderSettings();
    renderer->Render();

    ecsRttQuad.GetEntityManager()->DeactivateEntity(rttQuad); // to avoid rendering its coordinates
    linesRenderer->ApplyRenderSettings();
    dtrSys->ProcessActiveEntities();
    ecsRttQuad.GetEntityManager()->ActivateEntity(rttQuad);

    // draw the quad on top layer
    ecsRttQuad.Process();
    renderer->Render();

    win.SwapBuffers();
  }

  // -----------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
