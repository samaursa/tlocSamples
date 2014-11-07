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
    s32 timeInMs = core_utils::CastNumber<s32>(m_timer.ElapsedMicroSeconds());\
    TLOC_LOG_CORE_INFO() << core_str::Format(_text_": %u us", timeInMs);\
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
  typedef gfx_cs::Quad                      quad_type;
  typedef gfx_cs::Material                  mat_type;
  typedef core_cs::Entity                   ent_type;
  typedef core_cs::entity_vptr              ent_ptr;
  typedef core_cs::const_entity_vptr        const_ent_ptr;

  typedef phys_box2d::PhysicsManager        phys_mgr_type;

  typedef gfx_win::WindowEvent              event_type;
  typedef gfx_win::Window::graphics_mode    graphics_mode;

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  glProgram()
    : m_mouseVisible(true)
    , m_endGame(false)
    , m_keyPresses(key_count)
    , m_entityMgr( MakeArgs(m_eventMgr.get()) )
  
  { m_win.Register(this); }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  void Initialize()
  {
    // trying to match iPad retina display ratio (not resolution)
    m_win.Create(graphics_mode::Properties(1024, 768),
                 gfx_win::WindowSettings("Raypicking 2D"));

    ParamList<core_t::Any> params;
    params.m_param1 = m_win.GetWindowHandle();
    m_inputMgr = core_sptr::MakeShared<input::InputManagerB>(params);
    m_inputMgrImm = core_sptr::MakeShared<input::InputManagerI>(params);

    m_keyboard = m_inputMgr->CreateHID<input::hid::KeyboardB>();
    m_keyboard->Register(this);

    m_mouse = m_inputMgr->CreateHID<input::hid::MouseB>();
    m_mouse->Register(this);

    m_touchSurface = m_inputMgrImm->CreateHID<input::hid::TouchSurfaceI>();

    // -----------------------------------------------------------------------
    // Initialize graphics platform
    if (gfx_gl::InitializePlatform() != ErrorSuccess)
    {
      TLOC_ASSERT_FALSE("Graphics platform failed to initialize");
      exit(0);
    }

    // -----------------------------------------------------------------------
    // Get the default renderer

    using namespace gfx_rend::p_renderer;
    m_renderer = m_win.GetRenderer();
    {
      gfx_rend::Renderer::Params p(m_renderer->GetParams());
      p.AddClearBit<clear::ColorBufferBit>();
      m_renderer->SetParams(p);
    }
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

  void MoveMouseAndCheckCollision(f32 absX, f32 absY)
  {
    using math_utils::scale_f32_f32;
    typedef math_utils::scale_f32_f32::range_small range_small;
    typedef math_utils::scale_f32_f32::range_large range_large;
    using namespace core::component_system;

    range_small smallR(-1.0f, 1.1f);
    range_large largeRX(0.0f, (f32)m_win.GetWidth());
    range_large largeRY(0.0f, (f32)m_win.GetHeight());
    scale_f32_f32 scx(smallR, largeRX);
    scale_f32_f32 scy(smallR, largeRY);

    math_t::Vec3f32 xyz(scx.ScaleDown((f32)(absX) ),
                        scy.ScaleDown((f32)(m_win.GetHeight() -
                                            absY - 1 )),
                        -1.0f);

    math_t::Ray3f ray =
      m_cameraEnt->GetComponent<gfx_cs::Camera>()->GetFrustumRef().GetRay(xyz);

    TLOC_LOG_DEFAULT_DEBUG() << "Ray: " 
      << ray.GetOrigin()[0] << ", " 
      << ray.GetOrigin()[1] << ", " 
      << ray.GetOrigin()[2];

    // Transform with inverse of camera
    math_cs::Transformf32 camTrans =
      *m_cameraEnt->GetComponent<math_cs::Transformf32>();
    math_t::Mat4f32 camTransMatInv = camTrans.GetTransformation();

    ray = ray * camTransMatInv;

    // Set the mouse pointer
    m_mouseFan->GetComponent<math_cs::Transform>()->
      SetPosition(math_t::Vec3f32(ray.GetOrigin()[0],
                                  ray.GetOrigin()[1], 0.0f) );

    // Now start transform the ray from world to fan entity co-ordinates for
    // the intersection test

    ray = ray * m_fanEnt->GetComponent<math_cs::Transform>()->Invert().GetTransformation();

    math_t::Vec2f rayPos2f = ray.GetOrigin().ConvertTo<math_t::Vec2f32>();
    math_t::Vec2f rayDir2f = ray.GetDirection().ConvertTo<math_t::Vec2f32>();

    math_t::Ray2f ray2 = math_t::Ray2f(math_t::Ray2f::origin(rayPos2f),
                         math_t::Ray2f::direction(rayDir2f) );

    static tl_int intersectionCounter = 0;
    static tl_int nonIntersectionCounter = 0;

    if (m_fanEnt->GetComponent<gfx_cs::Fan>()->GetEllipseRef().Intersects(ray2))
    {
      nonIntersectionCounter = 0;
      ++intersectionCounter;
      if (intersectionCounter == 1)
      {
        gfx_cs::material_sptr mat = m_fanEnt->GetComponent<gfx_cs::Material>();

        if (mat != m_crateMat)
        { *mat = *m_crateMat;}

        TLOC_LOG_CORE_INFO() << "Intersecting with circle!";
      }
    }
    else
    {
      intersectionCounter = 0;
      ++nonIntersectionCounter;

      if (nonIntersectionCounter == 1)
      {
        gfx_cs::material_sptr mat = m_fanEnt->GetComponent<gfx_cs::Material>();

        if (mat != m_henryMat)
        { *mat = *m_henryMat; }

        TLOC_LOG_CORE_INFO() << "NOT intersecting with circle!";
      }
    }
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  void AddShaders(gfx_cs::material_vptr a_mat)
  {
    {
#if defined (TLOC_OS_WIN)
      core_str::String shaderPath("/shaders/mvpTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
      core_str::String shaderPath("/shaders/mvpTextureVS_gl_es_2_0.glsl");
#endif
      shaderPath = GetAssetsPath() + shaderPath;
      core_io::FileIO_ReadA file( (core_io::Path(shaderPath)) );

      file.Open();
      core_str::String code;
      file.GetContents(code);
      a_mat->SetVertexSource(code);
    }

    {
#if defined (TLOC_OS_WIN)
      core_str::String shaderPath("/shaders/mvpTextureFS.glsl");
#elif defined (TLOC_OS_IPHONE)
      core_str::String shaderPath("/shaders/mvpTextureFS_gl_es_2_0.glsl");
#endif
      shaderPath = GetAssetsPath() + shaderPath;
      core_io::FileIO_ReadA file( (core_io::Path(shaderPath)) );
      file.Open();

      core_str::String code;
      file.GetContents(code);
      a_mat->SetFragmentSource(code);
    }
  }

  void RunGame()
  {
    using namespace core;
    using namespace math;
    using namespace graphics;
    using namespace physics;

    using math::types::Rectf32_c;
    using math::types::Circlef32;

    using gfx_cs::CameraSystem;
    using gfx_cs::QuadRenderSystem;
    using gfx_cs::FanRenderSystem;
    using gfx_cs::MaterialSystem;
    using phys_cs::RigidBodySystem;

    using core::component_system::ComponentPoolManager;
    using core_str::String;

    //------------------------------------------------------------------------
    // Systems and Entity Preparation
    FanRenderSystem   fanSys(m_eventMgr.get(), m_entityMgr.get());
    fanSys.SetRenderer(m_renderer);
    CameraSystem      camSys(m_eventMgr.get(), m_entityMgr.get());
    MaterialSystem    matSys(m_eventMgr.get(), m_entityMgr.get());

    m_henryMat = core_sptr::MakeShared<gfx_cs::Material>();
    m_crateMat = core_sptr::MakeShared<gfx_cs::Material>();

    m_texObjHenry = core_sptr::MakeShared<gfx_gl::TextureObject>();
    m_texObjCrate = core_sptr::MakeShared<gfx_gl::TextureObject>();

    AddShaders(core_sptr::ToVirtualPtr(m_henryMat));
    AddShaders(core_sptr::ToVirtualPtr(m_crateMat));

    //------------------------------------------------------------------------
    // Add the shader operators
    {
      gfx_med::ImageLoaderPng image;
      core_str::String filePath(GetAssetsPath());
      filePath += "/images/henry.png";
      core_io::Path path(filePath.c_str());

      if (image.Load(path) != ErrorSuccess)
      { TLOC_ASSERT_FALSE("Image did not load"); }

      m_texObjHenry->Initialize(image.GetImage());

      gfx_gl::uniform_vso uniform;
      uniform->SetName("shaderTexture").SetValueAs(*m_texObjHenry);

      m_henryMat->AddUniform(*uniform);
    }

    {
      gfx_med::ImageLoaderPng image;
      core_str::String filePath(GetAssetsPath());
      filePath += "/images/crate.png";
      core_io::Path path(filePath.c_str());

      if (image.Load(path) != ErrorSuccess)
      { TLOC_ASSERT_FALSE("Image did not load"); }

      m_texObjCrate->Initialize(image.GetImage());

      gfx_gl::uniform_vso uniform;
      uniform->SetName("shaderTexture").SetValueAs(*m_texObjCrate);

      m_crateMat->AddUniform(*uniform);
    }

    // Create internal materials
    gfx_cs::material_pool_vptr matPool =
      m_compPoolMgr->CreateNewPool<gfx_cs::Material>();

    {
      // Create a fan ent
      Circlef32 circle( Circlef32::radius(5.0f) );
      m_fanEnt = pref_gfx::Fan(m_entityMgr.get(), m_compPoolMgr.get()).
        Sides(12).Circle(circle).Create();

      tl_float posX = rng::g_defaultRNG.GetRandomFloat(-10.0f, 10.0f);
      tl_float posY = rng::g_defaultRNG.GetRandomFloat(-10.0f, 10.0f);

      m_fanEnt->GetComponent<math_cs::Transform>()->
        SetPosition(math_t::Vec3f(posX, posY, 0));

      gfx_cs::material_pool::iterator matPoolItr = matPool->GetNext();
      (*matPoolItr)->SetValue(core_sptr::MakeShared<gfx_cs::Material>(*m_henryMat));

      m_entityMgr->InsertComponent(core_cs::EntityManager::Params
                                   (m_fanEnt, *(*matPoolItr)->GetValuePtr()) );
      m_entityMgr->InsertComponent(core_cs::EntityManager::Params
                                   (m_fanEnt, m_henryMat) );
    }

    {
      // Create a fan ent
      Circlef32 circle( Circlef32::radius(0.5f) );
      m_mouseFan = pref_gfx::Fan(m_entityMgr.get(), m_compPoolMgr.get())
        .Sides(12).Circle(circle).Create();

      gfx_cs::material_pool::iterator matPoolItr = matPool->GetNext();
      (*matPoolItr)->SetValue(core_sptr::MakeShared<gfx_cs::Material>(*m_crateMat));

      m_entityMgr->InsertComponent(core_cs::EntityManager::Params
                                   (m_mouseFan, *(*matPoolItr)->GetValuePtr()) );
      m_entityMgr->InsertComponent(core_cs::EntityManager::Params
                                   (m_mouseFan, m_crateMat) );
    }

    // randomize camera position to ensure it's taken into account when 
    // performing ray intersections
    tl_float posX = rng::g_defaultRNG.GetRandomFloat(-10.0f, 10.0f);
    tl_float posY = rng::g_defaultRNG.GetRandomFloat(-10.0f, 10.0f);

    m_cameraEnt = pref_gfx::Camera(m_entityMgr.get(), m_compPoolMgr.get())
      .Near(0.1f)
      .Far(100.0f)
      .Perspective(false)
      .Position(math_t::Vec3f(posX, posY, 1.0f))
      .Create( core_ds::Divide<tl_size>(10, m_win.GetDimensions()) );

    fanSys.SetCamera(m_cameraEnt);

    matSys.Initialize();
    fanSys.Initialize();
    camSys.Initialize();

    TLOC_LOG_CORE_DEBUG() << "V - toggle mouse cursor visibility";

    while (m_win.IsValid() && m_keyPresses.IsMarked(key_exit) == false &&
           m_endGame == false)
    {
      // Update our HIDs
      if (m_win.IsValid())
      { m_inputMgr->Update(); }

      // Without this, the window would appear to be frozen
      gfx_win::WindowEvent evt;
      while (m_win.GetEvent(evt))
      { }

      // Handle the touches
      m_inputMgrImm->Update();
      m_currentTouches = m_touchSurface->GetCurrentTouches();
      if (m_currentTouches.size() != 0)
      {
        f32 xAbs = (f32)m_currentTouches[0].m_X.m_abs();
        f32 yAbs = (f32)m_currentTouches[0].m_Y.m_abs();

        MoveMouseAndCheckCollision(xAbs, yAbs);
      }
      m_inputMgrImm->Reset();

      // Window may have closed by this time
      if (m_win.IsValid())
      {
        m_renderFrameTime.Reset();

        camSys.ProcessActiveEntities();
        matSys.ProcessActiveEntities();

        m_renderer->ApplyRenderSettings();
        fanSys.ProcessActiveEntities();

        m_win.SwapBuffers();
      }
    }

    TLOC_LOG_CORE_INFO() << "Exiting normally";
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
    else if (a_event.m_keyCode == input::hid::KeyboardEvent::q)
    {
      m_keyPresses.Toggle(key_exit);
    }
    else if (a_event.m_keyCode == input::hid::KeyboardEvent::c)
    {
      m_keyPresses.Toggle(key_cameraPersp);

      if (m_keyPresses.IsMarked(key_cameraPersp) == false)
      {
        math_t::Rectf_c fRect(
          (math_t::Rectf_c::width((tl_float)m_win.GetWidth() / 10.0f)),
          (math_t::Rectf_c::height((tl_float)m_win.GetHeight() / 10.0f)) );

        math_proj::frustum_ortho_f32 fr =
          math_proj::FrustumOrtho (fRect, 0.1f, 100.0f);
        fr.BuildFrustum();

        m_cameraEnt->GetComponent<math_cs::Transform>()->
          SetPosition(math_t::Vec3f32(0, 0, 1.0f));
        m_cameraEnt->GetComponent<gfx_cs::Camera>()->
          SetFrustum(fr);
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
    else if (a_event.m_keyCode == input::hid::KeyboardEvent::v)
    {
      m_mouseVisible = !m_mouseVisible;
      m_win.SetMouseVisibility(m_mouseVisible);
    }

    return core_dispatch::f_event::Continue();
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyRelease(const tl_size, const input::hid::KeyboardEvent&)
  {
    return core_dispatch::f_event::Continue();
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnMouseButtonPress(const tl_size , 
                       const input_hid::MouseEvent& , 
                       const input_hid::MouseEvent::button_code_type)
  {
    return core_dispatch::f_event::Continue();
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnMouseButtonRelease(const tl_size , 
                         const input_hid::MouseEvent& , 
                         const input_hid::MouseEvent::button_code_type)
  {
    return core_dispatch::f_event::Continue();
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnMouseMove(const tl_size , const input_hid::MouseEvent& a_event)
  {
    MoveMouseAndCheckCollision((f32)a_event.m_X.m_abs().Get(),
                               (f32)a_event.m_Y.m_abs().Get());

    return core_dispatch::f_event::Continue();
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnWindowEvent(const event_type& a_event)
  {
    using namespace gfx_win;

    if (a_event.m_type == WindowEvent::close)
    { m_endGame = true; }

    return core_dispatch::f_event::Continue();
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  // window should be destroyed last ALWAYS, because when it is closed, its
  // OpenGL context is destroyed so we make sure that it is the first one
  // constructed
  gfx_win::Window         m_win;

  gfx_rend::renderer_sptr     m_renderer;
  core_time::Timer32          m_timer;
  core_time::Timer32          m_frameTimer;
  core_time::Timer32          m_renderFrameTime;
  ent_ptr                     m_cameraEnt;
  bool                        m_mouseVisible;
  bool                        m_endGame;

  ent_ptr                     m_fanEnt;
  ent_ptr                     m_mouseFan;

  gfx_gl::texture_object_sptr m_texObjHenry;
  gfx_gl::texture_object_sptr m_texObjCrate;
  gfx_cs::material_sptr       m_henryMat;
  gfx_cs::material_sptr       m_crateMat;

  input::input_mgr_b_ptr      m_inputMgr;
  input::input_mgr_i_ptr      m_inputMgrImm;

  input::hid::keyboard_b_vptr       m_keyboard;
  input::hid::mouse_b_vptr          m_mouse;
  input::hid::touch_surface_i_vptr  m_touchSurface;

  input_hid::TouchSurfaceI::touch_container_type  m_currentTouches;
  core::utils::Checkpoints                        m_keyPresses;

  core_cs::component_pool_mgr_vso m_compPoolMgr;
  core_cs::event_manager_vso      m_eventMgr;
  core_cs::entity_manager_vso     m_entityMgr;
};
TLOC_DEF_TYPE(glProgram);

int TLOC_MAIN(int, char* [])
{
  glProgram p;
  p.Initialize();
  p.RunGame();

  return 0;
}
