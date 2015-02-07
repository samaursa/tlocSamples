#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocMath/tloc_math.inl.h>
#include <tlocInput/tloc_input.h>
#include <tlocPrefab/tloc_prefab.h>

#include <gameAssetsPath.h>

#include <tlocCore/smart_ptr/tloc_smart_ptr.inl.h>
#include <tlocCore/containers/tlocArray.inl.h>

using namespace tloc;

namespace {

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The quad render system expects a few shader variables to be declared
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
             gfx_win::WindowSettings("Mesh Sorting") );

  // -----------------------------------------------------------------------
  // input

  tloc::ParamList<core_t::Any> kbParams;
  kbParams.m_param1 = win.GetWindowHandle();

  auto inputMgr = core_sptr::MakeShared<input::InputManagerB>(kbParams);
  auto keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();
  auto mouse    = inputMgr->CreateHID<input_hid::MouseB>();

  TLOC_ASSERT_NOT_NULL(keyboard);
  TLOC_ASSERT_NOT_NULL(mouse);

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
      .AddClearBit<gfx_rend::p_renderer::clear::DepthBufferBit>()
      .Enable<gfx_rend::p_renderer::enable_disable::DepthTest>()
      .SetBlendFunction<gfx_rend::p_renderer::blend_function::SourceAlpha,
                        gfx_rend::p_renderer::blend_function::OneMinusSourceAlpha>();

    renderer->SetParams(renderParams);
  }

  // -----------------------------------------------------------------------
  // prepare the scene

  core_cs::ecs_vso    scene;
  auto arcBallControl = scene->AddSystem<input_cs::ArcBallControlSystem>();
  keyboard->Register(arcBallControl.get());
  mouse->Register(arcBallControl.get());

  scene->AddSystem<gfx_cs::ArcBallSystem>();
  scene->AddSystem<gfx_cs::MaterialSystem>();
  scene->AddSystem<gfx_cs::CameraSystem>();

  auto quadSys = scene->AddSystem<gfx_cs::MeshRenderSystem>();
  quadSys->SetRenderer(renderer);
  quadSys->SetEnabledSortingBackToFront(true);

  // -----------------------------------------------------------------------
  // camera

  auto camEnt= scene->CreatePrefab<pref_gfx::Camera>()
    .Near(3.0f)
    .Far(10000.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(0.0f, 0.0f, 100.0f))
    .Create(win.GetDimensions());
  scene->CreatePrefab<pref_gfx::ArcBall>().Add(camEnt);
  scene->CreatePrefab<pref_input::ArcBallControl>()
    .RotationMultiplier(math_t::Vec2f::ONE * 0.01f)
    .PanMultiplier(math_t::Vec2f::ONE * 0.1f)
    .Add(camEnt);

  quadSys->SetCamera(camEnt);

  // -----------------------------------------------------------------------
  // load PNG into uniform
  gfx_gl::uniform_vso u_to;
  {
    gfx_med::ImageLoaderPng png;
    {
      core_io::Path p(core_str::String(GetAssetsPath()) + "/images/star.png");
      if (png.Load(p) != ErrorSuccess)
      { 
        TLOC_LOG_DEFAULT_ERR() << "Image " << p << " failed to load.";
      }
    }

    gfx_gl::texture_object_vso to;
    to->Initialize(*png.GetImage());

    u_to->SetName("s_texture").SetValueAs(*to);
  }

  // -----------------------------------------------------------------------
  // shared material

  auto matPtr = scene->CreatePrefab<pref_gfx::Material>()
    .AddUniform(u_to.get())
    .Create(core_io::Path(GetAssetsPath() + shaderPathVS), 
            core_io::Path(GetAssetsPath() + shaderPathFS))
            ->GetComponent<gfx_cs::Material>();

  // -----------------------------------------------------------------------
  // transparent quads

  const tl_int quadCount = 100;
  for (tl_int i = 0; i < quadCount; ++i)
  {
    auto x = core_rng::g_defaultRNG.GetRandomFloat(0.0f, 200.0f);
    auto y = core_rng::g_defaultRNG.GetRandomFloat(0.0f, 200.0f);
    auto z = core_rng::g_defaultRNG.GetRandomFloat(0.0f, 200.0f);

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(10.0f),
                           math_t::Rectf32_c::height(10.0f));
    core_cs::entity_vptr q =
      scene->CreatePrefab<pref_gfx::Quad>()
      .Dimensions(rect)
      .Create();
    q->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(x, y, z));

    scene->InsertComponent(q, matPtr);
  }

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  scene->Initialize();

  //------------------------------------------------------------------------
  // Main loop

  TLOC_LOG_DEFAULT_DEBUG() << "Press [f] to switch to front to back sorting";
  TLOC_LOG_DEFAULT_DEBUG() << "Press [b] to switch to back to front sorting";

  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    if (keyboard->IsKeyDown(input_hid::KeyboardEvent::f))
    { quadSys->SetEnabledSortingFrontToBack(true); }
    else if (keyboard->IsKeyDown(input_hid::KeyboardEvent::b))
    { quadSys->SetEnabledSortingBackToFront(true); }

    inputMgr->Update();

    scene->Update();
    scene->Process();

    renderer->ApplyRenderSettings();
    renderer->Render();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
