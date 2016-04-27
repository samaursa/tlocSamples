#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>

#include <tlocApplication/tloc_application.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocInput/tloc_input.h>

#include <gameAssetsPath.h>

using namespace tloc;

namespace {

  // -----------------------------------------------------------------------
  // We need a material to attach to our entity (which we have not yet created).

  core_str::String shaderPathVS("/shaders/tlocTexturedMeshVS.glsl");
  core_str::String shaderPathFS("/shaders/tlocTexturedMeshPlusStencilFS.glsl");

  core_str::String shaderPathOneTextureBillboardVS("/shaders/tlocOneTextureBillboardVS.glsl");
  core_str::String shaderPathOneTextureBillboardFS("/shaders/tlocOneTexturePlusLightStencilFS.glsl");

  core_str::String shaderPathGodrayVS("/shaders/tlocPostProcessGodrayVS.glsl");
  core_str::String shaderPathGodrayFS("/shaders/tlocPostProcessGodrayFS.glsl");

  core_str::String shaderPathAdditiveVS("/shaders/tlocOneTextureVS.glsl");
  core_str::String shaderPathAdditiveFS("/shaders/tlocAdditiveBlendingFS.glsl");

  core_str::String g_assetsPath = GetAssetsPath();

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

template <typename T_UniformPtr, typename T>
void ModifyUniform(T_UniformPtr a_uniform, T a_min, T a_max, T a_step)
{
  auto a_value = a_uniform->GetValueAs<T>();
  a_value += a_step;
  a_value = core::Clamp(a_value, a_min, a_max);

  a_uniform->SetValueAs(a_value);
  TLOC_LOG_DEFAULT_DEBUG() << a_uniform->GetName() << ": " << a_value;
}

int TLOC_MAIN(int, char**)
{
  gfx_win::Window win;
  WindowCallback  winCallback;
  win.Register(&winCallback);

  const tl_int winWidth = 800;
  const tl_int winHeight = 600;

  win.Create( gfx_win::Window::graphics_mode::Properties(winWidth, winHeight),
    gfx_win::WindowSettings("Godrays") );

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
    p.SetClearColor(gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f))
     .Enable<enable_disable::DepthTest>()
     .AddClearBit<clear::ColorBufferBit>()
     .AddClearBit<clear::DepthBufferBit>();

    renderer->SetParams(p);
  }

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> params;
  params.m_param1 = win.GetWindowHandle();

  auto inputMgr = core_sptr::MakeShared<input::InputManagerB>(params);

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  auto keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();
  auto mouse = inputMgr->CreateHID<input_hid::MouseB>();

  // Check pointers
  TLOC_LOG_CORE_WARN_IF(keyboard == nullptr) << "No keyboard detected";
  TLOC_LOG_CORE_WARN_IF(mouse == nullptr) << "No mouse detected";

  // -----------------------------------------------------------------------

  gfx::Rtt rtt(win.GetDimensions());
  auto toColScene = rtt.AddColorAttachment(0);
  auto toColStencil = rtt.AddColorAttachment(1);
  rtt.AddDepthAttachment();
  {
    auto rttRenderParams = rtt.GetRenderer()->GetParams();
    rttRenderParams.Enable<enable_disable::Blend>()
                   .SetClearColor(gfx_t::Color(0.1f, 0.1f, 0.1f, 0.0f))
                   .SetBlendFunction<blend_function::SourceAlpha, 
                                     blend_function::OneMinusSourceAlpha>();
    rtt.GetRenderer()->SetParams(rttRenderParams);
  }

  gfx::Rtt rttGodrayPass(core_ds::Divide(1u, win.GetDimensions()));
  auto toGodray = rttGodrayPass.AddColorAttachment(0);
  {
    auto texParams = toGodray->GetParams();
    texParams.Wrap_R<gfx_gl::p_texture_object::wrap_technique::Repeat>();
    texParams.Wrap_S<gfx_gl::p_texture_object::wrap_technique::Repeat>();
    toGodray->SetParams(texParams);
  }
  auto godrayRenderer = rttGodrayPass.GetRenderer();

  // -----------------------------------------------------------------------
  // ECS

  core_cs::ECS rttScene, godrayScene, mainScene;

  rttScene.AddSystem<gfx_cs::ArcBallSystem>("Update");
  auto abcSys = rttScene.AddSystem<input_cs::ArcBallControlSystem>("Update");
  mouse->Register(abcSys.get());
  keyboard->Register(abcSys.get());

  rttScene.AddSystem<gfx_cs::MaterialSystem>("Render");
  rttScene.AddSystem<gfx_cs::CameraSystem>("Render");

  auto meshSys = rttScene.AddSystem<gfx_cs::MeshRenderSystem>("Render");
  meshSys->SetRenderer(rtt.GetRenderer());

  godrayScene.AddSystem<gfx_cs::MaterialSystem>("Render");
  auto quadSysGodray = godrayScene.AddSystem<gfx_cs::MeshRenderSystem>("Render");
  quadSysGodray->SetRenderer(godrayRenderer);

  mainScene.AddSystem<gfx_cs::MaterialSystem>("Render");
  {
    auto quadSys = mainScene.AddSystem<gfx_cs::MeshRenderSystem>("Render");
    quadSys->SetRenderer(renderer);
  }

  // -----------------------------------------------------------------------
  // ObjLoader can load (basic) .obj files

  auto objLoader = gfx_med::f_obj_loader::LoadObjFile
    (core_io::Path(g_assetsPath + "/models/Crate.obj"));

  gfx_med::ObjLoader::vert_cont_type vertices;
  objLoader->GetUnpacked(vertices, 0);

  // -----------------------------------------------------------------------
  // Create the mesh and add the material

  gfx_gl::uniform_vso  u_toLight;
  {
    auto to = app_res::f_resource::LoadImageAsTextureObject
      (core_io::Path(g_assetsPath + "/images/light.png"));
    u_toLight->SetName("s_texture").SetValueAs(*to);
  }

  using math_t::Rectf32_c;
  auto rect = Rectf32_c(Rectf32_c::width(3.0f), Rectf32_c::height(3.0f));
  auto light = rttScene.CreatePrefab<pref_gfx::Quad>().Dimensions(rect).Create();
  rttScene.CreatePrefab<pref_gfx::Material>()
    .AddUniform(u_toLight.get())
    .Add(light, core_io::Path(g_assetsPath + shaderPathOneTextureBillboardVS), 
                core_io::Path(g_assetsPath + shaderPathOneTextureBillboardFS));
  light->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(10.0f, 5.0f, 10.0f));

  auto lightMat = light->GetComponent<gfx_cs::Material>();
  lightMat->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewProjectionMatrix>(false);
  lightMat->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewMatrix>();
  lightMat->SetEnableUniform<gfx_cs::p_material::uniforms::k_projectionMatrix>();

  auto ent =
    rttScene.CreatePrefab<pref_gfx::Mesh>().Create(vertices);
  ent->GetComponent<gfx_cs::Mesh>()->
    SetEnableUniform<gfx_cs::p_renderable::uniforms::k_normalMatrix>();

  gfx_gl::uniform_vso  u_to;
  {
    auto to = app_res::f_resource::LoadImageAsTextureObject
      (core_io::Path(g_assetsPath + "/images/crateTexture.png"));
    u_to->SetName("s_texture").SetValueAs(*to);
  }

  gfx_gl::uniform_vso  u_lightPos;
  u_lightPos->SetName("u_lightDir").SetValueAs(light->GetComponent<math_cs::Transform>()->GetPosition());

  rttScene.CreatePrefab<pref_gfx::Material>()
    .AddUniform(u_to.get())
    .AddUniform(u_lightPos.get())
    .Add(ent, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));

  auto matPtr = ent->GetComponent<gfx_cs::Material>();
  matPtr->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewMatrix>();
  
  // -----------------------------------------------------------------------

  auto rttRect = Rectf32_c(Rectf32_c::width(2.0f), Rectf32_c::height(2.0f));
  auto rttQuadGodRay = godrayScene.CreatePrefab<pref_gfx::Quad>().Dimensions(rttRect).Create();
  auto matPref = godrayScene.CreatePrefab<pref_gfx::Material>();
  {
    gfx_gl::uniform_vso  u_toStencil;
    u_toStencil->SetName("s_stencil").SetValueAs(*toColStencil);

    gfx_gl::uniform_vso  u_numSamples;
    u_numSamples->SetName("u_numSamples").SetValueAs(200);

    gfx_gl::uniform_vso  u_density;
    u_density->SetName("u_density").SetValueAs(0.2f);

    gfx_gl::uniform_vso  u_decay;
    u_decay->SetName("u_decay").SetValueAs(0.98f);

    gfx_gl::uniform_vso  u_weight;
    u_weight->SetName("u_weight").SetValueAs(0.5f);

    gfx_gl::uniform_vso  u_exposure;
    u_exposure->SetName("u_exposure").SetValueAs(0.16f);
    
    gfx_gl::uniform_vso  u_illumDecay;
    u_illumDecay->SetName("u_illumDecay").SetValueAs(0.8f);

    matPref
      .AddUniform(u_toStencil.get())
      .AddUniform(u_lightPos.get())
      .AddUniform(u_numSamples.get())
      .AddUniform(u_density.get())
      .AddUniform(u_decay.get())
      .AddUniform(u_weight.get())
      .AddUniform(u_exposure.get())
      .AddUniform(u_illumDecay.get())
      .Add(rttQuadGodRay, core_io::Path(g_assetsPath + shaderPathGodrayVS), 
                          core_io::Path(g_assetsPath + shaderPathGodrayFS));
  }

  auto godraySO     = rttQuadGodRay->GetComponent<gfx_cs::Material>()->GetShaderOperator();
  auto u_numSamples = gfx_gl::f_shader_operator::GetUniform(*godraySO, "u_numSamples");
  auto u_density    = gfx_gl::f_shader_operator::GetUniform(*godraySO, "u_density");
  auto u_decay      = gfx_gl::f_shader_operator::GetUniform(*godraySO, "u_decay");
  auto u_weight     = gfx_gl::f_shader_operator::GetUniform(*godraySO, "u_weight");
  auto u_exposure   = gfx_gl::f_shader_operator::GetUniform(*godraySO, "u_exposure");
  auto u_illumDecay = gfx_gl::f_shader_operator::GetUniform(*godraySO, "u_illumDecay");

  rttQuadGodRay->GetComponent<gfx_cs::Material>()->
    SetEnableUniform<gfx_cs::p_material::uniforms::k_viewProjectionMatrix>();
  rttQuadGodRay->GetComponent<gfx_cs::Mesh>()->
    SetEnableUniform<gfx_cs::p_renderable::uniforms::k_modelMatrix>(false);

  // -----------------------------------------------------------------------

  gfx_gl::uniform_vso  u_toScene;
  u_toScene->SetName("s_texture").SetValueAs(*toColScene);

  gfx_gl::uniform_vso  u_toProcessedGodray;
  u_toProcessedGodray->SetName("s_texture2").SetValueAs(*toGodray);

  auto rttFinalScene = mainScene.CreatePrefab<pref_gfx::Quad>().Dimensions(rttRect).Create();
  mainScene.CreatePrefab<pref_gfx::Material>()
    .AddUniform(u_toScene.get())
    .AddUniform(u_toProcessedGodray.get())
    .Add(rttFinalScene, core_io::Path(g_assetsPath + shaderPathAdditiveVS), 
                        core_io::Path(g_assetsPath + shaderPathAdditiveFS));

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library

  core_cs::entity_vptr m_cameraEnt =
    rttScene.CreatePrefab<pref_gfx::Camera>()
    .Perspective(true)
    .Near(1.0f)
    .Far(100.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(0.0f, 20.0f, 20.0f))
    .Create(win.GetDimensions());

  rttScene.CreatePrefab<pref_gfx::ArcBall>().Add(m_cameraEnt);
  rttScene.CreatePrefab<pref_input::ArcBallControl>()
    .GlobalMultiplier(math_t::Vec2f(0.01f, 0.01f))
    .Add(m_cameraEnt);

  meshSys->SetCamera(m_cameraEnt);
  quadSysGodray->SetCamera(m_cameraEnt);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  godrayScene.Initialize();
  rttScene.Initialize();
  mainScene.Initialize();

  //------------------------------------------------------------------------
  // In order to update at a pre-defined time interval, a timer must be created
  core_time::Timer32 inputFrameTime;
  inputFrameTime.Reset();

  TLOC_LOG_CORE_DEBUG() << 
    "Press ALT and Left, Middle and Right mouse buttons to manipulate the camera";
  TLOC_LOG_CORE_DEBUG() << "Press n/N to increase/decrease number of samples";
  TLOC_LOG_CORE_DEBUG() << "Press p/P to increase/decrease density of (fake) particles";
  TLOC_LOG_CORE_DEBUG() << "Press d/D to increase/decrease effect decay";
  TLOC_LOG_CORE_DEBUG() << "Press w/W to increase/decrease weighting of each sample";
  TLOC_LOG_CORE_DEBUG() << "Press e/E to increase/decrease exposure";
  TLOC_LOG_CORE_DEBUG() << "Press i/I to increase/decrease illumination decay";

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    //------------------------------------------------------------------------
    // Update Input
    if (win.IsValid() && inputFrameTime.ElapsedMilliSeconds() > 16)
    {
      if(keyboard->IsKeyDown(input_hid::KeyboardEvent::n))
      {
        if (keyboard->IsModifierDown(input_hid::KeyboardEvent::Shift))
        { ModifyUniform(u_numSamples, 0, 500, -1); }
        else
        { ModifyUniform(u_numSamples, 0, 500, 1); }
      }
      if(keyboard->IsKeyDown(input_hid::KeyboardEvent::p))
      {
        if (keyboard->IsModifierDown(input_hid::KeyboardEvent::Shift))
        { ModifyUniform(u_density, 0.0f, 10.0f, -0.01f); }
        else
        { ModifyUniform(u_density, 0.0f, 10.0f, 0.01f); }
      }
      if(keyboard->IsKeyDown(input_hid::KeyboardEvent::d))
      {
        if (keyboard->IsModifierDown(input_hid::KeyboardEvent::Shift))
        { ModifyUniform(u_decay, 0.0f, 1.0f, -0.001f); }
        else
        { ModifyUniform(u_decay, 0.0f, 1.0f, 0.001f); }
      }
      if(keyboard->IsKeyDown(input_hid::KeyboardEvent::w))
      {
        if (keyboard->IsModifierDown(input_hid::KeyboardEvent::Shift))
        { ModifyUniform(u_weight, 0.0f, 1.0f, -0.01f); }
        else
        { ModifyUniform(u_weight, 0.0f, 1.0f, 0.01f); }
      }
      if(keyboard->IsKeyDown(input_hid::KeyboardEvent::e))
      {
        if (keyboard->IsModifierDown(input_hid::KeyboardEvent::Shift))
        { ModifyUniform(u_exposure, 0.0f, 1.0f, -0.01f); }
        else
        { ModifyUniform(u_exposure, 0.0f, 1.0f, 0.01f); }
      }
      if(keyboard->IsKeyDown(input_hid::KeyboardEvent::i))
      {
        if (keyboard->IsModifierDown(input_hid::KeyboardEvent::Shift))
        { ModifyUniform(u_illumDecay, 0.0f, 1.0f, -0.01f); }
        else
        { ModifyUniform(u_illumDecay, 0.0f, 1.0f, 0.01f); }
      }

      inputFrameTime.Reset();

      // This updates all HIDs attached to the InputManager. An InputManager can
      // be updated at a different interval than the main render update.
      inputMgr->Update();

      rttScene.Update(1.0 / 60.0);
      rttScene.Process(1.0 / 60.0);

      rtt.GetRenderer()->ApplyRenderSettings();
      rtt.GetRenderer()->Render();

      godrayScene.Update(1.0 / 60.0f);
      godrayScene.Process(1.0 / 60.0f);

      godrayRenderer->ApplyRenderSettings();
      godrayRenderer->Render();

      mainScene.Process(1.0 / 60.0);

      renderer->ApplyRenderSettings();
      renderer->Render();

      win.SwapBuffers();
      inputMgr->Reset();
    }
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
