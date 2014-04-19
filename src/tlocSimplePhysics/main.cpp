#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocPhysics/tloc_physics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <tlocCore/memory/tlocLinkMe.cpp>

#include <samplesAssetsPath.h>

using namespace tloc;

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

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
    gfx_win::WindowSettings("Basic Physics") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { printf("\nGraphics platform failed to initialize"); return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  gfx_rend::Renderer::Params p(renderer->GetParams());
  {
    p.SetClearColor(gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f))
      .AddClearBit<clear::ColorBufferBit>();
    renderer->SetParams(p);
  }

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  // See explanation in SimpleQuad sample on why it must be created first.
  core_cs::component_pool_mgr_vso cpoolMgr;

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso  eventMgr;
  core_cs::entity_manager_vso entityMgr( MakeArgs(eventMgr.get()) );

  //------------------------------------------------------------------------
  // To render a quad, we need a quad render system - this is a specialized
  // system to render this primitive
  gfx_cs::QuadRenderSystem  quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The quad render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathVS("/tlocPassthroughVertexShader.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathVS("/tlocPassthroughVertexShader_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathFS("/tlocPassthroughFragmentShader.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathFS("/tlocPassthroughFragmentShader_gl_es_2_0.glsl");
#endif

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  math_t::Rectf32_c rect(math_t::Rectf32_c::width(0.5f),
    math_t::Rectf32_c::height(0.5f));
  core_cs::entity_vptr q = pref_gfx::Quad(entityMgr.get(), cpoolMgr.get())
    .TexCoords(false).Dimensions(rect).Create();

  pref_gfx::Material(entityMgr.get(), cpoolMgr.get())
    .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
            core_io::Path(GetAssetsPath() + shaderPathFS));

  //------------------------------------------------------------------------
  // For physics, we need a physics manager and the relevant systems

  phys_box2d::PhysicsManager::vec_type g(0, -2.0f);
  phys_box2d::PhysicsManager  physMgr;
  physMgr.Initialize(phys_box2d::PhysicsManager::gravity(g));

  phys_cs::RigidBodySystem rbSys(eventMgr.get(), entityMgr.get(),
                                 &physMgr.GetWorld());

  // Make the above quad a rigidbody
  phys_box2d::rigid_body_def_sptr rbDef =
    core_sptr::MakeShared<phys_box2d::RigidBodyDef>();
  rbDef->SetPosition(phys_box2d::RigidBodyDef::vec_type(0, 1.0f));
  rbDef->SetType<phys_box2d::p_rigid_body::DynamicBody>();

  pref_phys::RigidBody(entityMgr.get(), cpoolMgr.get()).Add(q, rbDef);
  pref_phys::RigidBodyShape(entityMgr.get(), cpoolMgr.get())
    .Add(q, rect, pref_phys::RigidBodyShape::density(1.0f));

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  quadSys.Initialize();
  matSys.Initialize();
  rbSys.Initialize();

  // We need a timer for physics
  core_time::Timer physTimer;

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    if (physTimer.ElapsedSeconds() > 0.01f)
    {
      physMgr.Update((tl_float)physTimer.ElapsedSeconds());
      physTimer.Reset();

      rbSys.ProcessActiveEntities();
    }

    // Wrap the quad
    core_cs::ComponentMapper<phys_cs::RigidBody> rbList =
      q->GetComponents(phys_cs::components::k_rigidBody);
    if (rbList.size())
    {
      phys_cs::rigid_body_sptr myRb = rbList[0];
      phys_box2d::RigidBody::vec_type pos;
      myRb->GetRigidBody().GetPosition(pos);
      if (pos[1] < -1.0f)
      {
        myRb->GetRigidBody().SetTransform
          ( phys_box2d::RigidBody::vec_type(0, 1.0f), math_t::Degree(0.0f) );
      }
    }

    renderer->ApplyRenderSettings();
    quadSys.ProcessActiveEntities();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  printf("\nExiting normally\n");

  return 0;
}
