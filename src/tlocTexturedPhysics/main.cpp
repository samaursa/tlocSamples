#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>
#include <tlocCore/memory/tlocLinkMe.cpp>

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

#include <samplesAssetsPath.h>

using namespace tloc;

#define PROFILE_START()\
    m_timer.Reset()

#pragma warning(disable:4127)
#define PROFILE_END(_text_)\
  do{\
    s32 timeInMs = core_utils::CastNumber<s32>(m_timer.ElapsedMicroSeconds());\
    printf("\n"#_text_": %u us", timeInMs);\
  }while(0)

enum
{
  key_pause = 0,
  key_exit,
  key_cameraPersp,
  key_count
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
    : m_keyPresses(key_count)
    , m_entityMgr(m_eventMgr.get())
    , m_quadSys(m_eventMgr.get(), m_entityMgr.get())
    , m_endGame(false)

  { m_win.Register(this); }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  void Initialize()
  {
    // trying to match iPad retina display ratio (not resolution)
    m_win.Create(graphics_mode::Properties(1024, 768),
                 gfx_win::WindowSettings("Atom & Eve"));

    ParamList<core_t::Any> params;
    params.m_param1 = m_win.GetWindowHandle();
    m_inputMgr = input::input_mgr_b_ptr(new input::InputManagerB(params));

    m_keyboard = m_inputMgr->CreateHID<input::hid::KeyboardB>();
    m_keyboard->Register(this);

    //------------------------------------------------------------------------
    // Initialize graphics platform
    if (gfx_gl::InitializePlatform() != ErrorSuccess)
    { TLOC_ASSERT(false, "\nGraphics platform failed to initialize"); exit(0); }

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

    using math::types::Rectf32;
    using math::types::Circlef32;

    using gfx_cs::CameraSystem;
    using gfx_cs::QuadRenderSystem;
    using gfx_cs::FanRenderSystem;
    using gfx_cs::MaterialSystem;
    using phys_cs::RigidBodySystem;

    using core_str::String;

    //------------------------------------------------------------------------
    // Systems and Entity Preparation
    core_conts::Array<math_t::Vec4f32> g_vertex_color_data_1 = GetQuadColor();

    QuadRenderSystem  quadSys   (m_eventMgr.get(), m_entityMgr.get());
    FanRenderSystem   fanSys    (m_eventMgr.get(), m_entityMgr.get());
    CameraSystem      camSys    (m_eventMgr.get(), m_entityMgr.get());
    MaterialSystem    matSys    (m_eventMgr.get(), m_entityMgr.get());
    RigidBodySystem   physicsSys(m_eventMgr.get(), m_entityMgr.get(),
                                 &m_physicsMgr.GetWorld());

    // attach the default renderer to both rendering systems
    quadSys.SetRenderer(m_renderer);
    fanSys.SetRenderer(m_renderer);

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

    //------------------------------------------------------------------------
    // Create the uniforms holding the texture objects

    gl::uniform_vso  u_crateTo;
    {
      gfx_gl::texture_object_sptr crateTo(new gfx_gl::TextureObject());
      {
        gfx_med::ImageLoaderPng png;
        {
          core_str::String filePath(GetAssetsPath());
          filePath += "/images/crate.png";
          core_io::Path path(filePath.c_str());
          if (png.Load(path) != ErrorSuccess)
          { TLOC_ASSERT(false, "Image did not load"); }
        }
        crateTo->Initialize(png.GetImage());
        crateTo->Activate();
      }
      u_crateTo->SetName("shaderTexture").SetValueAs(*crateTo);
    }

    gl::uniform_vso  u_henryTo;
    {
      gfx_gl::texture_object_sptr to(new gfx_gl::TextureObject());
      {
        gfx_med::ImageLoaderPng png;
        {
          core_str::String filePath(GetAssetsPath());
          filePath += "/images/henry.png";
          core_io::Path path(filePath.c_str());

          if (png.Load(path) != ErrorSuccess)
          { TLOC_ASSERT(false, "Image did not load"); }
        }
        to->Initialize(png.GetImage());
        to->Activate();
      }
      u_henryTo->SetName("shaderTexture").SetValueAs(*to);
    }

    gfx_cs::material_vptr crateMat =
      prefab_gfx::Material(m_entityMgr.get(), m_compPoolMgr.get())
      .AddUniform(u_crateTo.get())
      .Create(core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS))
              ->GetComponent<gfx_cs::Material>();

    gfx_cs::material_vptr henryMat =
      prefab_gfx::Material(m_entityMgr.get(), m_compPoolMgr.get())
      .AddUniform(u_henryTo.get())
      .Create(core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS))
              ->GetComponent<gfx_cs::Material>();

    PROFILE_START();
    const tl_int repeat = 150;
    for (tl_int i = repeat + 1; i > 0; --i)
    {
      tl_float posX = rng::g_defaultRNG.GetRandomFloat(-5.0f, 5.0f);
      tl_float posY = rng::g_defaultRNG.GetRandomFloat(20.0f, 480.0f);

      if (rng::g_defaultRNG.GetRandomInteger(0, 2) == 1)
      {
        // Create a quad ent
        Rectf32 rect(Rectf32::width(3.0f), Rectf32::height(3.0f));
        ent_ptr quadEnt =
          prefab_gfx::Quad(m_entityMgr.get(), m_compPoolMgr.get())
          .Dimensions(rect).Create();

        box2d::rigid_body_def_sptr rbDef(new box2d::RigidBodyDef());
        rbDef->SetPosition(box2d::RigidBodyDef::vec_type(posX, posY));
        rbDef->SetType<box2d::p_rigid_body::DynamicBody>();

        prefab_phys::RigidBody(m_entityMgr.get(), m_compPoolMgr.get())
          .Add(quadEnt, rbDef);
        prefab_phys::RigidBodyShape(m_entityMgr.get(), m_compPoolMgr.get())
          .Add(quadEnt, rect, prefab_phys::RigidBodyShape::density(1.0f));

        m_entityMgr->InsertComponent(quadEnt, crateMat);
      }
      else
      {
        // Create a fan ent
        Circlef32 circle( Circlef32::radius(1.5f) );
        ent_ptr fanEnt = prefab_gfx::Fan(m_entityMgr.get(), m_compPoolMgr.get())
          .Sides(8)
          .Circle(circle)
          .Create();

        box2d::rigid_body_def_sptr rbDef(new box2d::RigidBodyDef());
        rbDef->SetPosition(box2d::RigidBodyDef::vec_type(posX, posY));
        rbDef->SetType<box2d::p_rigid_body::DynamicBody>();
        prefab_phys::RigidBody(m_entityMgr.get(), m_compPoolMgr.get())
          .Add(fanEnt, rbDef);

        box2d::RigidBodyShapeDef rbShape(circle);
        rbShape.SetRestitution(1.0f);
        prefab_phys::RigidBodyShape(m_entityMgr.get(), m_compPoolMgr.get())
          .Add(fanEnt, rbShape);

        m_entityMgr->InsertComponent(fanEnt, henryMat);
      }
    }
    PROFILE_END("Generating Quads and Fans");

    {
      // Create a fan ent
      Circlef32 circle( Circlef32::radius(5.0f) );
      ent_ptr fanEnt = prefab_gfx::Fan(m_entityMgr.get(), m_compPoolMgr.get())
        .Sides(12).Circle(circle).Create();

      box2d::rigid_body_def_sptr rbDef(new box2d::RigidBodyDef());
      rbDef->SetType<box2d::p_rigid_body::StaticBody>();
      rbDef->SetPosition(box2d::RigidBodyDef::vec_type(0.0f, -10.f));
      prefab_phys::RigidBody(m_entityMgr.get(), m_compPoolMgr.get())
        .Add(fanEnt, rbDef);

      box2d::RigidBodyShapeDef rbCircleShape(circle);
      prefab_phys::RigidBodyShape(m_entityMgr.get(), m_compPoolMgr.get())
        .Add(fanEnt, rbCircleShape);

        m_entityMgr->InsertComponent(fanEnt, henryMat);
    }

    tl_float winWidth = (tl_float)m_win.GetWidth();
    tl_float winHeight = (tl_float)m_win.GetHeight();

    // For some reason, if we remove the brackets, C++ assumes the following is
    // a function declaration. Generally, something like math_t::Rectf proj();
    // might be considered a function declaration but not when we give
    // arguments
    //
    // TODO: Look into this problem and find a way to remove the extra
    // brackets.
    math_t::Rectf fRect( (math_t::Rectf::width(winWidth / 10.0f)),
                         (math_t::Rectf::height(winHeight / 10.0f)) );

    m_ortho = math_proj::FrustumOrtho (fRect, 0.1f, 100.0f);
    m_ortho.BuildFrustum();

    m_cameraEnt = prefab_gfx::Camera(m_entityMgr.get(), m_compPoolMgr.get())
      .Create(m_ortho, math_t::Vec3f(0, 0, 1.0f));

    quadSys.SetCamera(m_cameraEnt);
    fanSys.SetCamera(m_cameraEnt);

    camSys.Initialize();

    PROFILE_START();
    matSys.Initialize();
    PROFILE_END("Material System Init");

    PROFILE_START();
    quadSys.Initialize();
    PROFILE_END("Quad System Init");

    PROFILE_START();
    fanSys.Initialize();
    PROFILE_END("Fan System Init");

    PROFILE_START();
    physicsSys.Initialize();
    PROFILE_END("Physics System Init");

    printf("\n\n[P] to pause simulation");
    printf("\n[Q] to quit simulation");

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
          { m_physicsMgr.Update((tl_float)physicsDt * 0.001f); }
          m_accumulator -= physicsDt;
        }
      }

      // Window may have closed by this time
      if (m_win.IsValid() && m_renderFrameTime.ElapsedMilliSeconds() > 16)
      {
        m_renderFrameTime.Reset();

        // Since all systems use one renderer, we need to do this only once
        m_renderer->ApplyRenderSettings();

        camSys.ProcessActiveEntities();
        physicsSys.ProcessActiveEntities();
        matSys.ProcessActiveEntities();
        quadSys.ProcessActiveEntities();
        fanSys.ProcessActiveEntities();

        m_win.SwapBuffers();

        // Update FPS
        f64 fps = 0;
        for (tl_int i = 0; i < (tl_int)m_frameTimeBuff.size() - 1; ++i)
        {
          fps += m_frameTimeBuff[i];
        }
        fps /= (m_frameTimeBuff.size() - 1);
        fps = 0.7 * m_frameTimeBuff.back() + fps * 0.3;

        char buff[20];
        sprintf(buff, "%f", fps);
        m_win.SetTitle(buff);
      }

      m_frameTimeBuff.pop_back();
      m_frameTimeBuff.insert(m_frameTimeBuff.begin(), 1.0f / m_frameTimer.ElapsedSeconds());
    }

    // -----------------------------------------------------------------------
    // Cleanup

    crateMat.reset();
    henryMat.reset();
    printf("\nExitting normally\n");
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnKeyPress(const tl_size, const input::hid::KeyboardEvent& a_event)
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
        m_cameraEnt->GetComponent<math_cs::Transform>()->
          SetPosition(math_t::Vec3f32(0, 0, 1.0f));
        m_cameraEnt->GetComponent<gfx_cs::Camera>()->
          SetFrustum(m_ortho);
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

    return false;
  }

  bool OnKeyRelease(const tl_size, const input::hid::KeyboardEvent&)
  {
    return false;
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  void OnWindowEvent(const event_type& a_event)
  {
    using namespace gfx_win;

    if (a_event.m_type == WindowEvent::close)
    {
      m_endGame = true;
    }
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  gfx_win::Window                   m_win;

  bool                              m_endGame;
  gfx_rend::renderer_sptr           m_renderer;
  core_time::Timer32                m_timer;
  core_time::Timer32                m_frameTimer;
  core_time::Timer32                m_renderFrameTime;
  core_time::Timer32                m_physFrameTime;

  math_proj::FrustumOrtho           m_ortho;

  tl_int                            m_accumulator;

  input::input_mgr_b_ptr            m_inputMgr;
  input::hid::KeyboardB*            m_keyboard;
  core::utils::Checkpoints          m_keyPresses;

  core_cs::component_pool_mgr_vso   m_compPoolMgr;
  core_cs::event_manager_vso        m_eventMgr;
  core_cs::entity_manager_vso       m_entityMgr;

  ent_ptr                           m_cameraEnt;

  phys_mgr_type                     m_physicsMgr;
  gfx_cs::QuadRenderSystem          m_quadSys;
};
TLOC_DEF_TYPE(glProgram);

int TLOC_MAIN(int, char* [])
{
  glProgram p;
  p.Initialize();
  p.RunGame();

  return 0;
}
