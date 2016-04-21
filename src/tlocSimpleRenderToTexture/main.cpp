#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <gameAssetsPath.h>

using namespace tloc;

namespace {

  const u32 g_rttResX = 512;
  const u32 g_rttResY = 512;

  // We need a material to attach to our entity (which we have not yet created).
#if defined (TLOC_OS_WIN)
  core_str::String shaderPathVS("/shaders/tlocOneTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathFS("/shaders/tlocOneTextureMultipleOutsFS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathFS("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathBlurFS("/shaders/tlocOneTextureBlurFS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathBlurFS("/shaders/tlocOneTextureBlurFS_gl_es_2_0.glsl");
#endif

}

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
    renderer->SetParams(GetParamsCommon(renderer->GetParams().GetFBO(), 
                                        renderer->GetParams().GetDimensions(),
                                        gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f)) );
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
  to->Initialize(*png.GetImage());

  // -----------------------------------------------------------------------

  gfx::Rtt rtt(core_ds::MakeTuple(g_rttResX, g_rttResY));
  auto rttTo        = rtt.AddColorAttachment<0, gfx_t::color_u16_rgba>();
  auto rttRenderer  = rtt.GetRenderer();
  auto rttTo2       = rtt.AddColorAttachment(1);

  //------------------------------------------------------------------------

  core_cs::ECS rttScene, mainScene;

  //------------------------------------------------------------------------
  // To render the texture we need a quad
  mainScene.AddSystem<gfx_cs::MaterialSystem>();
  auto  quadSys = mainScene.AddSystem<gfx_cs::MeshRenderSystem>();
  quadSys->SetRenderer(renderer);

  rttScene.AddSystem<gfx_cs::MaterialSystem>();
  auto  fanSys = rttScene.AddSystem<gfx_cs::MeshRenderSystem>();
  fanSys->SetRenderer(rttRenderer);

  //------------------------------------------------------------------------
  // Add a texture to the material. We need:
  //  * an image
  //  * a TextureObject (preparing the image for OpenGL)
  //  * a Uniform (all textures are uniforms in shaders)
  //  * a ShaderOperator (this is what the material will take)

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  gfx_gl::shader_operator_vso so;
  so->AddUniform(*u_to);

  // -----------------------------------------------------------------------
  // RTT material

  gfx_gl::uniform_vso u_rttTo;
  u_rttTo->SetName("s_texture").SetValueAs(core_sptr::ToVirtualPtr(rttTo));

  gfx_gl::uniform_vso u_rttTo2;
  u_rttTo2->SetName("s_texture").SetValueAs(core_sptr::ToVirtualPtr(rttTo2));

  gfx_gl::uniform_vso  u_blur;
  u_blur->SetName("u_blur").SetValueAs(5);

  gfx_gl::uniform_vso u_winResX;
  u_winResX->SetName("u_winResX").SetValueAs(core_utils::CastNumber<s32>(g_rttResX));

  gfx_gl::uniform_vso u_winResY;
  u_winResY->SetName("u_winResY").SetValueAs(core_utils::CastNumber<s32>(g_rttResY));

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  // the circle that will be rendered to textures
  math_t::Circlef32 circ(math_t::Circlef32::radius(1.0f));

  auto q = rttScene.CreatePrefab<pref_gfx::Fan>()
    .Sides(64).Circle(circ).Create();

  rttScene.CreatePrefab<pref_gfx::Material>()
    .AddUniform(u_to.get())
    .AssetsPath(GetAssetsPath())
    .Add(q, core_io::Path(shaderPathVS), core_io::Path(shaderPathFS));

#if defined (TLOC_OS_WIN)
  math_t::Rectf32_c rect(math_t::Rectf32_c::width(0.6f),
                         math_t::Rectf32_c::height(0.6f));

  // the first quad with the circle diffuse render
  auto diffuseQuad =
    mainScene.CreatePrefab<pref_gfx::Quad>()
    .Dimensions(rect).DispatchTo(quadSys.get()).Create();

  diffuseQuad->GetComponent<math_cs::Transform>()
    ->SetPosition(math_t::Vec3f32(-0.5f, 0, 0));

  mainScene.CreatePrefab<pref_gfx::Material>()
    .AssetsPath(GetAssetsPath())
    .AddUniform(u_rttTo.get()).AddUniform(u_blur.get())
    .AddUniform(u_winResX.get()).AddUniform(u_winResY.get())
    .Add(diffuseQuad,
         core_io::Path(shaderPathVS),
         core_io::Path(shaderPathBlurFS));

  // the second quad rendering the circle using texcoords
  auto texQuad =
    mainScene.CreatePrefab<pref_gfx::Quad>()
    .Dimensions(rect).DispatchTo(quadSys.get()).Create();

  texQuad->GetComponent<math_cs::Transform>()
    ->SetPosition(math_t::Vec3f32(0.5f, 0, 0));

  mainScene.CreatePrefab<pref_gfx::Material>()
    .AssetsPath(GetAssetsPath())
    .AddUniform(u_rttTo2.get()).AddUniform(u_blur.get())
    .AddUniform(u_winResX.get()).AddUniform(u_winResY.get())
    .Add(texQuad,
         core_io::Path(shaderPathVS),
         core_io::Path(shaderPathBlurFS));

#elif defined (TLOC_OS_IPHONE)
  math_t::Rectf32_c rect(math_t::Rectf32_c::width(1.5f),
                         math_t::Rectf32_c::height(1.5f));

  // the first quad with the circle diffuse render
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
#endif

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  rttScene.Initialize();
  mainScene.Initialize();

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    // render the fan, which will be rendered to a texture
    rttScene.Process(1.0/60.0);
    fanSys->GetRenderer()->ApplyRenderSettings();
    fanSys->GetRenderer()->Render();

    // render the quad, which will be rendered to the front buffer with the
    // default renderer
    mainScene.Process(1.0/60.0);

    quadSys->GetRenderer()->ApplyRenderSettings();
    quadSys->GetRenderer()->Render();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
