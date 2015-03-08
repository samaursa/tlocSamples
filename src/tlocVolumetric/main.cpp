#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <gameAssetsPath.h>

using namespace tloc;

namespace {

  // We need a material to attach to our entity (which we have not yet created).

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathVS("/shaders/tlocOneTextureRotVS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathFS("/shaders/tlocOne3dTextureRotFS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathFS("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
#endif

  const tl_size g_sliceCount = 10;

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
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
    gfx_win::WindowSettings("Texture Fan") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer

  gfx_rend::renderer_sptr renderer = win.GetRenderer();
  {
    auto renderParams = renderer->GetParams();
    renderParams.Enable<gfx_rend::p_renderer::enable_disable::Blend>()
      .AddClearBit<gfx_rend::p_renderer::clear::ColorBufferBit>()
      .Disable<gfx_rend::p_renderer::enable_disable::DepthTest>()
      .SetBlendFunction<gfx_rend::p_renderer::blend_function::SourceAlpha,
                        gfx_rend::p_renderer::blend_function::OneMinusSourceAlpha>();

    renderer->SetParams(renderParams);
  }

  // -----------------------------------------------------------------------
  // prepare the scene

  core_cs::ECS scene;
  scene.AddSystem<gfx_cs::MaterialSystem>();
  scene.AddSystem<gfx_cs::CameraSystem>();
  auto meshSys = scene.AddSystem<gfx_cs::MeshRenderSystem>(); 
  meshSys->SetRenderer(renderer);
  meshSys->SetEnabledSortingByMaterial(false);
  meshSys->SetEnabledSortingBackToFront(true);

  // -----------------------------------------------------------------------
  // Load the required resources

  gfx_med::ImageLoaderPng png;
  {
    core_io::Path p(core_str::String(GetAssetsPath()) + "/images/star.png");
    if (png.Load(p) != ErrorSuccess)
    {
      TLOC_LOG_DEFAULT_ERR() << "Image " << p << " failed to load.";
    }
  }

  const auto pngDim = png.GetImage()->GetDimensions();

  gfx_med::Image3D  img3d;
  img3d.Create(core_ds::MakeTuple(pngDim[0], pngDim[1], g_sliceCount), gfx_t::Color::COLOR_WHITE);
  {
    for (tl_int i = 0; i < g_sliceCount; ++i)
    { img3d.SetImage(0, 0, i, *png.GetImage()); }
  }

  gfx_gl::texture_object_3d_vso to;
  {
    auto texParams = to->GetParams();
    texParams.Wrap_S<gfx_gl::p_texture_object::wrap_technique::ClampToBorder>();
    texParams.Wrap_T<gfx_gl::p_texture_object::wrap_technique::ClampToBorder>();
    texParams.Wrap_R<gfx_gl::p_texture_object::wrap_technique::ClampToBorder>();
    to->SetParams(texParams);
  }
  to->Initialize(img3d);
  to->ReserveTextureUnit();

  //------------------------------------------------------------------------
  // uniforms

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  math_t::mat3_f32_vso rotMat;
  rotMat->MakeIdentity();

  gfx_gl::uniform_vso  u_rot;
  u_rot->SetName("u_rot").SetValueAs(rotMat.get());

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  auto_cref matPtr = scene.CreatePrefab<pref_gfx::Material>()
    .AddUniform(u_to.get())
    .AddUniform(u_rot.get())
    .Create(core_io::Path(GetAssetsPath() + shaderPathVS), 
            core_io::Path(GetAssetsPath() + shaderPathFS))->
            GetComponent<gfx_cs::Material>();

  math_t::Rectf32_c rect(math_t::Rectf32_c::width(15.0f),
                         math_t::Rectf32_c::height(15.0f));

  for (tl_size i = 0; i < g_sliceCount; ++i)
  {
    core_cs::entity_vptr ent =
      scene.CreatePrefab<pref_gfx::Quad>().Dimensions(rect).Create();

    ent->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f(0, 0, i * 0.01f));

    auto_cref meshPtr = ent->GetComponent<gfx_cs::Mesh>();

    const f32 sliceVal = (f32)i/(f32)g_sliceCount * 0.1f;

    gfx_gl::uniform_vso u_slice;
    u_slice->SetName("u_slice").SetValueAs(sliceVal + 0.5f);

    meshPtr->GetUserShaderOperator()->AddUniform(*u_slice);

    scene.GetEntityManager()->
      InsertComponent(core_cs::EntityManager::Params(ent, matPtr));
  }

  // -----------------------------------------------------------------------
  // camera

  auto camEnt = scene.CreatePrefab<pref_gfx::Camera>()
    .Near(0.1f)
    .Far(100.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(0.0f, 0.0f, 30.0f))
    .Create(win.GetDimensions());
  meshSys->SetCamera(camEnt);

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  scene.Initialize();

  //------------------------------------------------------------------------
  // Main loop

  float currAngle = 0.0f;
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    currAngle = currAngle + 1.0f;
    rotMat->MakeRotationY(math_t::Degree(currAngle));
    //rotMat->MakeIdentity

    scene.Process();

    renderer->ApplyRenderSettings();
    renderer->Render();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
