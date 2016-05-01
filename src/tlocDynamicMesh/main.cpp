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

  core_str::String shaderPathVS("/shaders/tlocTexturedMeshDispVS.glsl");
  core_str::String shaderPathFS("/shaders/tlocTexturedMeshFS.glsl");

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
    gfx_win::WindowSettings("SkyBox") );

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
   .AddClearBit<clear::ColorBufferBit>()
   .AddClearBit<clear::DepthBufferBit>();

  renderer->SetParams(p);

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> params;
  params.m_param1 = win.GetWindowHandle();

  auto inputMgr = core_sptr::MakeShared<input::InputManagerB>(params);

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  auto  keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();
  auto  mouse = inputMgr->CreateHID<input_hid::MouseB>();

  // Check pointers
  TLOC_LOG_CORE_WARN_IF(keyboard == nullptr) << "No keyboard detected";
  TLOC_LOG_CORE_WARN_IF(mouse == nullptr) << "No mouse detected";

  // -----------------------------------------------------------------------
  // ECS

  core_cs::ECS ecs;

  auto abcSys = ecs.AddSystem<input_cs::ArcBallControlSystem>("Update");

  ecs.AddSystem<gfx_cs::MaterialSystem>("Render");
  ecs.AddSystem<gfx_cs::ArcBallSystem>("Render");
  ecs.AddSystem<gfx_cs::CameraSystem>("Render");

  auto meshSys = ecs.AddSystem<gfx_cs::MeshRenderSystem>("Render");
  meshSys->SetRenderer(renderer);

  // -----------------------------------------------------------------------
  // Load the required resources

  auto to = app_res::f_resource::LoadImageAsTextureObject
    (core_io::Path (g_assetsPath + "/images/uv_grid_col.png"));

  // -----------------------------------------------------------------------

  // these vertices are indexed i.e. to make the actual triangles, we need to
  // use indices on this container to get our final mesh
  core_conts::Array<gfx_t::Vert3fpnt> vertices;
  const int xDiv = 8;
  const int yDiv = 8;
  for (int y = 0; y < yDiv; ++y)
  {
    for (int x = 0; x < xDiv; ++x)
    {
      decltype(vertices)::value_type v;

      auto xPos = (f32)x / 1.0f;
      auto yPos = (f32)y / 1.0f;

      v.SetPosition(math_t::Vec3f32(xPos, yPos, 0.0f));
      v.SetTexCoord(math_t::Vec2f32(xPos / (f32)(xDiv-1), yPos / (f32)(yDiv-1)));
      v.SetNormal(math_t::Vec3f32(0, 0, 1));
      vertices.push_back(v);
    }
  }

  // the indices specify the triangles using the vertices above - each vertex
  // may be shared multiple times
  core_conts::Array<s32> indices;
  for (int y = 0; y < yDiv - 1; ++y)
  {
    for (int x = 0; x < xDiv - 1; ++x)
    {
      auto index   = (y * yDiv) + x; // this row
      auto index_n = ( (y + 1) * yDiv) + x; // next row
      indices.push_back(index + 1);
      indices.push_back(index + 0);
      indices.push_back(index_n);

      indices.push_back(index + 1);
      indices.push_back(index_n);
      indices.push_back(index_n + 1);
    }
  }

  // these vertices are the ones actually passed to our Mesh prefab to generate
  // the mesh - notice how we use the indices to reference the original container
  // storing the shared vertex information
  core_conts::Array<gfx_t::Vert3fpnt> unpackedVerts;
  for (auto i : indices)
  { unpackedVerts.push_back(vertices[i]); }

  core_cs::entity_vptr ent =
    ecs.CreatePrefab<pref_gfx::Mesh>().Create(unpackedVerts);

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  gfx_gl::uniform_vso  u_lightDir;
  u_lightDir->SetName("u_lightDir").SetValueAs(math_t::Vec3f32(3.0f, 3.0f, 3.0f));

  ecs.CreatePrefab<pref_gfx::Material>()
    .AddUniform(u_to.get())
    .AddUniform(u_lightDir.get())
    .Add(ent, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));

  auto matPtr = ent->GetComponent<gfx_cs::Material>();
  matPtr->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewMatrix>();

  // The displacement container contains values to displace the vertex positions
  // in the shader
  core_conts::Array<math_t::Vec3f32> displacement;
  displacement.resize(vertices.size());

  // The attributeVBO, like the Uniform, is sent to the shader but unlike the
  // uniform, it's value in the shader is different for every vertex
  gfx_gl::attributeVBO_vso v_disp;
  v_disp->AddName("a_vertDisp");
  {
    core_conts::Array<math_t::Vec3f32> tempDisp;
    tempDisp.resize(indices.size());
    v_disp->SetValueAs<gfx_gl::p_vbo::target::ArrayBuffer, 
                       gfx_gl::p_vbo::usage::DynamicDraw>(tempDisp);
  }

  auto meshPtr = ent->GetComponent<gfx_cs::Mesh>();
  auto v_dispPtr = meshPtr->GetUserShaderOperator()->AddAttributeVBO(*v_disp);
  meshPtr->SetEnableUniform<gfx_cs::p_renderable::uniforms::k_normalMatrix>();

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library

  core_cs::entity_vptr m_cameraEnt =
    ecs.CreatePrefab<pref_gfx::Camera>()
    .Perspective(true)
    .Near(1.0f)
    .Far(100.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(10.0f, yDiv / 2.0f, 5.0f))
    .Create(win.GetDimensions());

  ecs.CreatePrefab<pref_gfx::ArcBall>().Focus(math_t::Vec3f32(0.0f, yDiv/2.0f, 0.0f)).Add(m_cameraEnt);
  ecs.CreatePrefab<pref_input::ArcBallControl>()
    .GlobalMultiplier(math_t::Vec2f(0.01f, 0.01f))
    .Add(m_cameraEnt);

  meshSys->SetCamera(m_cameraEnt);
  keyboard->Register(abcSys.get());
  mouse->Register(abcSys.get());

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  ecs.Initialize();

  //------------------------------------------------------------------------
  // In order to update at a pre-defined time interval, a timer must be created
  core_time::Timer32 inputFrameTime;
  inputFrameTime.Reset();

  bool quit = false;

  //------------------------------------------------------------------------
  // Main loop

  core_time::Timer dispTimer;

  const auto PI = 3.142f;
  auto phase = 0.0f; auto currAngle = 0.0f; auto counter = 0;
  while (win.IsValid() && !winCallback.m_endProgram && !quit)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    // displace every 10ms
    if (dispTimer.ElapsedSeconds() > 0.01f)
    {
      // start manipulating the vertex displacement - remember, this is indexed
      for (auto& d : displacement)
      {
        using core_rng::g_defaultRNG;
        auto z = math::Sin(math_t::Radian(math_t::Degree(currAngle)) + phase);

        counter += 1;
        if (counter == xDiv)
        {
          phase += (2 * PI) / (decltype(PI))xDiv;
          currAngle += 1.0f;
          counter = 0;
        }

        d[2] = z;
      }

      // now prepare the ACTUAL buffer so that we can pass it to the attributeVBO
      {
        core_conts::Array<math_t::Vec3f32> tempDisp;
        for (auto i : indices)
        { tempDisp.push_back(displacement[i]); }

        v_disp->UpdateData(tempDisp);
      }

      dispTimer.Reset();
    }

    //------------------------------------------------------------------------
    // Update Input
    if (win.IsValid() && inputFrameTime.ElapsedMilliSeconds() > 10)
    {
      inputFrameTime.Reset();

      // This updates all HIDs attached to the InputManager. An InputManager can
      // be updated at a different interval than the main render update.
      inputMgr->Update();

      ecs.Update(1.0/60.0);
      ecs.Process("Update", 1.0/100.0);
      ecs.Process("Render", 1.0/60.0);

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
