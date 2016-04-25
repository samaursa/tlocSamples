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

int TLOC_MAIN(int, char**)
{
  gfx_win::Window win;
  WindowCallback  winCallback;
  win.Register(&winCallback);

  const tl_int winWidth = 800;
  const tl_int winHeight = 600;

  win.Create( gfx_win::Window::graphics_mode::Properties(winWidth, winHeight),
    gfx_win::WindowSettings("FPS Camera") );

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

  input::input_mgr_i_ptr inputMgr =
    core_sptr::MakeShared<input::InputManagerI>(params);

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::keyboard_i_vptr      keyboard = 
    inputMgr->CreateHID<input_hid::KeyboardI>();
  input_hid::mouse_i_vptr         mouse = 
    inputMgr->CreateHID<input_hid::MouseI>();

  // Check pointers
  TLOC_LOG_CORE_WARN_IF(keyboard == nullptr) << "No keyboard detected";
  TLOC_LOG_CORE_WARN_IF(mouse == nullptr) << "No mouse detected";

  // -----------------------------------------------------------------------

  gfx::Rtt rtt(win.GetDimensions());
  auto toColScene = rtt.AddColorAttachment(0);
  auto toColStencil = core_sptr::MakeShared<gfx_gl::TextureObject>();
  {
    gfx_med::Image img;
    img.Create(win.GetDimensions());
    toColStencil->Initialize(img);
  }
  rtt.AddColorAttachment(1, toColStencil);
  rtt.AddDepthAttachment();
  {
    auto rttRenderParams = rtt.GetRenderer()->GetParams();
    rttRenderParams.Enable<enable_disable::Blend>()
                   .SetClearColor(gfx_t::Color(0.1f, 0.1f, 0.1f, 1.0f))
                   .SetBlendFunction<blend_function::SourceAlpha, 
                                     blend_function::OneMinusSourceAlpha>();
    rtt.GetRenderer()->SetParams(rttRenderParams);
  }

  // -----------------------------------------------------------------------
  // ECS

  core_cs::ECS rttScene, mainScene;

  rttScene.AddSystem<gfx_cs::MaterialSystem>("Render");
  rttScene.AddSystem<gfx_cs::CameraSystem>("Render");

  auto meshSys = rttScene.AddSystem<gfx_cs::MeshRenderSystem>("Render");
  meshSys->SetRenderer(rtt.GetRenderer());

  mainScene.AddSystem<gfx_cs::MaterialSystem>("Render");
  auto quadSys = mainScene.AddSystem<gfx_cs::MeshRenderSystem>("Render");
  quadSys->SetRenderer(renderer);

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

  gfx_gl::uniform_vso  u_toScene;
  u_toScene->SetName("s_texture").SetValueAs(*toColScene);

  gfx_gl::uniform_vso  u_toStencil;
  u_toStencil->SetName("s_stencil").SetValueAs(*toColStencil);

  auto rttRect = Rectf32_c(Rectf32_c::width(2.0f), Rectf32_c::height(2.0f));
  auto rttQuad = mainScene.CreatePrefab<pref_gfx::Quad>().Dimensions(rttRect).Create();
  mainScene.CreatePrefab<pref_gfx::Material>()
    .AddUniform(u_toScene.get())
    .AddUniform(u_toStencil.get())
    .AddUniform(u_lightPos.get())
    .Add(rttQuad, core_io::Path(g_assetsPath + shaderPathGodrayVS), 
                core_io::Path(g_assetsPath + shaderPathGodrayFS));

  rttQuad->GetComponent<gfx_cs::Material>()->
    SetEnableUniform<gfx_cs::p_material::uniforms::k_viewProjectionMatrix>();
  rttQuad->GetComponent<gfx_cs::Mesh>()->
    SetEnableUniform<gfx_cs::p_renderable::uniforms::k_modelMatrix>(false);

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library

  core_cs::entity_vptr m_cameraEnt =
    rttScene.CreatePrefab<pref_gfx::Camera>()
    .Perspective(true)
    .Near(1.0f)
    .Far(100.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(0.0f, 0.0f, 5.0f))
    .Create(win.GetDimensions());

  meshSys->SetCamera(m_cameraEnt);
  quadSys->SetCamera(m_cameraEnt);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  rttScene.Initialize();
  mainScene.Initialize();

  //------------------------------------------------------------------------
  // In order to update at a pre-defined time interval, a timer must be created
  core_time::Timer32 inputFrameTime;
  inputFrameTime.Reset();

  win.SetMouseVisibility(false);
  win.ConfineMouseToWindow(true);

  f32 pitch = 0.0f;
  f32 yaw = 0.0f;
  bool quit = false;

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram && !quit)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    //------------------------------------------------------------------------
    // Update Input
    if (win.IsValid() && inputFrameTime.ElapsedMilliSeconds() > 16)
    {
      inputFrameTime.Reset();

      // This updates all HIDs attached to the InputManager. An InputManager can
      // be updated at a different interval than the main render update.
      inputMgr->Update();

      // process input
      {
        auto camTrans = m_cameraEnt->GetComponent<math_cs::Transform>();
        auto camDir = camTrans->GetOrientation().GetCol(2).ConvertTo<math_t::Vec3f32>();
        auto camLeft = camTrans->GetOrientation().GetCol(0).ConvertTo<math_t::Vec3f32>();

        camDir *= 0.1f;
        camLeft *= 0.1f;

        // translation
        if (keyboard->IsKeyDown(input_hid::KeyboardEvent::w))
        { camTrans->SetPosition(camTrans->GetPosition() - camDir); }
        if (keyboard->IsKeyDown(input_hid::KeyboardEvent::s))
        { camTrans->SetPosition(camTrans->GetPosition() + camDir); }
        if (keyboard->IsKeyDown(input_hid::KeyboardEvent::a))
        { camTrans->SetPosition(camTrans->GetPosition() - camLeft); }
        if (keyboard->IsKeyDown(input_hid::KeyboardEvent::d))
        { camTrans->SetPosition(camTrans->GetPosition() + camLeft); }
        if (keyboard->IsKeyDown(input_hid::KeyboardEvent::q))
        { quit = true; }

        auto mouseState = mouse->GetState();
        f32 absX = (f32)mouseState.m_X.m_rel;
        f32 absY = (f32)mouseState.m_Y.m_rel;

        pitch += absY * 0.1f;
        yaw += absX * 0.1f;

        auto rotX = math_t::Mat3f32::IDENTITY;
        rotX.MakeRotationX(math_t::Degree(-pitch));

        auto rotY = math_t::Mat3f32::IDENTITY;
        rotY.MakeRotationY(math_t::Degree(-yaw));

        auto finalRot = rotY * rotX;
        camTrans->SetOrientation(finalRot);
      }

      rttScene.Update(1.0 / 60.0);
      rttScene.Process(1.0 / 60.0);

      rtt.GetRenderer()->ApplyRenderSettings();
      rtt.GetRenderer()->Render();

      mainScene.Update(1.0 / 60.0);
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
