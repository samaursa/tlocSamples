#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>

#include <tlocGraphics/tloc_graphics.h>
#include <tlocGraphics/tloc_graphics.inl.h>

#include <tlocMath/tloc_math.h>
#include <tlocMath/tloc_math.inl.h>

#include <tlocPhysics/tloc_physics.h>
#include <tlocPhysics/tloc_physics.inl.h>

#include <tlocInput/tloc_input.h>
#include <tlocInput/tloc_input.inl.h>

#include <tlocPrefab/tloc_prefab.h>
#include <tlocPrefab/tloc_prefab.inl.h>

#include <gameAssetsPath.h>

using namespace tloc;

#define PROFILE_START()\
    m_timer.Reset()

#ifdef _MSC_VER
  #pragma warning(disable:4127)
#endif
#define PROFILE_END(_text_)\
  do{\
    auto timeInMs = core_utils::CastNumber<s32>(m_timer.ElapsedMicroSeconds());\
    TLOC_LOG_CORE_DEBUG() << #_text_ << ": " << timeInMs << " us";\
  }while(0)

namespace {

  enum
  {
    key_pause = 0,
    key_exit,
    key_cameraPersp,
    key_count
  };

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathVS("/shaders/mvpTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathVS("/shaders/mvpTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathFS("/shaders/mvpTextureFS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathFS("/shaders/mvpTextureFS_gl_es_2_0.glsl");
#endif

};

struct glProgram
{
  typedef gfx_cs::Quad          quad_type;
  typedef gfx_cs::Material      mat_type;
  typedef core_cs::Entity       ent_type;
  typedef core_cs::entity_vptr  ent_ptr;

  typedef phys_box2d::PhysicsManager        phys_mgr_type;

  typedef gfx_win::WindowEvent              event_type;
  typedef gfx_win::Window::graphics_mode    graphics_mode;

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  glProgram()
    : m_endGame(false)
    , m_keyPresses(key_count)
    , m_entityMgr(m_eventMgr.get())

  { m_win.Register(this); }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  void Initialize()
  {
    // trying to match iPad retina display ratio (not resolution)
    m_win.Create(graphics_mode::Properties(800, 600),
                 gfx_win::WindowSettings("Atom & Eve"));

    ParamList<core_t::Any> params;
    params.m_param1 = m_win.GetWindowHandle();
    m_inputMgr = core_sptr::MakeShared<input::InputManagerB>(params);

    m_keyboard = m_inputMgr->CreateHID<input::hid::KeyboardB>();
    m_keyboard->Register(this);

    //------------------------------------------------------------------------
    // Initialize graphics platform
    if (gfx_gl::InitializePlatform() != ErrorSuccess)
    { TLOC_ASSERT_FALSE("Graphics platform failed to initialize"); exit(0); }

    // -----------------------------------------------------------------------
    // Get the default renderer
    m_renderer = m_win.GetRenderer();
    gfx_rend::Renderer::Params p(m_renderer->GetParams());
    p.AddClearBit<gfx_rend::p_renderer::clear::ColorBufferBit>()
     .AddClearBit<gfx_rend::p_renderer::clear::DepthBufferBit>();
    m_renderer->SetParams(p);

    phys_mgr_type::error_type result =
      m_physicsMgr.Initialize(phys_mgr_type::gravity(math_t::Vec2f(0.0f, -10.0f)),
                              phys_mgr_type::velocity_iterations(6),
                              phys_mgr_type::position_iterations(2));

    TLOC_ASSERT(result == ErrorSuccess, "Physics failed to initialize!");
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_conts::Array<math_t::Vec4f32> GetQuadColor()
  {
    using namespace core;
    using namespace core_conts;
    using namespace math_t;

    Array<Vec4f32> colorData;

    { // set the color
      using gfx_t::Color;
      using gfx_t::p_color::format::RGBA;

      Color triColor(1.0f, 0.0f, 0.0f, 1.0f);
      Vec4f32 colVec;
      triColor.GetAs<RGBA>(colVec);
      colorData.push_back(colVec);

      triColor.SetAs(0.0f, 1.0f, 0.0f, 1.0f);
      triColor.GetAs<RGBA>(colVec);
      colorData.push_back(colVec);

      triColor.SetAs(0.0f, 0.0f, 1.0f, 1.0f);
      triColor.GetAs<RGBA>(colVec);
      colorData.push_back(colVec);

      triColor.SetAs(0.5f, 1.0f, 1.0f, 1.0f);
      triColor.GetAs<RGBA>(colVec);
      colorData.push_back(colVec);
    }

    return colorData;
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  void RunGame()
  {
    using namespace tloc;
    using namespace core;
    using namespace math;
    using namespace graphics;
    using namespace physics;

    using math::types::Rectf32_c;
    using math::types::Circlef32;

    using gfx_cs::CameraSystem;
    using gfx_cs::MaterialSystem;
    using gfx_cs::MeshRenderSystem;
    using phys_cs::RigidBodySystem;
    using gfx_cs::DebugTransformRenderSystem;

    using core_str::String;

    //------------------------------------------------------------------------
    // Systems and Entity Preparation
    MeshRenderSystem  meshSys   (m_eventMgr.get(), m_entityMgr.get());
    CameraSystem      camSys    (m_eventMgr.get(), m_entityMgr.get());
    MaterialSystem    matSys    (m_eventMgr.get(), m_entityMgr.get());
    RigidBodySystem   physicsSys(m_eventMgr.get(), m_entityMgr.get(),
                                 &m_physicsMgr.GetWorld());

    DebugTransformRenderSystem dtrSys(m_eventMgr.get(), m_entityMgr.get());
    dtrSys.SetScale(2.0f);

    // attach the default renderer to both rendering systems
    meshSys.SetRenderer(m_renderer);

    //------------------------------------------------------------------------
    // Create the uniforms holding the texture objects

    gl::uniform_vso  u_crateTo;
    {
      gfx_gl::texture_object_sptr crateTo =
        core_sptr::MakeShared<gfx_gl::TextureObject>();
      {
        gfx_med::ImageLoaderPng png;
        {
          core_str::String filePath(GetAssetsPath());
          filePath += "/images/crate.png";
          core_io::Path path(filePath.c_str());
          if (png.Load(path) != ErrorSuccess)
          { TLOC_ASSERT_FALSE("Image did not load"); }
        }
        crateTo->Initialize(*png.GetImage());
      }
      u_crateTo->SetName("shaderTexture").SetValueAs(*crateTo);
    }

    gl::uniform_vso  u_henryTo;
    {
      gfx_gl::texture_object_sptr to =
        core_sptr::MakeShared<gfx_gl::TextureObject>();
      {
        gfx_med::ImageLoaderPng png;
        {
          core_str::String filePath(GetAssetsPath());
          filePath += "/images/henry.png";
          core_io::Path path(filePath.c_str());

          if (png.Load(path) != ErrorSuccess)
          { TLOC_ASSERT_FALSE("Image did not load"); }
        }
        to->Initialize(*png.GetImage());
      }
      u_henryTo->SetName("shaderTexture").SetValueAs(*to);
    }

    gfx_cs::material_sptr crateMat =
      pref_gfx::Material(m_entityMgr.get(), m_compPoolMgr.get())
      .AddUniform(u_crateTo.get())
      .Create(core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS))
              ->GetComponent<gfx_cs::Material>();
    crateMat->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewProjectionMatrix>(false);

    gfx_cs::material_sptr henryMat =
      pref_gfx::Material(m_entityMgr.get(), m_compPoolMgr.get())
      .AddUniform(u_henryTo.get())
      .Create(core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS))
              ->GetComponent<gfx_cs::Material>();
    henryMat->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewProjectionMatrix>(false);

    PROFILE_START();
    const tl_int repeat = 300;
    for (tl_int i = repeat + 1; i > 0; --i)
    {
      tl_float posX = rng::g_defaultRNG.GetRandomFloat(-50.0f, 50.0f);
      tl_float posY = rng::g_defaultRNG.GetRandomFloat(20.0f, 480.0f);

      if (rng::g_defaultRNG.GetRandomInteger(0, 2) == 1)
      {
        //Create a quad ent
        Rectf32_c rect(Rectf32_c::width(3.0f), Rectf32_c::height(3.0f));
        ent_ptr quadEnt =
          pref_gfx::Quad(m_entityMgr.get(), m_compPoolMgr.get())
          .Dimensions(rect).Create();
        quadEnt->GetComponent<gfx_cs::Mesh>()->
          SetEnableUniform<gfx_cs::p_renderable::uniforms::k_modelViewProjectionMatrix>()
          .SetEnableUniform<gfx_cs::p_renderable::uniforms::k_modelMatrix>(false);

        box2d::rigid_body_def_sptr rbDef =
          core_sptr::MakeShared<box2d::RigidBodyDef>();
        rbDef->SetPosition(box2d::RigidBodyDef::vec_type(posX, posY));
        rbDef->SetType<box2d::p_rigid_body::DynamicBody>();

        pref_phys::RigidBody(m_entityMgr.get(), m_compPoolMgr.get())
          .Add(quadEnt, rbDef);
        pref_phys::RigidBodyShape(m_entityMgr.get(), m_compPoolMgr.get())
          .Add(quadEnt, rect, pref_phys::RigidBodyShape::density(1.0f));

        m_entityMgr->InsertComponent(core_cs::EntityManager::Params(quadEnt, crateMat));
      }
      else
      {
        // Create a fan ent
        Circlef32 circle( Circlef32::radius(1.5f) );
        ent_ptr fanEnt = pref_gfx::Fan(m_entityMgr.get(), m_compPoolMgr.get())
          .Sides(30)
          .Circle(circle)
          .Create();

        fanEnt->GetComponent<gfx_cs::Mesh>()->
          SetEnableUniform<gfx_cs::p_renderable::uniforms::k_modelViewProjectionMatrix>()
          .SetEnableUniform<gfx_cs::p_renderable::uniforms::k_modelMatrix>(false);

        box2d::rigid_body_def_sptr rbDef =
          core_sptr::MakeShared<box2d::RigidBodyDef>();
        rbDef->SetPosition(box2d::RigidBodyDef::vec_type(posX, posY));
        rbDef->SetType<box2d::p_rigid_body::DynamicBody>();
        pref_phys::RigidBody(m_entityMgr.get(), m_compPoolMgr.get())
          .Add(fanEnt, rbDef);

        box2d::RigidBodyShapeDef rbShape(circle);
        rbShape.SetRestitution(1.0f);
        pref_phys::RigidBodyShape(m_entityMgr.get(), m_compPoolMgr.get())
          .Add(fanEnt, rbShape);

        m_entityMgr->InsertComponent(core_cs::EntityManager::Params(fanEnt, henryMat));
      }
    }
    PROFILE_END("Generating Quads and Fans");

    {
      // Create a fan ent
      Circlef32 circle( Circlef32::radius(5.0f) );
      ent_ptr fanEnt = pref_gfx::Fan(m_entityMgr.get(), m_compPoolMgr.get())
        .Sides(30).Circle(circle).Create();

      fanEnt->GetComponent<gfx_cs::Mesh>()->
        SetEnableUniform<gfx_cs::p_renderable::uniforms::k_modelViewProjectionMatrix>()
        .SetEnableUniform<gfx_cs::p_renderable::uniforms::k_modelMatrix>(false);

      box2d::rigid_body_def_sptr rbDef =
        core_sptr::MakeShared<box2d::RigidBodyDef>();
      rbDef->SetType<box2d::p_rigid_body::StaticBody>();
      rbDef->SetPosition(box2d::RigidBodyDef::vec_type(0.0f, -10.f));
      pref_phys::RigidBody(m_entityMgr.get(), m_compPoolMgr.get())
        .Add(fanEnt, rbDef);

      box2d::RigidBodyShapeDef rbCircleShape(circle);
      pref_phys::RigidBodyShape(m_entityMgr.get(), m_compPoolMgr.get())
        .Add(fanEnt, rbCircleShape);

        m_entityMgr->InsertComponent(core_cs::EntityManager::Params(fanEnt, henryMat));
    }

    // -----------------------------------------------------------------------
    // Create the camera from the prefab library

    m_cameraEnt = pref_gfx::Camera(m_entityMgr.get(), m_compPoolMgr.get())
      .Near(0.1f)
      .Far(10.0f)
      .Perspective(false)
      .Position(math_t::Vec3f(0, 0, 1.0f))
      .Create(core_ds::Divide<tl_size>(10, m_win.GetDimensions()) );

    meshSys.SetCamera(m_cameraEnt);
    dtrSys.SetCamera(m_cameraEnt);

    // -----------------------------------------------------------------------
    // Initialize all the system

    dtrSys.Initialize();

    camSys.Initialize();
    PROFILE_START();
    matSys.Initialize();
    PROFILE_END("Material System Init");

    PROFILE_START();
    meshSys.Initialize();
    PROFILE_END("Mesh System Init");

    PROFILE_START();
    physicsSys.Initialize();
    PROFILE_END("Physics System Init");

    TLOC_LOG_CORE_DEBUG() << "[P] to pause simulation";
    TLOC_LOG_CORE_DEBUG() << "[Q] to quit simulation";

    const tl_int physicsDt = 10; //ms

    core_conts::tl_array_fixed<f64, 10>::type m_frameTimeBuff(10, 0);
    m_renderFrameTime.Reset();
    m_physFrameTime.Reset();
    m_frameTimer.Reset();
    m_accumulator = 0;
    while (m_win.IsValid() && m_keyPresses.IsMarked(key_exit) == false &&
           m_endGame == false)
    {
      // Update our HIDs
      if (m_win.IsValid())
      { m_inputMgr->Update(); }

      // Frame timer
      m_frameTimer.Reset();

      // Without this, the window would appear to be frozen
      gfx_win::WindowEvent evt;
      while (m_win.GetEvent(evt))
      { }

      tl_int currPhysFrameTime = m_physFrameTime.ElapsedMilliSeconds();
      if (currPhysFrameTime >= physicsDt)
      {
        m_accumulator += m_physFrameTime.ElapsedMilliSeconds();
        m_physFrameTime.Reset();

        if (m_accumulator > physicsDt * 3)
        { m_accumulator = physicsDt * 3; }

        while (m_accumulator > physicsDt)
        {
          if (m_keyPresses.IsMarked(key_pause) == false)
          { m_physicsMgr.Update((tl_float) physicsDt * 0.001f); }

          m_accumulator -= physicsDt;
        }
      }

      // Window may have closed by this time
      if (m_win.IsValid() && m_renderFrameTime.ElapsedMilliSeconds() > 16)
      {
        m_renderFrameTime.Reset();

        auto profile = m_profileTimer.ElapsedSeconds() > 1.0f;

        if (profile) { PROFILE_START(); }
        {
          if (m_keyPresses.IsMarked(key_pause) == false)
          { physicsSys.ProcessActiveEntities(); }
        }
        if (profile) { PROFILE_END("Physics System Process"); }

        // Since all systems use one renderer, we need to do this only once
        m_renderer->ApplyRenderSettings();
        m_entityMgr->Update();
        camSys.ProcessActiveEntities();
        matSys.ProcessActiveEntities();

        if (profile) { PROFILE_START(); }
        {
          meshSys.ProcessActiveEntities();
        }
        if (profile) { PROFILE_END("Mesh System Process"); }

        m_renderer->Render();

        dtrSys.ProcessActiveEntities();

        m_win.SwapBuffers();

        if (profile)
        { m_profileTimer.Reset(); }

        // Update FPS
        f64 fps = 0;
        for (tl_int i = 0; i < (tl_int)m_frameTimeBuff.size() - 1; ++i)
        {
          fps += m_frameTimeBuff[i];
        }
        fps /= (m_frameTimeBuff.size() - 1);
        fps = 0.7 * m_frameTimeBuff.back() + fps * 0.3;

        m_win.SetTitle(core_str::Format("Draw Calls: %d, FPS: %.2f", 
          m_renderer->GetNumDrawCalls(), fps));
      }

      m_frameTimeBuff.pop_back();
      m_frameTimeBuff.insert(m_frameTimeBuff.begin(), 1.0f / m_frameTimer.ElapsedSeconds());
    }

    // -----------------------------------------------------------------------
    // Cleanup

    crateMat.reset();
    henryMat.reset();
    TLOC_LOG_CORE_INFO() << "Exitting normally";
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyPress(const tl_size, const input::hid::KeyboardEvent& a_event)
  {
    using namespace input;

    if (a_event.m_keyCode == input::hid::KeyboardEvent::p)
    {
      m_keyPresses.Toggle(key_pause);
    }
    if (a_event.m_keyCode == input::hid::KeyboardEvent::q)
    {
      m_keyPresses.Toggle(key_exit);
    }
    if (a_event.m_keyCode == input::hid::KeyboardEvent::c)
    {
      m_keyPresses.Toggle(key_cameraPersp);

      if (m_keyPresses.IsMarked(key_cameraPersp) == false)
      {
        using math_t::Rectf_c;
        Rectf_c fRect =
          Rectf_c(Rectf_c::width(core_utils::CastNumber<tl_float>(m_win.GetDimensions()[0])),
                  Rectf_c::height(core_utils::CastNumber<tl_float>(m_win.GetDimensions()[1])) );

        using math_proj::FrustumOrtho;
        FrustumOrtho frOrtho = FrustumOrtho(fRect, 0.1f, 10.0f);
        frOrtho.BuildFrustum();

        m_cameraEnt->GetComponent<math_cs::Transform>()->
          SetPosition(math_t::Vec3f32(0, 0, 1.0f));
        m_cameraEnt->GetComponent<gfx_cs::Camera>()->SetFrustum(frOrtho);
      }
      else
      {
        m_cameraEnt->GetComponent<math_cs::Transform>()->
          SetPosition(math_t::Vec3f32(0, 0, 30.0f));

        math_t::AspectRatio ar(math_t::AspectRatio::width( (tl_float)m_win.GetWidth()),
                               math_t::AspectRatio::height( (tl_float)m_win.GetHeight()) );
        math_t::FOV fov(math_t::Degree(60.0f), ar, math_t::p_FOV::vertical());

        math_proj::FrustumPersp::Params params(fov);
        params.SetFar(100.0f).SetNear(1.0f);

        math_proj::FrustumPersp fr(params);
        fr.BuildFrustum();

        m_cameraEnt->GetComponent<gfx_cs::Camera>()->SetFrustum(fr);
      }
    }

    return core_dispatch::f_event::Continue();
  }

  core_dispatch::Event 
    OnKeyRelease(const tl_size, const input::hid::KeyboardEvent&)
  {
    return core_dispatch::f_event::Continue();
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnWindowEvent(const event_type& a_event)
  {
    using namespace gfx_win;

    if (a_event.m_type == WindowEvent::close)
    {
      m_endGame = true;
    }

    return core_dispatch::f_event::Continue();
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  gfx_win::Window                   m_win;

  bool                              m_endGame;
  gfx_rend::renderer_sptr           m_renderer;
  core_time::Timer32                m_timer;
  core_time::Timer32                m_frameTimer;
  core_time::Timer32                m_renderFrameTime;
  core_time::Timer32                m_physFrameTime;
  core_time::Timer32                m_profileTimer;

  tl_int                            m_accumulator;

  input::input_mgr_b_ptr            m_inputMgr;
  input::hid::keyboard_b_vptr       m_keyboard;
  core::utils::Checkpoints          m_keyPresses;

  core_cs::component_pool_mgr_vso   m_compPoolMgr;
  core_cs::event_manager_vso        m_eventMgr;
  core_cs::entity_manager_vso       m_entityMgr;

  ent_ptr                           m_cameraEnt;

  phys_mgr_type                     m_physicsMgr;
};
TLOC_DEF_TYPE(glProgram);

int TLOC_MAIN(int, char* [])
{
  core_mem::tracking::DoDisableTracking();

  glProgram p;
  p.Initialize();
  p.RunGame();

  return 0;
}
