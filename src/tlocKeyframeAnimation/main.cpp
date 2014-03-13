#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocInput/tloc_input.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocAnimation/tloc_animation.h>

#include <tlocCore/memory/tlocLinkMe.cpp>

#include <samplesAssetsPath.h>

using namespace tloc;

anim_cs::transform_animation_vptr g_tformAnimComp = nullptr;
const tl_size g_fpsInterval = 5;

class WindowCallback
{
public:
  WindowCallback()
    : m_endProgram(false)
  { }

  void OnWindowEvent(const gfx_win::WindowEvent& a_event)
  {
    if (a_event.m_type == gfx_win::WindowEvent::close)
    { m_endProgram = true; }
  }

  bool  m_endProgram;
};
TLOC_DEF_TYPE(WindowCallback);

//------------------------------------------------------------------------
// KeyboardCallback

class MayaCam
{
  enum
  {
    k_altPressed = 0,
    k_rotating,
    k_panning,
    k_dolly,
    k_count
  };

public:
  MayaCam(core_cs::entity_vptr a_camera)
    : m_camera(a_camera)
    , m_flags(k_count)
  {
    TLOC_ASSERT(a_camera->HasComponent(gfx_cs::components::arcball),
      "Camera does not have ArcBall component");
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnButtonPress(const tl_size ,
                     const input_hid::MouseEvent&,
                     const input_hid::MouseEvent::button_code_type a_button)
  {
    if (a_button == input_hid::MouseEvent::left)
    {
      if (m_flags.IsMarked(k_altPressed))
      { m_flags.Mark(k_rotating); }
      else
      { m_flags.Unmark(k_rotating); }
    }

    if (a_button == input_hid::MouseEvent::middle)
    {
      if (m_flags.IsMarked(k_altPressed))
      { m_flags.Mark(k_panning); }
      else
      { m_flags.Unmark(k_panning); }
    }

    if (a_button == input_hid::MouseEvent::right)
    {
      if (m_flags.IsMarked(k_altPressed))
      { m_flags.Mark(k_dolly); }
      else
      { m_flags.Unmark(k_dolly); }
    }

    return false;
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnButtonRelease(const tl_size ,
                       const input_hid::MouseEvent&,
                       const input_hid::MouseEvent::button_code_type a_button)
  {
    if (a_button == input_hid::MouseEvent::left)
    {
      m_flags.Unmark(k_rotating);
    }

    if (a_button == input_hid::MouseEvent::middle)
    {
      m_flags.Unmark(k_panning);
    }

    if (a_button == input_hid::MouseEvent::right)
    {
      m_flags.Unmark(k_dolly);
    }

    return false;
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnMouseMove(const tl_size ,
                   const input_hid::MouseEvent& a_event)
  {
    f32 xRel = core_utils::CastNumber<f32>(a_event.m_X.m_rel());
    f32 yRel = core_utils::CastNumber<f32>(a_event.m_Y.m_rel());

    if (m_flags.IsMarked(k_rotating))
    {
      gfx_cs::arcball_vptr arcBall = m_camera->GetComponent<gfx_cs::ArcBall>();

      arcBall->MoveVertical(yRel * 0.01f );
      arcBall->MoveHorizontal(xRel * 0.01f );
    }
    else if (m_flags.IsMarked(k_panning))
    {
      math_cs::transform_vptr t = m_camera->GetComponent<math_cs::Transform>();
      gfx_cs::arcball_vptr arcBall = m_camera->GetComponent<gfx_cs::ArcBall>();

      math_t::Vec3f32 leftVec; t->GetOrientation().GetCol(0, leftVec);
      math_t::Vec3f32 upVec; t->GetOrientation().GetCol(1, upVec);

      leftVec *= xRel * 0.01f;
      upVec *= yRel * 0.01f;

      t->SetPosition(t->GetPosition() - leftVec + upVec);
      arcBall->SetFocus(arcBall->GetFocus() - leftVec + upVec);
    }
    else if (m_flags.IsMarked(k_dolly))
    {
      math_cs::transform_vptr t = m_camera->GetComponent<math_cs::Transform>();

      math_t::Vec3f32 dirVec; t->GetOrientation().GetCol(2, dirVec);

      dirVec *= xRel * 0.01f;

      t->SetPosition(t->GetPosition() - dirVec);
    }

    return false;
  }

  // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  bool OnKeyPress(const tl_size ,
                  const input_hid::KeyboardEvent& a_event)
  {
    if (a_event.m_keyCode == input_hid::KeyboardEvent::left_alt)
    {
      m_flags.Mark(k_altPressed);
    }
    return false;
  }

  //------------------------------------------------------------------------
  // Called when a key is released. Currently will printf tloc's representation
  // of the key.
  bool OnKeyRelease(const tl_size ,
                    const input_hid::KeyboardEvent& a_event)
  {
    if (a_event.m_keyCode == input_hid::KeyboardEvent::left_alt)
    {
      m_flags.Unmark(k_altPressed);
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::r)
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
      printf("\nCurrent FPS: %u", g_tformAnimComp->GetFPS());
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::minus_main)
    {
      if (g_tformAnimComp->GetFPS() >= g_fpsInterval)
      {
        g_tformAnimComp->SetFPS(g_tformAnimComp->GetFPS() - g_fpsInterval);
      printf("\nCurrent FPS: %u", g_tformAnimComp->GetFPS());
      }
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::n0)
    {
      g_tformAnimComp->SetCurrentKFSequence(0);
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::n1)
    {
      g_tformAnimComp->SetCurrentKFSequence(1);
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
        printf("\nInterpolation type: linear");
        break;
      case anim_t::p_keyframe::k_ease_in_cubic:
        printf("\nInterpolation type: ease in (cubic)");
        break;
      case anim_t::p_keyframe::k_ease_out_cubic:
        printf("\nInterpolation type: ease out (cubic)");
        break;
      case anim_t::p_keyframe::k_ease_in_out_cubic:
        printf("\nInterpolation type: ease in-out (cubic)");
        break;
      case anim_t::p_keyframe::k_ease_in_quadratic:
        printf("\nInterpolation type: ease in (quadratic)");
        break;
      case anim_t::p_keyframe::k_ease_out_quadratic:
        printf("\nInterpolation type: ease out (quadratic)");
        break;
      case anim_t::p_keyframe::k_ease_in_out_quadratic:
        printf("\nInterpolation type: ease in-out (quadratic)");
        break;
      case anim_t::p_keyframe::k_ease_in_sin:
        printf("\nInterpolation type: ease in (sin)");
        break;
      case anim_t::p_keyframe::k_ease_out_sin:
        printf("\nInterpolation type: ease out (sin)");
        break;
      case anim_t::p_keyframe::k_ease_in_out_sin:
        printf("\nInterpolation type: ease in-out (sin)");
        break;
      }
    }

    return false;
  }

  core_cs::entity_vptr      m_camera;
  core_utils::Checkpoints   m_flags;
};
TLOC_DEF_TYPE(MayaCam);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(1024, 768),
    gfx_win::WindowSettings("Keyframe Animation") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { printf("\nGraphics platform failed to initialize"); return -1; }

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
  ParamList<core_t::Any> kbParams;
  kbParams.m_param1 = win.GetWindowHandle();

  input::input_mgr_b_ptr inputMgr =
    input::input_mgr_b_ptr(new input::InputManagerB(kbParams));

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::KeyboardB* keyboard = inputMgr->CreateHID<input_hid::KeyboardB>();
  input_hid::MouseB* mouse = inputMgr->CreateHID<input_hid::MouseB>();

  // Check pointers
  TLOC_ASSERT_NOT_NULL(keyboard);
  TLOC_ASSERT_NOT_NULL(mouse);

  // -----------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  core_cs::component_pool_mgr_vso cpoolMgr;

  // -----------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso  eventMgr;
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

  // -----------------------------------------------------------------------
  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The fan render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.

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

  // -----------------------------------------------------------------------
  // Add a texture to the material. We need:
  //  * an image
  //  * a TextureObject (preparing the image for OpenGL)
  //  * a Uniform (all textures are uniforms in shaders)
  //  * a ShaderOperator (this is what the material will take)

  gfx_med::ImageLoaderPng png;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                       "/images/crateTexture.png").c_str() );

  if (png.Load(path) != ErrorSuccess)
  { TLOC_ASSERT(false, "Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  gfx_gl::texture_object_vso to;
  to->Initialize(png.GetImage());
  to->Activate();

  gfx_gl::uniform_vso u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  // -----------------------------------------------------------------------
  // ObjLoader can load (basic) .obj files

  path = core_io::Path( (core_str::String(GetAssetsPath()) +
                         "/models/crate.obj").c_str() );

  core_io::FileIO_ReadA objFile(path);
  if (objFile.Open() != ErrorSuccess)
  { printf("\nUnable to open the .obj file."); return 1;}

  core_str::String objFileContents;
  objFile.GetContents(objFileContents);

  gfx_med::ObjLoader objLoader;
  if (objLoader.Init(objFileContents) != ErrorSuccess)
  { printf("\nParsing errors in .obj file."); return 1; }

  if (objLoader.GetNumGroups() == 0)
  { printf("\nObj file does not have any objects."); return 1; }

  gfx_med::ObjLoader::vert_cont_type vertices;
  objLoader.GetUnpacked(vertices, 0);

  // -----------------------------------------------------------------------
  // Create the mesh and add the material

  core_cs::entity_vptr ent =
    prefab_gfx::Mesh(entityMgr.get(), cpoolMgr.get()).Create(vertices);

  anim_t::keyframe_sequence_mat4f32 KFs;

  math_cs::Transform transform;

  using namespace anim_t::p_keyframe;
  {
    transform.SetPosition(math_t::Vec3f32(0, 0, 0));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 0,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(10.0f, 0, 0));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 24 * 4,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(0.0f, 0, 0));
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

  prefab_anim::TransformAnimation ta(entityMgr.get(), cpoolMgr.get());
  ta.Fps(60).Loop(true).StartingFrame(0).Add(ent, KFs);

  anim_t::keyframe_sequence_mat4f32 KFs_2;

  using namespace anim_t::p_keyframe;
  {
    transform.SetPosition(math_t::Vec3f32(0, 0, 0));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 0,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs_2.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(3.0f, 0, 0));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 24 * 2,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs_2.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(3.0f, 0, 3.0f));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 24 * 4,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs_2.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(0.0f, 0, 3.0f));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 24 * 6,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs_2.AddKeyframe(kf);
  }

  {
    transform.SetPosition(math_t::Vec3f32(0.0f, 0.0f, 0));
    anim_t::keyframe_mat4f32 kf(transform.GetTransformation(), 24 * 8,
      anim_t::keyframe_mat4f32::interpolation_type(k_linear));
    KFs_2.AddKeyframe(kf);
  }

  ta.Fps(48).Add(ent, KFs_2);

  g_tformAnimComp = ent->GetComponent<anim_cs::TransformAnimation>();

  // -----------------------------------------------------------------------
  // Create and add a material to ent

  prefab_gfx::Material(entityMgr.get(), cpoolMgr.get())
    .AddUniform(u_to.get())
    .Add(ent, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library

  math_t::AspectRatio ar(math_t::AspectRatio::width( (tl_float)win.GetWidth()),
    math_t::AspectRatio::height( (tl_float)win.GetHeight()) );
  math_t::FOV fov(math_t::Degree(60.0f), ar, math_t::p_FOV::vertical());

  math_proj::FrustumPersp::Params params(fov);
  params.SetFar(100.0f).SetNear(1.0f);

  math_proj::FrustumPersp fr(params);
  fr.BuildFrustum();

  core_cs::entity_vptr m_cameraEnt =
    prefab_gfx::Camera(entityMgr.get(), cpoolMgr.get())
    .Create(fr, math_t::Vec3f(5.0f, 5.0f, 10.0f));

  prefab_gfx::ArcBall(entityMgr.get(), cpoolMgr.get())
    .Focus(math_t::Vec3f32(5.0f, 0.0f, 0.0f)).Add(m_cameraEnt);

  meshSys.SetCamera(m_cameraEnt);

  MayaCam mayaCam(m_cameraEnt);
  keyboard->Register(&mayaCam);
  mouse->Register(&mayaCam);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  meshSys.Initialize();
  matSys.Initialize();
  taSys.Initialize();
  camSys.Initialize();
  arcBallSys.Initialize();

  // -----------------------------------------------------------------------
  // Main loop

  printf("\nPress ALT and Left, Middle and Right mouse buttons to manipulate the camera");

  printf("\nP - to toggle pause");
  printf("\nL - to toggle looping");
  printf("\nS - to toggle stop");
  printf("\nI - to change interpolation type");
  printf("\n= - increase FPS");
  printf("\n- - decrease FPS");

  printf("\n\n0 - First keyframe sequence");
  printf("\n1 - Second keyframe sequence");

  printf("\n\nRight Arrow - goto previous frame");
  printf("\nLeft Arrow  - goto next frame");

  core_time::Timer64 t;

  // Very important to enable depth testing
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    inputMgr->Update();

    f64 deltaT = t.ElapsedSeconds();

    if (deltaT > 1.0f/60.0f)
    {
      arcBallSys.ProcessActiveEntities();
      camSys.ProcessActiveEntities();
      taSys.ProcessActiveEntities(deltaT);
      // Finally, process (render) the mesh
      renderer->ApplyRenderSettings();
      meshSys.ProcessActiveEntities();

      win.SwapBuffers();
      t.Reset();
    }
  }

  // -----------------------------------------------------------------------
  // cleanup

  g_tformAnimComp.reset();

  // -----------------------------------------------------------------------
  // Exiting
  printf("\nExiting normally\n");

  return 0;

}
