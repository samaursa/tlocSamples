#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocInput/tloc_input.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <tlocCore/memory/tlocLinkMe.cpp>

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
  MayaCam(core_cs::entity_vptr a_camera)
    : m_camera(a_camera)
    , m_flags(k_count)
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

      arcBall->MoveVertical(yRel * 0.01f * -1.0f);
      arcBall->MoveHorizontal(xRel * 0.01f);
    }
    else if (m_flags.IsMarked(k_panning))
    {
      math_cs::transform_vptr t = m_camera->GetComponent<math_cs::Transform>();
      gfx_cs::arcball_vptr arcBall = m_camera->GetComponent<gfx_cs::ArcBall>();

      math_t::Vec3f32 leftVec; t->GetOrientation().GetCol(0, leftVec);
      math_t::Vec3f32 upVec; t->GetOrientation().GetCol(1, upVec);

      leftVec *= xRel * 0.01f;
      upVec *= yRel * 0.01f * -1.0f;

      t->SetPosition(t->GetPosition() - leftVec + upVec);
      arcBall->SetFocus(arcBall->GetFocus() - leftVec + upVec);
    }
    else if (m_flags.IsMarked(k_dolly))
    {
      math_cs::transform_vptr t = m_camera->GetComponent<math_cs::Transform>();

      math_t::Vec3f32 dirVec; t->GetOrientation().GetCol(2, dirVec);

      dirVec *= xRel * 0.01f;

      t->SetPosition(t->GetPosition() - dirVec);
    }

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

  core_cs::entity_vptr        m_camera;
  core_utils::Checkpoints m_flags;
};
TLOC_DEF_TYPE(MayaCam);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  gfx_win::WindowSettings ws("Stereoscopic 3D");
  if (g_fullScreen)
  {
    ws.ClearStyles().AddStyle<gfx_win::p_window_settings::style::FullScreen>();
  }

  win.Create( gfx_win::Window::graphics_mode::Properties(1280, 720), ws);

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { printf("\nGraphics platform failed to initialize"); return -1; }

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

    toLeft.reset(new TextureObject() );
    Image rttImg;
    rttImg.Create(halfWinDim, Image::color_type::COLOR_WHITE);
    toLeft->Initialize(rttImg);
    toLeft->Activate();

    RenderbufferObject::Params rboParam;
    rboParam.InternalFormat<p_renderbuffer_object::internal_format::DepthComponent24>();
    rboParam.Dimensions(halfWinDim.ConvertTo<RenderbufferObject::Params::dimension_type>());

    gfx_gl::render_buffer_object_sptr rbo;
    rbo.reset(new RenderbufferObject(rboParam));
    rbo->Initialize();

    using namespace gfx_gl::p_framebuffer_object;
    framebuffer_object_sptr fbo(new FramebufferObject());
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

    rttRenderLeft.reset(new Renderer(p));
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
    toRight.reset(new TextureObject(texParams) );
    Image rttImg;
    rttImg.Create(halfWinDim, Image::color_type::COLOR_WHITE);
    toRight->Initialize(rttImg);
    toRight->Activate();

    RenderbufferObject::Params rboParam;
    if (g_renderDepthToRightViewport == false)
    {
      rboParam.InternalFormat<p_renderbuffer_object::internal_format::DepthComponent16>();
    }
    rboParam.Dimensions(halfWinDim.ConvertTo<RenderbufferObject::Params::dimension_type>());

    gfx_gl::render_buffer_object_sptr rbo;
    rbo.reset(new RenderbufferObject(rboParam));
    rbo->Initialize();

    using namespace gfx_gl::p_framebuffer_object;
    framebuffer_object_sptr fbo(new FramebufferObject());
    if (g_renderDepthToRightViewport)
    {
      fbo->Attach<target::DrawFramebuffer,
                  attachment::ColorAttachment<0> >(*rbo);
      fbo->Attach<target::DrawFramebuffer,
                  attachment::Depth>(*toRight);
    }
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

    rttRenderRight.reset(new Renderer(p));
  }

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> kbParams;
  kbParams.m_param1 = win.GetWindowHandle();

  input::input_mgr_b_ptr inputMgr =
    input::input_mgr_b_ptr(new input::InputManagerB(kbParams));

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  // See explanation in SimpleQuad sample on why it must be created first.
  core_cs::component_pool_mgr_vso cpoolMgr;

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::KeyboardB* keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();
  input_hid::MouseB* mouse = inputMgr->CreateHID<input_hid::MouseB>();

  // Check pointers
  TLOC_ASSERT_NOT_NULL(keyboard);
  TLOC_ASSERT_NOT_NULL(mouse);

  // -----------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso  eventMgr;
  core_cs::entity_manager_vso entityMgr(eventMgr.get());

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

  // -----------------------------------------------------------------------
  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The fan render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.

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
  crateTo->Activate();

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
  { printf("\nUnable to open the .obj file."); return 1;}

  core_str::String objFileContents;
  objFile.GetContents(objFileContents);

  gfx_med::ObjLoader objLoader;
  if (objLoader.Init(objFileContents) != ErrorSuccess)
  { printf("\nParsing errors in .obj file."); return 1; }

  if (objLoader.GetNumGroups() == 0)
  { printf("\nObj file does not have any objects."); return 1; }

  gfx_med::ObjLoader::vert_cont_type vertices;
  objLoader.GetUnpacked(vertices, 0);

  // -----------------------------------------------------------------------
  // Create the mesh and add the material

  core_cs::entity_vptr crateMesh =
    pref_gfx::Mesh(entityMgr.get(), cpoolMgr.get()).Create(vertices);
  pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
    .AddUniform(u_crateTo.get())
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
    pref_gfx::Camera(entityMgr.get(), cpoolMgr.get()).
    Create(frLeft, math_t::Vec3f(0.0f, 0.0f, 5.0f));

  pref_gfx::ArcBall(entityMgr.get(), cpoolMgr.get()).Add(m_cameraEntLeft);

  meshSys.SetCamera(m_cameraEntLeft);

  MayaCam mayaCamLeft(m_cameraEntLeft);
  keyboard->Register(&mayaCamLeft);
  mouse->Register(&mayaCamLeft);


  params.SetInteraxial(g_interaxial * -1.0f);
  math_proj::FrustumPersp frRight(params);
  frRight.BuildFrustum();

  core_cs::entity_vptr m_cameraEntRight =
    pref_gfx::Camera(entityMgr.get(), cpoolMgr.get()).
    Create(frRight, math_t::Vec3f(0.0f, 0.0f, 5.0f));

  pref_gfx::ArcBall(entityMgr.get(), cpoolMgr.get()).Add(m_cameraEntRight);

  meshSys.SetCamera(m_cameraEntRight);

  MayaCam mayaCamRight(m_cameraEntRight);
  keyboard->Register(&mayaCamRight);
  mouse->Register(&mayaCamRight);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  meshSys.Initialize();
  quadSys.Initialize();
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
  printf("\nExiting normally\n");

  return 0;

}
