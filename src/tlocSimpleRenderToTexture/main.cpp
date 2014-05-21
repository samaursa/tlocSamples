#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <tlocCore/memory/tlocLinkMe.cpp>

#include <samplesAssetsPath.h>

using namespace tloc;

const u32 g_rttResX = 128;
const u32 g_rttResY = 128;

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
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
    gfx_win::WindowSettings("Render to texture (RTT)") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = win.GetRenderer();
  {
    gfx_rend::Renderer::Params p(renderer->GetParams());
    p.AddClearBit<clear::ColorBufferBit>()
     .AddClearBit<clear::DepthBufferBit>()
     .SetClearColor(gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f))
     .SetDimensions(core_ds::MakeTuple(500, 500));
    renderer->SetParams(p);
  }

  // -----------------------------------------------------------------------
  // Load the required resources. Note that these VSOs will check vptr count
  // before destruction which is why we are placing them here. We want the
  // component pool manager to be destroyed before these are destroyed.

  gfx_med::ImageLoaderPng png;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                      "/images/henry.png").c_str() );

  if (png.Load(path) != ErrorSuccess)
  { TLOC_ASSERT_FALSE("Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  gfx_gl::texture_object_vso to;
  to->Initialize(png.GetImage());
  to->Activate();


  // -----------------------------------------------------------------------
  // to render to texture to a quad we need a renderer with the correct
  // Framebuffer. This particular Framebuffer will be rendering to a texture

  // For rendering to a texture we need a texture object and a render buffer
  // to render the depth
  gfx_gl::texture_object_vso rttTo;
  gfx_med::Image rttImg;
  rttImg.Create
    (core_ds::MakeTuple(g_rttResX, g_rttResY),
     gfx_med::Image::color_type::COLOR_WHITE);

  rttTo->Initialize(rttImg);
  rttTo->Activate();

  using namespace gfx_gl::p_framebuffer_object;
  gfx_gl::framebuffer_object_sptr fbo =
    core_sptr::MakeShared<gfx_gl::FramebufferObject>();
  
  fbo->Attach<target::DrawFramebuffer,
              attachment::ColorAttachment<0> >(*rttTo);

  using namespace gfx_rend::p_renderer;
  gfx_rend::Renderer::Params p;
  p.SetFBO(fbo);
  p.AddClearBit<clear::ColorBufferBit>()
   .AddClearBit<clear::DepthBufferBit>()
   .SetClearColor(gfx_t::Color(0.0f, 0.0f, 0.0f, 1.0f))
   .SetDimensions(core_ds::MakeTuple(g_rttResX, g_rttResY));
  gfx_rend::renderer_sptr rttRenderer =
    core_sptr::MakeShared<gfx_rend::Renderer>(p);

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  // See explanation in SimpleQuad sample on why it must be created first.
  core_cs::component_pool_mgr_vso cpoolMgr;

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso  eventMgr;
  core_cs::entity_manager_vso entityMgr( MakeArgs(eventMgr.get()) );

  //------------------------------------------------------------------------
  // To render the texture we need a quad
  gfx_cs::QuadRenderSystem  quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);

  //------------------------------------------------------------------------
  // To render a fan, we need a fan render system - this is a specialized
  // system to render this primitive
  gfx_cs::FanRenderSystem   fanSys(eventMgr.get(), entityMgr.get());
  fanSys.SetRenderer(rttRenderer);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The fan render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.
#if defined (TLOC_OS_WIN)
    core_str::String shaderPathVS("/shaders/tlocOneTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathFS("/shaders/tlocOneTextureFS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathFS("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathBlurFS("/shaders/tlocOneTextureBlurFS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathBlurFS("/shaders/tlocOneTextureBlurFS_gl_es_2_0.glsl");
#endif

  //------------------------------------------------------------------------
  // Add a texture to the material. We need:
  //  * an image
  //  * a TextureObject (preparing the image for OpenGL)
  //  * a Uniform (all textures are uniforms in shaders)
  //  * a ShaderOperator (this is what the material will take)
  //
  // The material takes in a 'MasterShaderOperator' which is the user defined
  // shader operator and over-rides any shader operators that the systems
  // may be setting. Any uniforms/shaders set in the 'MasterShaderOperator'
  // that have the same name as the one in the system will be given preference.

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  gfx_gl::shader_operator_vso so;
  so->AddUniform(*u_to);

  // -----------------------------------------------------------------------
  // RTT material

  gfx_gl::uniform_vso u_rttTo;
  u_rttTo->SetName("s_texture").SetValueAs(rttTo.get());

  gfx_gl::uniform_vso  u_blur;
  u_blur->SetName("u_blur").SetValueAs(5);

  gfx_gl::uniform_vso u_winResX;
  u_winResX->SetName("u_winResX").SetValueAs(core_utils::CastNumber<s32>(g_rttResX));

  gfx_gl::uniform_vso u_winResY;
  u_winResY->SetName("u_winResY").SetValueAs(core_utils::CastNumber<s32>(g_rttResY));

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  math_t::Circlef32 circ(math_t::Circlef32::radius(1.0f));
  core_cs::entity_vptr q = pref_gfx::Fan(entityMgr.get(), cpoolMgr.get())
    .Sides(64).Circle(circ).Create();

  pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
    .AddUniform(u_to.get())
    .AssetsPath(GetAssetsPath())
    .Add(q, core_io::Path(shaderPathVS), core_io::Path(shaderPathFS));

  math_t::Rectf32_c rect(math_t::Rectf32_c::width(1.5f), 
                         math_t::Rectf32_c::height(1.5f));
  core_cs::entity_vptr fullScreenQuad =
    pref_gfx::Quad(entityMgr.get(), cpoolMgr.get())
    .Dimensions(rect).Create();

  pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
    .AssetsPath(GetAssetsPath())
    .AddUniform(u_rttTo.get()).AddUniform(u_blur.get())
    .AddUniform(u_winResX.get()).AddUniform(u_winResY.get())
    .Add(fullScreenQuad,
         core_io::Path(shaderPathVS),
         core_io::Path(shaderPathBlurFS));

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  fanSys.Initialize();
  quadSys.Initialize();
  matSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    // render the fan, which will be rendered to a texture
    fanSys.GetRenderer()->ApplyRenderSettings();
    fanSys.ProcessActiveEntities();

    // render the quad, which will be rendered to the front buffer with the
    // default renderer
    quadSys.GetRenderer()->ApplyRenderSettings();
    quadSys.ProcessActiveEntities();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
