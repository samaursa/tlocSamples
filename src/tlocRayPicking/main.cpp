#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl>
#include <tlocCore/memory/tlocLinkMe.cpp>

#include <tlocGraphics/tloc_graphics.h>
#include <tlocGraphics/tloc_graphics.inl>

#include <tlocMath/tloc_math.h>
#include <tlocMath/tloc_math.inl>

#include <tlocPhysics/tloc_physics.h>
#include <tlocPhysics/tloc_physics.inl>

#include <tlocInput/tloc_input.h>
#include <tlocInput/tloc_input.inl>

#include <tlocPrefab/tloc_prefab.h>
#include <tlocPrefab/tloc_prefab.inl>

#include <samplesAssetsPath.h>

using namespace tloc;

#define PROFILE_START()\
    m_timer.Reset()

#pragma warning(disable:4127)
#define PROFILE_END(_text_)\
  do{\
    s32 timeInMs = core_utils::CastNumber<s32>(m_timer.ElapsedMicroSeconds());\
    printf("\n"_text_": %u us", timeInMs);\
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
  typedef gfx_cs::Quad      quad_type;
  typedef gfx_cs::Material  mat_type;
  typedef core_cs::Entity   ent_type;

  typedef phys_box2d::PhysicsManager        phys_mgr_type;

  typedef gfx_win::WindowEvent              event_type;
  typedef gfx_win::Window::graphics_mode    graphics_mode;

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  glProgram()
    : m_keyPresses(key_count)
    , m_eventMgr(new core_cs::EventManager())
    , m_entityMgr(new core_cs::EntityManager(m_eventMgr))
    , m_quadSys(m_eventMgr, m_entityMgr)

  { m_win.Register(this); }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  void Initialize()
  {
    // trying to match iPad retina display ratio (not resolution)
    m_win.Create(graphics_mode::Properties(1024, 768),
                 gfx_win::WindowSettings("Atom & Eve"));
    m_win.SetMouseVisibility(true);

    ParamList<core_t::Any> params;
    params.m_param1 = m_win.GetWindowHandle();
    m_inputMgr = input::input_mgr_b_ptr(new input::InputManagerB(params));
    m_inputMgrImm = input::input_mgr_i_ptr(new input::InputManagerI(params));

    m_keyboard = m_inputMgr->CreateHID<input::hid::KeyboardB>();
    m_keyboard->Register(this);

    m_mouse = m_inputMgr->CreateHID<input::hid::MouseB>();
    m_mouse->Register(this);

    m_touchSurface = m_inputMgrImm->CreateHID<input::hid::TouchSurfaceI>();

    if (m_renderer.Initialize() != ErrorSuccess)
    {
      TLOC_ASSERT(false, "Renderer failed to initialize");
      exit(0);
    }

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

  void MoveMouseAndCheckCollision(f32 absX, f32 absY)
  {
    using math_utils::scale_f32_f32;
    typedef math_utils::scale_f32_f32::range_small range_small;
    typedef math_utils::scale_f32_f32::range_large range_large;
    using namespace core::component_system;

    //printf("\n%i, %i", a_event.m_X.m_abs(), a_event.m_Y.m_abs());

    range_small smallR(-1.0f, 1.0f);
    range_large largeRX(0.0f, (f32)m_win.GetWidth());
    range_large largeRY(0.0f, (f32)m_win.GetHeight());
    scale_f32_f32 scx(smallR, largeRX);
    scale_f32_f32 scy(smallR, largeRY);

    math_t::Vec3f32 xyz(scx.ScaleDown((f32)(absX) ),
                        scy.ScaleDown((f32)(m_win.GetHeight() -
                                            absY - 1 )),
                        -1.0f);
    math_t::Ray3f ray = m_ortho.GetRay(xyz);

    // Transform with inverse of camera
    math_cs::Transformf32 camTrans =
      *m_cameraEnt->GetComponent<math_cs::Transformf32>();
    math_cs::Transformf32 camTransInv = camTrans.Invert();
    math_t::Mat4f32 camTransMatInv = camTransInv.GetTransformation();

    math_t::Vec3f32 rayPosTrans = ray.GetOrigin();
    rayPosTrans.ConvertFrom<f32, 4>
      (camTransMatInv *
       rayPosTrans.ConvertTo<math_t::Vec4f32, core_ds::p_tuple::overflow_one>());

    ray = math_t::Ray3f32(math_t::Ray3f32::origin(rayPosTrans),
          math_t::Ray3f32::direction(ray.GetDirection()) );

    // Set the mouse pointer
    m_mouseFan->GetComponent<math_cs::Transform>()->
      SetPosition(math_t::Vec3f32(ray.GetOrigin()[0],
                                  ray.GetOrigin()[1], 0.0f) );

    // Now start transform the ray from world to fan entity co-ordinates for
    // the intersection test

    math_t::Vec3f rayPos = ray.GetOrigin();
    math_t::Mat3f rot = m_fanEnt->GetComponent<math_cs::Transform>()->GetOrientation();
    rot.Inverse();

    math_t::Vec3f fanPos = m_fanEnt->GetComponent<math_cs::Transform>()->GetPosition();
    rayPos = rayPos - fanPos;
    rayPos = rot * rayPos;

    math_t::Vec2f rayPos2f = rayPos.ConvertTo<math_t::Vec2f32>();
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
        auto mat = m_fanEnt->GetComponent<gfx_cs::Material>();

        if (mat != m_crateMat.get())
        { *mat = *m_crateMat;}

        printf("\nIntersecting with circle!");
      }
    }
    else
    {
      intersectionCounter = 0;
      ++nonIntersectionCounter;

      if (nonIntersectionCounter == 1)
      {
        auto mat = m_fanEnt->GetComponent<gfx_cs::Material>();

        if (mat != m_henryMat.get())
        { *mat = *m_henryMat; }

        printf("\nNOT intersecting with circle!");
      }
    }
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  void AddShaders(gfx_cs::material_sptr a_mat)
  {
    {
#if defined (TLOC_OS_WIN)
      core_str::String shaderPath("/shaders/mvpTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
      core_str::String shaderPath("/shaders/mvpTextureVS_gl_es_2_0.glsl");
#endif
      shaderPath = GetAssetsPath() + shaderPath;
      core_io::FileIO_ReadA file(shaderPath.c_str());

      if(file.Open() != ErrorSuccess)
      {
        printf("\n%s", shaderPath.c_str());
        printf("\nUnable to open vertex shader");
        TLOC_ASSERT(false, "");
      }

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
      core_io::FileIO_ReadA file(shaderPath.c_str());
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

    using math::types::Rectf32;
    using math::types::Circlef32;

    using gfx_cs::QuadRenderSystem;
    using gfx_cs::FanRenderSystem;
    using gfx_cs::MaterialSystem;
    using phys_cs::RigidBodySystem;

    using core::component_system::ComponentPoolManager;
    using core_str::String;

    //------------------------------------------------------------------------
    // Systems and Entity Preparation
    QuadRenderSystem quadSys(m_eventMgr, m_entityMgr);
    FanRenderSystem fanSys(m_eventMgr, m_entityMgr);
    MaterialSystem matSys(m_eventMgr, m_entityMgr);

    ComponentPoolManager poolMgr;

    m_henryMat.reset(new gfx_cs::Material());
    m_crateMat.reset(new gfx_cs::Material());

    AddShaders(m_henryMat);
    AddShaders(m_crateMat);

    //------------------------------------------------------------------------
    // Add the shader operators
    {
      gfx_med::ImageLoaderPng image;
      core_str::String filePath(GetAssetsPath());
      filePath += "/images/henry.png";
      core_io::Path path(filePath.c_str());

      if (image.Load(path) != ErrorSuccess)
      { TLOC_ASSERT(false, "Image did not load"); }

      m_texObjHenry.reset(new gfx_gl::TextureObject());
      m_texObjHenry->Initialize(image.GetImage());

      gfx_gl::uniform_sptr uniform( (new gl::Uniform()) );
      uniform->SetName("shaderTexture").SetValueAs(m_texObjHenry);

      gl::shader_operator_sptr so =
        gl::shader_operator_sptr(new gl::ShaderOperator());
      so->AddUniform(uniform);

      m_henryMat->AddShaderOperator(so);
    }

    {
      gfx_med::ImageLoaderPng image;
      core_str::String filePath(GetAssetsPath());
      filePath += "/images/crate.png";
      core_io::Path path(filePath.c_str());

      if (image.Load(path) != ErrorSuccess)
      { TLOC_ASSERT(false, "Image did not load"); }

      m_texObjCrate.reset(new gfx_gl::TextureObject());
      m_texObjCrate->Initialize(image.GetImage());

      gfx_gl::uniform_sptr uniform(new gl::Uniform());
      uniform->SetName("shaderTexture").SetValueAs(m_texObjCrate);

      auto so = gl::shader_operator_sptr(new gl::ShaderOperator());
      so->AddUniform(uniform);

      m_crateMat->AddShaderOperator(so);
    }

    // Create internal materials
    auto matPool = poolMgr.CreateNewPool<gfx_cs::material_sptr>();

    {
      // Create a fan ent
      Circlef32 circle( Circlef32::radius(5.0f) );
      m_fanEnt =
        prefab_gfx::CreateFan(*m_entityMgr, poolMgr, circle, 12);

      tl_float posX = rng::g_defaultRNG.GetRandomFloat(-10.0f, 10.0f);
      tl_float posY = rng::g_defaultRNG.GetRandomFloat(-10.0f, 10.0f);

      m_fanEnt->GetComponent<math_cs::Transform>()->
        SetPosition(math_t::Vec3f(posX, posY, 0));

      auto matPoolItr = matPool->GetNext();
      gfx_cs::material_sptr newMat(new gfx_cs::Material(*m_henryMat) );
      matPoolItr->SetValue(newMat);

      m_entityMgr->InsertComponent(m_fanEnt, matPoolItr->GetValue().get());
      m_entityMgr->InsertComponent(m_fanEnt, m_henryMat.get());
    }

    {
      // Create a fan ent
      Circlef32 circle( Circlef32::radius(0.5f) );
      m_mouseFan =
        prefab_gfx::CreateFan(*m_entityMgr, poolMgr, circle, 12);

      auto matPoolItr = matPool->GetNext();
      gfx_cs::material_sptr newMat(new gfx_cs::Material(*m_crateMat) );
      matPoolItr->SetValue(newMat);

      m_entityMgr->InsertComponent(m_mouseFan, matPoolItr->GetValue().get());
      m_entityMgr->InsertComponent(m_mouseFan, m_crateMat.get());
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

    tl_float posX = rng::g_defaultRNG.GetRandomFloat(-10.0f, 10.0f);
    tl_float posY = rng::g_defaultRNG.GetRandomFloat(-10.0f, 10.0f);

    m_cameraEnt = prefab_gfx::CreateCamera(*m_entityMgr, poolMgr, m_ortho,
                                            math_t::Vec3f(posX, posY, -1.0f));

    quadSys.AttachCamera(m_cameraEnt);
    fanSys.AttachCamera(m_cameraEnt);

    matSys.Initialize();
    quadSys.Initialize();
    fanSys.Initialize();

    while (m_win.IsValid() && m_keyPresses.IsMarked(key_exit) == false)
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        matSys.ProcessActiveEntities();
        quadSys.ProcessActiveEntities();
        fanSys.ProcessActiveEntities();

        m_win.SwapBuffers();
      }
    }
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
          SetPosition(math_t::Vec3f32(0, 0, -1.0f));
        m_cameraEnt->GetComponent<math_cs::Projection>()->
          SetFrustum(m_ortho);
      }
      else
      {
        m_cameraEnt->GetComponent<math_cs::Transform>()->
          SetPosition(math_t::Vec3f32(0, 0, -30.0f));

        math_t::AspectRatio ar(math_t::AspectRatio::width( (tl_float)m_win.GetWidth()),
                               math_t::AspectRatio::height( (tl_float)m_win.GetHeight()) );
        math_t::FOV fov(math_t::Degree(60.0f), ar, math_t::p_FOV::vertical());

        math_proj::FrustumPersp::Params params(fov);
        params.SetFar(100.0f).SetNear(1.0f);

        math_proj::FrustumPersp fr(params);
        fr.BuildFrustum();

        m_cameraEnt->GetComponent<math_cs::Projection>()->SetFrustum(fr);
      }
    }

    return false;
  }

  bool OnKeyRelease(const tl_size, const input::hid::KeyboardEvent&)
  {
    return false;
  }

  bool OnButtonPress(const tl_size ,
                     const input_hid::MouseEvent& ,
                     const input_hid::MouseEvent::button_code_type)
  {
    return false;
  }

  //------------------------------------------------------------------------
  // Called when a button is released. Currently will printf tloc's representation
  // of all buttons.
  bool OnButtonRelease(const tl_size ,
                       const input_hid::MouseEvent& ,
                       const input_hid::MouseEvent::button_code_type)
  {
    return false;
  }

  //------------------------------------------------------------------------
  // Called when mouse is moved. Currently will printf mouse's relative and
  // absolute position.
  bool OnMouseMove(const tl_size ,
                   const input_hid::MouseEvent& a_event)
  {
    MoveMouseAndCheckCollision((f32)a_event.m_X.m_abs().Get(),
                               (f32)a_event.m_Y.m_abs().Get());
    return false;
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  void OnWindowEvent(const event_type& a_event)
  {
    using namespace gfx_win;

    if (a_event.m_type == WindowEvent::close)
    {
      m_win.Close();
    }
  }

  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  gfx_win::Window         m_win;
  gfx_rend::Renderer      m_renderer;
  core_time::Timer32      m_timer;
  core_time::Timer32      m_frameTimer;
  core_time::Timer32      m_renderFrameTime;
  core_time::Timer32      m_physFrameTime;
  const ent_type*         m_cameraEnt;

  ent_type*               m_fanEnt;
  ent_type*               m_mouseFan;

  gfx_gl::texture_object_sptr m_texObjHenry;
  gfx_gl::texture_object_sptr m_texObjCrate;
  gfx_cs::material_sptr       m_henryMat;
  gfx_cs::material_sptr       m_crateMat;

  math_proj::FrustumOrtho m_ortho;

  input::input_mgr_b_ptr     m_inputMgr;
  input::input_mgr_i_ptr     m_inputMgrImm;
  input::hid::KeyboardB*     m_keyboard;
  input::hid::MouseB*        m_mouse;
  input::hid::TouchSurfaceI* m_touchSurface;

  input_hid::TouchSurfaceI::touch_container_type m_currentTouches;
  core::utils::Checkpoints   m_keyPresses;

  core_cs::event_manager_sptr m_eventMgr;
  core_cs::entity_manager_sptr m_entityMgr;

  phys_mgr_type             m_physicsMgr;
  gfx_cs::QuadRenderSystem  m_quadSys;
};
TLOC_DEF_TYPE(glProgram);

int TLOC_MAIN(int, char* [])
{
  glProgram p;
  p.Initialize();
  p.RunGame();

  return 0;
}
