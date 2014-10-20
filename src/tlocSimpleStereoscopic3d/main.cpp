#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocInput/tloc_input.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <samplesAssetsPath.h>

using namespace tloc;

bool          g_renderDepthToRightViewport = false;
bool          g_fullScreen = false;
f32           g_convergence = 1.0f;
f32           g_interaxial = 0.05f;
gfx_t::Color  g_clearColor(0.1f, 0.1f, 0.1f, 0.1f);


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

// ///////////////////////////////////////////////////////////////////////
// Main

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  gfx_win::WindowSettings ws("Basic Stereoscopic 3D");
  if (g_fullScreen)
  {
    ws.ClearStyles().AddStyle<gfx_win::p_window_settings::style::FullScreen>();
  }

  win.Create( gfx_win::Window::graphics_mode::Properties(1280, 720), ws);

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  gfx_rend::Renderer::Params p(renderer->GetParams());
  p.SetClearColor(gfx_t::Color(1.0f, 1.0f, 1.0f, 1.0f))
   .Enable<enable_disable::DepthTest>()
   .AddClearBit<clear::ColorBufferBit>()
   .AddClearBit<clear::DepthBufferBit>();

  renderer->SetParams(p);

  // -----------------------------------------------------------------------
  // Create the two renderers

  gfx_rend::renderer_sptr     rttRenderLeft, rttRenderRight;
  gfx_gl::texture_object_sptr toLeft, toRight;

  gfx_med::Image::dimension_type halfWinDim
    (core_ds::Variadic<gfx_med::Image::dimension_type::value_type, 2>
    (win.GetWidth() / 2, win.GetHeight() / 2));

  {
    using namespace gfx_gl;
    using namespace gfx_med;

    toLeft = core_sptr::MakeShared<TextureObject>();
    Image rttImg;
    rttImg.Create(halfWinDim, Image::color_type::COLOR_WHITE);
    toLeft->Initialize(rttImg);

    RenderbufferObject::Params rboParam;
    rboParam.InternalFormat<p_renderbuffer_object::internal_format::DepthComponent24>();
    rboParam.Dimensions(halfWinDim.ConvertTo<RenderbufferObject::Params::dimension_type>());

    gfx_gl::render_buffer_object_sptr rbo;
    rbo = core_sptr::MakeShared<RenderbufferObject>(rboParam);
    rbo->Initialize();

    using namespace gfx_gl::p_fbo;
    framebuffer_object_sptr fbo = core_sptr::MakeShared<FramebufferObject>();
    fbo->Attach<target::Framebuffer,
                attachment::ColorAttachment<0> >(*toLeft);
    fbo->Attach<target::Framebuffer,
                attachment::Depth>(*rbo);

    using namespace gfx_rend::p_renderer;
    using namespace gfx_rend;
    Renderer::Params p;
    p.SetFBO(fbo);
    p.AddClearBit<clear::ColorBufferBit>()
     .AddClearBit<clear::DepthBufferBit>()
     .SetClearColor(g_clearColor)
     .SetDimensions(halfWinDim.ConvertTo<Renderer::dimension_type>());

    rttRenderLeft = core_sptr::MakeShared<Renderer>(p);
  }

  {
    using namespace gfx_gl;
    using namespace gfx_med;

    TextureObject::Params texParams;
    if (g_renderDepthToRightViewport)
    {
      texParams.InternalFormat<p_texture_object::internal_format::DepthComponent>();
      texParams.Format<p_texture_object::format::DepthComponent>();
    }
    toRight = core_sptr::MakeShared<TextureObject>(texParams);

    if (g_renderDepthToRightViewport)
    {
      image_f32_r rttImg;
      rttImg.Create(halfWinDim, image_f32_r::color_type(1.0f));
      toRight->Initialize(rttImg);
    }
    else
    {
      Image rttImg;
      rttImg.Create(halfWinDim, Image::color_type::COLOR_WHITE);
      toRight->Initialize(rttImg);
    }


    RenderbufferObject::Params rboParam;
    if (g_renderDepthToRightViewport == false)
    {
      rboParam.InternalFormat<p_renderbuffer_object::internal_format::DepthComponent16>();
    }
    rboParam.Dimensions(halfWinDim.ConvertTo<RenderbufferObject::Params::dimension_type>());

    gfx_gl::render_buffer_object_sptr rbo;
    rbo = core_sptr::MakeShared<RenderbufferObject>(rboParam);
    rbo->Initialize();

    using namespace gfx_gl::p_fbo;
    framebuffer_object_sptr fbo = core_sptr::MakeShared<FramebufferObject>();
    if (g_renderDepthToRightViewport)
    { fbo->Attach<target::DrawFramebuffer, attachment::Depth>(*toRight); }
    else
    {
      fbo->Attach<target::DrawFramebuffer,
                  attachment::ColorAttachment<0> >(*toRight);
      fbo->Attach<target::DrawFramebuffer,
                  attachment::Depth>(*rbo);
    }

    using namespace gfx_rend::p_renderer;
    using namespace gfx_rend;
    Renderer::Params p;
    p.SetFBO(fbo);
    p.AddClearBit<clear::ColorBufferBit>()
     .AddClearBit<clear::DepthBufferBit>()
     .SetClearColor(g_clearColor)
     .SetDimensions(halfWinDim.ConvertTo<Renderer::dimension_type>());

    rttRenderRight = core_sptr::MakeShared<Renderer>(p);
  }

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> kbParams;
  kbParams.m_param1 = win.GetWindowHandle();

  input::input_mgr_b_ptr inputMgr =
    core_sptr::MakeShared<input::InputManagerB>(kbParams);

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  // See explanation in SimpleQuad sample on why it must be created first.
  core_cs::component_pool_mgr_vso cpoolMgr;

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::keyboard_b_vptr keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();
  input_hid::mouse_b_vptr    mouse = inputMgr->CreateHID<input_hid::MouseB>();

  // Check pointers
  TLOC_ASSERT_NOT_NULL(keyboard);
  TLOC_ASSERT_NOT_NULL(mouse);

  // -----------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso  eventMgr;
  core_cs::entity_manager_vso entityMgr( MakeArgs(eventMgr.get()) );

  // -----------------------------------------------------------------------
  // To render a mesh, we need a mesh render system - this is a specialized
  // system to render this primitive
  gfx_cs::MeshRenderSystem  meshSys(eventMgr.get(), entityMgr.get());
  meshSys.SetRenderer(rttRenderLeft);

  // -----------------------------------------------------------------------
  // quad render system for two quads for two views
  gfx_cs::QuadRenderSystem  quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);

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

#if defined (TLOC_OS_WIN)
    core_str::String meshShaderPathVS("/shaders/tlocTexturedMeshVS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String meshShaderPathVS("/shaders/tlocTexturedMeshVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
    core_str::String meshShaderPathFS("/shaders/tlocTexturedMeshFS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String meshShaderPathFS("/shaders/tlocTexturedMeshFS_gl_es_2_0.glsl");
#endif

  // -----------------------------------------------------------------------
  // Add a texture to the material. We need:
  //  * an image
  //  * a TextureObject (preparing the image for OpenGL)
  //  * a Uniform (all textures are uniforms in shaders)
  //  * a ShaderOperator (this is what the material will take)

  gfx_med::ImageLoaderPng png;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                       "/images/crateTexture.png").c_str() );

  if (png.Load(path) != ErrorSuccess)
  { TLOC_ASSERT_FALSE("Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  gfx_gl::texture_object_vso crateTo;
  crateTo->Initialize(png.GetImage());

  gfx_gl::uniform_vso  u_crateTo;
  u_crateTo->SetName("s_texture").SetValueAs(*crateTo);

  // -----------------------------------------------------------------------
  // More shaderpaths

#if defined (TLOC_OS_WIN)
  core_str::String quadShaderVS("/shaders/tlocOneTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String quadShaderVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String quadShaderFS("/shaders/tlocOneTextureFS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String quadShaderFS("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
#endif

  pref_gfx::Mesh m(entityMgr.get(), cpoolMgr.get());

  // -----------------------------------------------------------------------
  // ObjLoader can load (basic) .obj files

  path = core_io::Path( (core_str::String(GetAssetsPath()) +
                         "/models/crate.obj").c_str() );

  core_io::FileIO_ReadA objFile(path);
  if (objFile.Open() != ErrorSuccess)
  { 
    TLOC_LOG_CORE_ERR() << "Unable to open the .obj file.";
    return 1;
  }

  core_str::String objFileContents;
  objFile.GetContents(objFileContents);

  gfx_med::ObjLoader objLoader;
  if (objLoader.Init(objFileContents) != ErrorSuccess)
  { 
    TLOC_LOG_CORE_ERR() << "Parsing errors in .obj file.";
    return 1;
  }

  if (objLoader.GetNumGroups() == 0)
  { 
    TLOC_LOG_CORE_ERR() << "Obj file does not have any objects.";
    return 1;
  }

  gfx_med::ObjLoader::vert_cont_type vertices;
  objLoader.GetUnpacked(vertices, 0);

  // -----------------------------------------------------------------------
  // Create the mesh and add the material

  gfx_gl::uniform_vso  u_lightDir;
  u_lightDir->SetName("u_lightDir").SetValueAs(math_t::Vec3f32(0.2f, 0.5f, 3.0f));

  core_cs::entity_vptr crateMesh =
    pref_gfx::Mesh(entityMgr.get(), cpoolMgr.get()).Create(vertices);
  pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
    .AddUniform(u_crateTo.get())
    .AddUniform(u_lightDir.get())
    .Add(crateMesh, core_io::Path(GetAssetsPath() + meshShaderPathVS),
                    core_io::Path(GetAssetsPath() + meshShaderPathFS));

  // -----------------------------------------------------------------------
  // Create the two quads

  using math_t::Rectf32_c;
  Rectf32_c leftQuad(Rectf32_c::width(1.0f), Rectf32_c::height(2.0f));

  core_cs::entity_vptr leftQuadEnt =
    pref_gfx::Quad(entityMgr.get(), cpoolMgr.get())
    .Dimensions(leftQuad).Create();

  leftQuadEnt->GetComponent<math_cs::Transform>()->
    SetPosition(math_t::Vec3f32(-0.5, 0, 0));

  {
    gfx_gl::uniform_vso  u_to;
    u_to->SetName("s_texture").SetValueAs(*toLeft);

    // create the material
    pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
      .AddUniform(u_to.get())
      .Add(leftQuadEnt, core_io::Path(GetAssetsPath() + quadShaderVS),
                        core_io::Path(GetAssetsPath() + quadShaderFS));
  }

  core_cs::entity_vptr rightQuadEnt =
    pref_gfx::Quad(entityMgr.get(), cpoolMgr.get())
    .Dimensions(leftQuad).Create();

  rightQuadEnt->GetComponent<math_cs::Transform>()->
    SetPosition(math_t::Vec3f32(0.5, 0, 0));

  {
    gfx_gl::uniform_vso  u_to;
    u_to->SetName("s_texture").SetValueAs(*toRight);

    // create the material
    pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
      .AddUniform(u_to.get())
      .Add(rightQuadEnt, core_io::Path(GetAssetsPath() + quadShaderVS),
                         core_io::Path(GetAssetsPath() + quadShaderFS));
  }

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library

  math_t::AspectRatio ar(math_t::AspectRatio::width( (tl_float)win.GetWidth()),
    math_t::AspectRatio::height( (tl_float)win.GetHeight()) );
  math_t::FOV fov(math_t::Degree(60.0f), ar, math_t::p_FOV::vertical());

  math_proj::FrustumPersp::Params params(fov);
  params.SetFar(100.0f).SetNear(1.0f).SetConvergence(g_convergence).SetInteraxial(g_interaxial);

  math_proj::FrustumPersp frLeft(params);
  frLeft.BuildFrustum();

  core_cs::entity_vptr m_cameraEntLeft =
    pref_gfx::Camera(entityMgr.get(), cpoolMgr.get())
    .Position(math_t::Vec3f(0.0f, 0.0f, 5.0f))
    .Create(frLeft);

  pref_gfx::ArcBall(entityMgr.get(), cpoolMgr.get()).Add(m_cameraEntLeft);
  pref_input::ArcBallControl(entityMgr.get(), cpoolMgr.get())
    .GlobalMultiplier(math_t::Vec2f(0.01f, -0.01f))
    .Add(m_cameraEntLeft);

  meshSys.SetCamera(m_cameraEntLeft);

  params.SetInteraxial(g_interaxial * -1.0f);
  math_proj::FrustumPersp frRight(params);
  frRight.BuildFrustum();

  core_cs::entity_vptr m_cameraEntRight =
    pref_gfx::Camera(entityMgr.get(), cpoolMgr.get())
    .Position(math_t::Vec3f(0.0f, 0.0f, 5.0f))
    .Create(frRight);

  pref_gfx::ArcBall(entityMgr.get(), cpoolMgr.get()).Add(m_cameraEntRight);
  pref_input::ArcBallControl(entityMgr.get(), cpoolMgr.get())
    .GlobalMultiplier(math_t::Vec2f(0.01f, -0.01f))
    .Add(m_cameraEntRight);

  meshSys.SetCamera(m_cameraEntRight);

  // -----------------------------------------------------------------------

  keyboard->Register(&arcBallControlSys);
  mouse->Register(&arcBallControlSys);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  meshSys.Initialize();
  quadSys.Initialize();
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

    rttRenderLeft->ApplyRenderSettings();
    meshSys.SetCamera(m_cameraEntLeft);
    meshSys.SetRenderer(rttRenderLeft);
    meshSys.ProcessActiveEntities();

    rttRenderRight->ApplyRenderSettings();
    meshSys.SetCamera(m_cameraEntRight);
    meshSys.SetRenderer(rttRenderRight);
    meshSys.ProcessActiveEntities();

    renderer->ApplyRenderSettings();
    quadSys.ProcessActiveEntities();

    win.SwapBuffers();
  }

  // -----------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
