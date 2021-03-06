#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocInput/tloc_input.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocAnimation/tloc_animation.h>

#include <gameAssetsPath.h>

TLOC_DEFINE_THIS_FILE_NAME();

using namespace tloc;

namespace {

  anim_cs::transform_animation_vptr g_tformAnimComp = nullptr;
  const tl_size g_fpsInterval = 5;

  // -----------------------------------------------------------------------
  // We need a material to attach to our entity (which we have not yet created).

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathVS("/shaders/tlocTexturedMeshVS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathVS("/shaders/tlocTexturedMeshVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathFS("/shaders/tlocTexturedMeshFS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathFS("/shaders/tlocTexturedMeshFS_gl_es_2_0.glsl");
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

//------------------------------------------------------------------------
// KeyboardCallback

class InputCallbacks
{
public:
  InputCallbacks()
  { }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyPress(const tl_size , const input_hid::KeyboardEvent& )
  {
    return core_dispatch::f_event::Continue();
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  core_dispatch::Event 
    OnKeyRelease(const tl_size , const input_hid::KeyboardEvent& a_event)
  {
    if (a_event.m_keyCode == input_hid::KeyboardEvent::r)
    { g_tformAnimComp->SetReverse(!g_tformAnimComp->IsReversed()); }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::l)
    { g_tformAnimComp->SetLooping(!g_tformAnimComp->IsLooping()); }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::p)
    { g_tformAnimComp->SetPaused(!g_tformAnimComp->IsPaused()); }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::s)
    { g_tformAnimComp->SetStopped(!g_tformAnimComp->IsStopped()); }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::right)
    { g_tformAnimComp->NextFrame(); }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::left)
    { g_tformAnimComp->PrevFrame(); }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::equals)
    {
      g_tformAnimComp->SetFPS(g_tformAnimComp->GetFPS() + g_fpsInterval);
      TLOC_LOG_CORE_INFO() << 
        core_str::Format("Current FPS: %lu", g_tformAnimComp->GetFPS());
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::minus_main)
    {
      if (g_tformAnimComp->GetFPS() >= g_fpsInterval)
      {
        g_tformAnimComp->SetFPS(g_tformAnimComp->GetFPS() - g_fpsInterval);

      TLOC_LOG_CORE_INFO() << 
        core_str::Format("Current FPS: %lu", g_tformAnimComp->GetFPS());
      }
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::i)
    {

      anim_cs::TransformAnimation::kf_seq_type& kfSeq = g_tformAnimComp->
        GetCurrentKeyframeSequence();

      for (tl_size i = 0; i < kfSeq.size(); ++i)
      {
        kfSeq[i].SetInterpolationType(kfSeq[i].GetInterpolationType() + 1);

        if (kfSeq[i].GetInterpolationType() == anim_t::p_keyframe::k_count)
        { kfSeq[i].SetInterpolationType(anim_t::p_keyframe::k_linear); }
      }

      switch(kfSeq[0].GetInterpolationType())
      {
      case anim_t::p_keyframe::k_linear:
        TLOC_LOG_CORE_INFO() << "Interpolation type: linear";
        break;
      case anim_t::p_keyframe::k_ease_in_cubic:
        TLOC_LOG_CORE_INFO() << "Interpolation type: ease in (cubic)";
        break;
      case anim_t::p_keyframe::k_ease_out_cubic:
        TLOC_LOG_CORE_INFO() << "Interpolation type: ease out (cubic)";
        break;
      case anim_t::p_keyframe::k_ease_in_out_cubic:
        TLOC_LOG_CORE_INFO() << "Interpolation type: ease in-out (cubic)";
        break;
      case anim_t::p_keyframe::k_ease_in_quadratic:
        TLOC_LOG_CORE_INFO() << "Interpolation type: ease in (quadratic)";
        break;
      case anim_t::p_keyframe::k_ease_out_quadratic:
        TLOC_LOG_CORE_INFO() << "Interpolation type: ease out (quadratic)";
        break;
      case anim_t::p_keyframe::k_ease_in_out_quadratic:
        TLOC_LOG_CORE_INFO() << "Interpolation type: ease in-out (quadratic)";
        break;
      case anim_t::p_keyframe::k_ease_in_sin:
        TLOC_LOG_CORE_INFO() << "Interpolation type: ease in (sin)";
        break;
      case anim_t::p_keyframe::k_ease_out_sin:
        TLOC_LOG_CORE_INFO() << "Interpolation type: ease out (sin)";
        break;
      case anim_t::p_keyframe::k_ease_in_out_sin:
        TLOC_LOG_CORE_INFO() << "Interpolation type: ease in-out (sin)";
        break;
      }
    }

    return core_dispatch::f_event::Continue();
  }
};
TLOC_DEF_TYPE(InputCallbacks);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(800, 600),
    gfx_win::WindowSettings("Simple Scenegraph") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  using gfx_rend::Renderer;
  gfx_rend::Renderer::Params p(renderer->GetParams());
  p.SetClearColor(gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f))
   .Enable<enable_disable::DepthTest>()
   .AddClearBit<clear::ColorBufferBit>()
   .AddClearBit<clear::DepthBufferBit>();

  renderer->SetParams(p);

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<core_t::Any> kbParams;
  kbParams.m_param1 = win.GetWindowHandle();

  input::input_mgr_b_ptr inputMgr =
    core_sptr::MakeShared<input::InputManagerB>(kbParams);

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::keyboard_b_vptr keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();
  input_hid::mouse_b_vptr    mouse = inputMgr->CreateHID<input_hid::MouseB>();

  // Check pointers
  TLOC_ASSERT_NOT_NULL(keyboard);
  TLOC_ASSERT_NOT_NULL(mouse);

  // -----------------------------------------------------------------------
  // Load the required resources. Note that these VSOs will check vptr count
  // before destruction which is why we are placing them here. We want the
  // component pool manager to be destroyed before these are destroyed.

  gfx_med::ImageLoaderPng png;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                       "/images/crateTexture.png").c_str() );

  if (png.Load(path) != ErrorSuccess)
  { TLOC_ASSERT_FALSE("Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  gfx_gl::texture_object_vso to;
  to->Initialize(*png.GetImage());

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  // See explanation in SimpleQuad sample on why it must be created first.
  core_cs::component_pool_mgr_vso cpoolMgr;

  // -----------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso eventMgr;
  core_cs::entity_manager_vso entityMgr(eventMgr.get());

  // -----------------------------------------------------------------------
  // To render a mesh, we need a mesh render system - this is a specialized
  // system to render this primitive
  gfx_cs::MeshRenderSystem  meshSys(eventMgr.get(), entityMgr.get());
  meshSys.SetRenderer(renderer);

  // -----------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  // -----------------------------------------------------------------------
  // One of the keyframe animation systems
  anim_cs::TransformAnimationSystem taSys(eventMgr.get(), entityMgr.get());

  // -----------------------------------------------------------------------
  // The camera's view transformations are calculated by the camera system
  gfx_cs::CameraSystem      camSys(eventMgr.get(), entityMgr.get());
  gfx_cs::ArcBallSystem     arcBallSys(eventMgr.get(), entityMgr.get());
  input_cs::ArcBallControlSystem arcBallControlSys(eventMgr.get(), entityMgr.get());

  // -----------------------------------------------------------------------
  // SceneNodes require the SceneGraphSystem
  gfx_cs::SceneGraphSystem  sgSys(eventMgr.get(), entityMgr.get());

  // -----------------------------------------------------------------------
  // Add a texture to the material. We need:
  //  * an image
  //  * a TextureObject (preparing the image for OpenGL)
  //  * a Uniform (all textures are uniforms in shaders)
  //  * a ShaderOperator (this is what the material will take)

  // -----------------------------------------------------------------------
  // ObjLoader can load (basic) .obj files

  path = core_io::Path( (core_str::String(GetAssetsPath()) +
                         "/models/Crate.obj").c_str() );

  core_io::FileIO_ReadA objFile(path);
  if (objFile.Open() != ErrorSuccess)
  { 
    TLOC_LOG_CORE_ERR() << "Unable to open the .obj file.";
    return 1;
  }

  core_str::String objFileContents;
  objFile.GetContents(objFileContents);

  gfx_med::ObjLoader objLoader;
  if (objLoader.Init(objFileContents) != ErrorSuccess)
  { 
    TLOC_LOG_CORE_ERR() << "Parsing errors in .obj file.";
    return 1;
  }

  if (objLoader.GetNumGroups() == 0)
  { 
    TLOC_LOG_CORE_ERR() << "Obj file does not have any objects.";
    return 1;
  }

  gfx_med::ObjLoader::vert_cont_type vertices;
  objLoader.GetUnpacked(vertices, 0);

  // -----------------------------------------------------------------------
  // Create the mesh and add the material

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(to.get());

  gfx_gl::uniform_vso  u_lightDir;
  u_lightDir->SetName("u_lightDir").SetValueAs(math_t::Vec3f32(0.2f, 0.5f, 3.0f));

  pref_gfx::Material prefMat(entityMgr.get(), cpoolMgr.get());
  prefMat.AddUniform(u_to.get())
         .AddUniform(u_lightDir.get());

  // Ent2 made first just to highlight the sorting feature of the scene graph.
  // That is, the SceneGraph will correctly sort the nodes regardless of when
  // the parent child hierarchies are made
  core_cs::entity_vptr ent_2 =
    pref_gfx::Mesh(entityMgr.get(), cpoolMgr.get()).Create(vertices);
  ent_2->GetComponent<gfx_cs::Mesh>()->
    SetEnableUniform<gfx_cs::p_renderable::uniforms::k_normalMatrix>();

  pref_gfx::SceneNode(entityMgr.get(), cpoolMgr.get()).Add(ent_2);

  prefMat.Add(ent_2, core_io::Path(GetAssetsPath() + shaderPathVS),
                     core_io::Path(GetAssetsPath() + shaderPathFS));
  {
    auto entMat = ent_2->GetComponent<gfx_cs::Material>();
    entMat->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewMatrix>();
  }

  core_cs::entity_vptr ent =
    pref_gfx::Mesh(entityMgr.get(), cpoolMgr.get()).Create(vertices);
  ent->GetComponent<gfx_cs::Mesh>()->
    SetEnableUniform<gfx_cs::p_renderable::uniforms::k_normalMatrix>();

  pref_gfx::SceneNode(entityMgr.get(), cpoolMgr.get()).Add(ent);

  prefMat.Add(ent, core_io::Path(GetAssetsPath() + shaderPathVS),
                   core_io::Path(GetAssetsPath() + shaderPathFS));
  {
    auto entMat = ent->GetComponent<gfx_cs::Material>();
    entMat->SetEnableUniform<gfx_cs::p_material::uniforms::k_viewMatrix>();
  }

  gfx_cs::scene_node_sptr parentNode = ent->GetComponent<gfx_cs::SceneNode>();

  // Set ent as the parent of ent2
  parentNode->AddChild
    (core_sptr::ToVirtualPtr(ent_2->GetComponent<gfx_cs::SceneNode>()) );

  anim_t::keyframe_sequence_mat4f32 KFs;
  anim_t::keyframe_sequence_mat4f32 KFs_2ndCrate;

  math_cs::Transform transform;

  using namespace anim_t::p_keyframe;
  {
    transform.SetPosition(math_t::Vec3f32(0, 0, 0));
    math_t::Mat3f32 mat;
    mat.MakeRotationZ(3.142f * 0.75f);
    transform.SetOrientation(mat);
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 0,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(10.0f, 0, 0));
    math_t::Mat3f32 mat;
    //mat.MakeEulerZYX(180, 0, 0);
    mat.MakeRotationZ(3.142f * 0.75f);
    transform.SetOrientation(mat);
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 24 * 4,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(0.0f, 0, 0));
    math_t::Mat3f32 mat;
    mat.MakeRotationZ(3.142f * 0.75f);
    transform.SetOrientation(mat);
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 24 * 8,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(5.0f, 0, 0));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 24 * 10,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(0.0f, 0, 0));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 24 * 12,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs.AddKeyframe(kf);
  }

  // -----------------------------------------------------------------------
  // Second crate
  {
    transform.SetPosition(math_t::Vec3f32(5.0f, 0, -5.0f));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 0,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs_2ndCrate.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(5.0f, 0, 5.0f));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 24 * 4,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs_2ndCrate.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(5.0f, 0, -5.0f));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 24 * 8,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs_2ndCrate.AddKeyframe(kf);
  }

  pref_anim::TransformAnimation ta(entityMgr.get(), cpoolMgr.get());
  ta.Fps(60).Loop(true).StartingFrame(0).Add(ent, KFs);

  pref_anim::TransformAnimation ta2(entityMgr.get(), cpoolMgr.get());
  ta.Fps(60).Loop(true).StartingFrame(0).Add(ent_2, KFs_2ndCrate);

  g_tformAnimComp = ent->GetComponent<anim_cs::TransformAnimation>();

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library

  core_cs::entity_vptr m_cameraEnt =
    pref_gfx::Camera(entityMgr.get(), cpoolMgr.get())
    .Near(1.0f)
    .Far(100.0f)
    .VerticalFOV(math_t::Degree(60.0f))
    .Position(math_t::Vec3f(5.0f, 5.0f, 10.0f))
    .Create(win.GetDimensions());

  pref_gfx::ArcBall(entityMgr.get(), cpoolMgr.get())
    .Focus(math_t::Vec3f32(5.0f, 0.0f, 0.0f)).Add(m_cameraEnt);
  pref_input::ArcBallControl(entityMgr.get(), cpoolMgr.get())
    .GlobalMultiplier(math_t::Vec2f(0.01f, 0.01f))
    .Add(m_cameraEnt);

  meshSys.SetCamera(m_cameraEnt);

  keyboard->Register(&arcBallControlSys);
  mouse->Register(&arcBallControlSys);

  InputCallbacks ic;
  keyboard->Register(&ic);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  meshSys.Initialize();
  matSys.Initialize();
  taSys.Initialize();
  camSys.Initialize();
  arcBallSys.Initialize();
  arcBallControlSys.Initialize();
  sgSys.Initialize();

  // -----------------------------------------------------------------------
  // Main loop

  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << 
    "Press ALT and Left, Middle and Right mouse buttons to manipulate the camera";

  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "P - to toggle pause";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "L - to toggle looping";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "S - to toggle stop";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "I - to change interpolation type";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "= - increase FPS";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "- - decrease FPS";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "-----------------------------------";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "0 - First keyframe sequence";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "1 - Second keyframe sequence";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "-----------------------------------";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "Right Arrow - goto previous frame";
  TLOC_LOG_CORE_DEBUG_NO_FILENAME() << "Left Arrow  - goto next frame";

  core_time::Timer64 t;

  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    inputMgr->Update();
    arcBallControlSys.ProcessActiveEntities();
    arcBallSys.ProcessActiveEntities();

    f64 deltaT = t.ElapsedSeconds();

    if (deltaT > 1.0f/60.0f)
    {
      renderer->ApplyRenderSettings();

      camSys.    ProcessActiveEntities();
      taSys.     ProcessActiveEntities(deltaT);
      sgSys.     ProcessActiveEntities(deltaT);
      meshSys.   ProcessActiveEntities();

      renderer->Render();

      win.SwapBuffers();
      t.Reset();
    }
  }

  // -----------------------------------------------------------------------
  // Cleanup

  g_tformAnimComp.reset();

  // -----------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
