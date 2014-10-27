#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocInput/tloc_input.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <samplesAssetsPath.h>

using namespace tloc;

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
  win.Create( gfx_win::Window::graphics_mode::Properties(1024, 768),
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
   .Disable<enable_disable::CullFace>()
   .AddClearBit<clear::ColorBufferBit>()
   .AddClearBit<clear::DepthBufferBit>();

  gfx_rend::Renderer::Params pNoDepth(renderer->GetParams());
  pNoDepth.SetClearColor(gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f))
          .Disable<enable_disable::DepthTest>() 
          .AddClearBit<clear::DepthBufferBit>();

  gfx_rend::renderer_sptr linesRenderer = 
    core_sptr::MakeShared<gfx_rend::Renderer>(pNoDepth);

  renderer->SetParams(p);

  // -----------------------------------------------------------------------
  // Prepare RTT

  gfx_gl::TextureObject::Params rttToParams;
  rttToParams.InternalFormat<gfx_gl::p_texture_object::internal_format::DepthComponent>();
  rttToParams.Format<gfx_gl::p_texture_object::format::DepthComponent>();

  gfx_gl::texture_object_vso rttTo;
  rttTo->SetParams(rttToParams);
  gfx_med::image_u16_r rttImg;

  rttImg.Create(core_ds::MakeTuple(1024, 1024),
                gfx_med::image_u16_r::color_type());
  rttTo->Initialize(rttImg);

  using namespace gfx_gl::p_framebuffer_object;

  gfx_gl::framebuffer_object_sptr fbo = 
    core_sptr::MakeShared<gfx_gl::FramebufferObject>();

  fbo->Attach<target::DrawFramebuffer, attachment::Depth>(*rttTo);

  using namespace gfx_rend::p_renderer;
  gfx_rend::Renderer::Params pRtt;
  pRtt.SetFBO(fbo);
  pRtt.AddClearBit<clear::ColorBufferBit>()
      .AddClearBit<clear::DepthBufferBit>()
      .Enable<enable_disable::DepthTest>()
      .Enable<enable_disable::CullFace>()
      .Cull<cull_face::Front>()
      .SetClearColor(gfx_t::Color::COLOR_WHITE)
      .SetDimensions(core_ds::MakeTuple(rttImg.GetWidth(), rttImg.GetHeight()));

  gfx_rend::renderer_sptr rttRenderer = 
    core_sptr::MakeShared<gfx_rend::Renderer>(pRtt);

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

  gfx_cs::QuadRenderSystem quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);

  // -----------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  // -----------------------------------------------------------------------
  // The camera's view transformations are calculated by the camera system
  gfx_cs::CameraSystem      camSys(eventMgr.get(), entityMgr.get());
  gfx_cs::ArcBallSystem     arcBallSys(eventMgr.get(), entityMgr.get());
  input_cs::ArcBallControlSystem arcBallControlSystem(eventMgr.get(), entityMgr.get());

  // -----------------------------------------------------------------------
  // Transformation debug rendering

  gfx_cs::DebugTransformRenderSystem dtrSys(eventMgr.get(), entityMgr.get());
  dtrSys.SetScale(1.0f);
  dtrSys.SetRenderer(linesRenderer);

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
    pref_gfx::Quad(entityMgr.get(), cpoolMgr.get())
    .Dimensions(rectDim)
    .Create();

  rttQuad->GetComponent<math_cs::Transform>()->
    SetPosition(math_t::Vec3f32(1.0f - rectDim.GetWidth() * 0.5f, 
                                1.0f - rectDim.GetHeight() * 0.5f, 
                                0.0f));

  gfx_gl::uniform_vso u_toRtt;
  u_toRtt->SetName("s_texture").SetValueAs(*rttTo);

  pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
    .AddUniform(u_toRtt.get())
    .Add(rttQuad, core_io::Path(GetAssetsPath() + shaderPathQuadVS),
                  core_io::Path(GetAssetsPath() + shaderPathQuadFS));

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library - this will be our scene camera

  core_cs::entity_vptr m_cameraEnt =
    pref_gfx::Camera(entityMgr.get(), cpoolMgr.get())
    .Perspective(true)
    .Near(1.0f)
    .Far(100.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(0.0f, 0.0f, 5.0f))
    .Create(win.GetDimensions());

  // -----------------------------------------------------------------------
  // Create another camera, this will be what the light sees

  core_cs::entity_vptr m_lightCamera =
    pref_gfx::Camera(entityMgr.get(), cpoolMgr.get())
    .Perspective(true)
    .Near(1.0f)
    .Far(100.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(4.0f, 4.0f, 4.0f))
    .Create(win.GetDimensions());

  m_lightCamera->GetComponent<gfx_cs::Camera>()->
    LookAt(math_t::Vec3f32::ZERO);

  pref_gfx::ArcBall(entityMgr.get(), cpoolMgr.get()).Add(m_cameraEnt);
  pref_input::ArcBallControl(entityMgr.get(), cpoolMgr.get())
    .GlobalMultiplier(math_t::Vec2f(0.01f, 0.01f))
    .Add(m_cameraEnt);

  dtrSys.SetCamera(m_cameraEnt);
  meshSys.SetCamera(m_cameraEnt);

  keyboard->Register(&arcBallControlSystem);
  mouse->Register(&arcBallControlSystem);
  touchSurface->Register(&arcBallControlSystem);

  // initialize the camera system early and process it once to update all
  // transformations
  camSys.Initialize();
  camSys.ProcessActiveEntities();

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
    crateTextureTo->Initialize(png.GetImage());
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
    pref_gfx::Mesh(entityMgr.get(), cpoolMgr.get()).Create(vertices);

  using math_t::Cuboidf32;
  Cuboidf32 cubSize(Cuboidf32::width(10.0f),
                    Cuboidf32::height(0.1f), 
                    Cuboidf32::depth(10.0f));

  core_cs::entity_vptr floorMesh = 
    pref_gfx::Cuboid(entityMgr.get(), cpoolMgr.get())
    .Dimensions(cubSize)
    .Create();

  floorMesh->GetComponent<math_cs::Transform>()->
    SetPosition(math_t::Vec3f32(0, -2.0f, 0));

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(*crateTextureTo);

  gfx_gl::uniform_vso  u_toShadowMap;
  u_toShadowMap->SetName("s_shadowMap").SetValueAs(*rttTo);

  gfx_gl::uniform_vso  u_lightDir;
  u_lightDir->SetName("u_lightDir").SetValueAs(lightDir);

  gfx_gl::uniform_vso  u_lightMVP;
  u_lightMVP->SetName("u_lightMVP").SetValueAs(lightMVP);

  pref_gfx::Material meshMat(entityMgr.get(), cpoolMgr.get());
  meshMat.AddUniform(u_to.get())
         .AddUniform(u_lightDir.get())
         .AddUniform(u_toShadowMap.get()) 
         .AddUniform(u_lightMVP.get());

  meshMat.Add(crateEnt, core_io::Path(GetAssetsPath() + shaderPathMeshVS), 
                   core_io::Path(GetAssetsPath() + shaderPathMeshFS));

  // change the texture to grayscale - over-riding does not change the previously
  // applied material because the uniforms are copied
  u_to->SetValueAs(*to);

  meshMat.Add(floorMesh, core_io::Path(GetAssetsPath() + shaderPathMeshVS), 
                         core_io::Path(GetAssetsPath() + shaderPathMeshFS));

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  dtrSys.Initialize();
  meshSys.Initialize();
  matSys.Initialize();
  quadSys.Initialize();
  arcBallSys.Initialize();
  arcBallControlSystem.Initialize();

  // -----------------------------------------------------------------------
  // Main loop

  TLOC_LOG_CORE_DEBUG() << 
    "Press ALT and Left, Middle and Right mouse buttons to manipulate the camera";

  math_t::Degree d(0.0f);

  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    inputMgr->Update();

    arcBallControlSystem.ProcessActiveEntities();
    arcBallSys.ProcessActiveEntities();
    camSys.ProcessActiveEntities();

    // rotate the crate
    math_t::Mat3f32 newRot;
    newRot.MakeRotationY(d);
    d += 1.0f;
    crateEnt->GetComponent<math_cs::Transform>()->SetOrientation(newRot);

    // render to RTT first
    rttRenderer->ApplyRenderSettings();
    meshSys.SetEnabledUniformModelMatrix(false);
    meshSys.SetCamera(m_lightCamera);
    meshSys.SetRenderer(rttRenderer);
    meshSys.ProcessActiveEntities();

    // render the scene normally
    renderer->ApplyRenderSettings();
    meshSys.SetEnabledUniformModelMatrix(true);
    meshSys.SetCamera(m_cameraEnt);
    meshSys.SetRenderer(renderer);
    meshSys.ProcessActiveEntities();

    entityMgr->DeactivateEntity(rttQuad); // to avoid rendering its coordinates
    linesRenderer->ApplyRenderSettings();
    dtrSys.ProcessActiveEntities();
    entityMgr->ActivateEntity(rttQuad);

    // draw the quad on top layer
    quadSys.ProcessActiveEntities();

    win.SwapBuffers();
  }

  // -----------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
