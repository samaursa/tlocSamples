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

  const float g_randMin = -50.0f;
  const float g_randMax =  50.0f;

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
  scene->AddSystem<gfx_cs::SceneGraphSystem>();

  auto quadSys = scene->AddSystem<gfx_cs::MeshRenderSystem>();
  quadSys->SetRenderer(renderer);
  quadSys->SetEnabledSortingBackToFront(false);
  quadSys->SetEnabledSortingFrontToBack(false);

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

  const tl_int quadCount = 200;
  const tl_int maxNumChildren = 3;
  tl_int nodeCounter = 0;
  core_cs::entity_vptr prevEnt;
  for (tl_int i = 0; i < quadCount; ++i)
  {
    auto x = core_rng::g_defaultRNG.GetRandomFloat(g_randMin, g_randMax);
    auto y = core_rng::g_defaultRNG.GetRandomFloat(g_randMin, g_randMax);
    auto z = core_rng::g_defaultRNG.GetRandomFloat(g_randMin, g_randMax);

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(10.0f),
                           math_t::Rectf32_c::height(10.0f));
    core_cs::entity_vptr q =
      scene->CreatePrefab<pref_gfx::Quad>()
      .Dimensions(rect)
      .Create();

    const auto pos = math_t::Vec3f32(x, y, z);

    // we're the parent node
    if (nodeCounter == 0)
    { scene->CreatePrefab<pref_gfx::SceneNode>().Position(pos).Add(q); }
    else // we're one of the children
    {
      scene->CreatePrefab<pref_gfx::SceneNode>()
        .Position(pos)
        .Parent(core_sptr::ToVirtualPtr(q->GetComponent<gfx_cs::SceneNode>()))
        .Add(q);
    }

    if (nodeCounter == maxNumChildren)
    { nodeCounter = 0; }

    prevEnt = q;

    scene->InsertComponent(q, matPtr);
  }

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  scene->Initialize();

  //------------------------------------------------------------------------
  // Main loop

  TLOC_LOG_DEFAULT_DEBUG() << "Press [f] to switch to front to back sorting";
  TLOC_LOG_DEFAULT_DEBUG() << "Press [b] to switch to back to front sorting";
  TLOC_LOG_DEFAULT_DEBUG() << "Press [g] to switch to front to back 2D sorting";
  TLOC_LOG_DEFAULT_DEBUG() << "Press [n] to switch to back to front 2D sorting";

  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    if (keyboard->IsKeyDown(input_hid::KeyboardEvent::f))
    { quadSys->SetEnabledSortingFrontToBack(true); }
    else if (keyboard->IsKeyDown(input_hid::KeyboardEvent::b))
    { quadSys->SetEnabledSortingBackToFront(true); }
    else if (keyboard->IsKeyDown(input_hid::KeyboardEvent::g))
    { quadSys->SetEnabledSortingFrontToBack_2D(true); }
    else if (keyboard->IsKeyDown(input_hid::KeyboardEvent::n))
    { quadSys->SetEnabledSortingBackToFront_2D(true); }

    inputMgr->Update();

    scene->Update(1.0/60.0);
    scene->Process(1.0/60.0);

    renderer->ApplyRenderSettings();
    renderer->Render();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
