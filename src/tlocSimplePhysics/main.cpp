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
    gfx_win::WindowSettings("tlocSimplePhysics") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { printf("\nGraphics platform failed to initialize"); return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = gfx_rend::GetDefaultRenderer();

  gfx_rend::Renderer::Params p;
  p.SetClearColor(gfx_t::Color(0.5f, 0.5f, 1.0f, 1.0f))
   .SetFBO(gfx_gl::FramebufferObject::GetDefaultFramebuffer())
   .AddClearBit<clear::ColorBufferBit>();

  renderer->SetParams(p);

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_sptr  eventMgr(new core_cs::EventManager());
  core_cs::entity_manager_sptr entityMgr(new core_cs::EntityManager(eventMgr));

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  core_cs::ComponentPoolManager cpoolMgr;

  //------------------------------------------------------------------------
  // To render a quad, we need a quad render system - this is a specialized
  // system to render this primitive
  gfx_cs::QuadRenderSystem  quadSys(eventMgr, entityMgr);
  quadSys.SetRenderer(renderer);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr, entityMgr);

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

  math_t::Rectf32 rect(math_t::Rectf32::width(0.5f),
    math_t::Rectf32::height(0.5f));
  core_cs::Entity* q = prefab_gfx::Quad(entityMgr.get(), &cpoolMgr).
    TexCoords(false).Dimensions(rect).Create();

  prefab_gfx::Material(entityMgr.get(), &cpoolMgr)
    .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
            core_io::Path(GetAssetsPath() + shaderPathFS));

  //------------------------------------------------------------------------
  // For physics, we need a physics manager and the relevant systems

  phys_box2d::PhysicsManager::vec_type g(0, -2.0f);
  phys_box2d::PhysicsManager  physMgr;
  physMgr.Initialize(phys_box2d::PhysicsManager::gravity(g));

  phys_cs::RigidBodySystem rbSys(eventMgr, entityMgr, &physMgr.GetWorld());

  // Make the above quad a rigidbody
  phys_box2d::rigid_body_def_sptr rbDef(new phys_box2d::RigidBodyDef());
  rbDef->SetPosition(phys_box2d::RigidBodyDef::vec_type(0, 1.0f));
  rbDef->SetType<phys_box2d::p_rigid_body::DynamicBody>();

  prefab_phys::RigidBody(entityMgr.get(), &cpoolMgr).Add(q, rbDef);
  prefab_phys::RigidBodyShape(entityMgr.get(), &cpoolMgr).
    Add(q, rect, prefab_phys::RigidBodyShape::density(1.0f));

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
      phys_cs::RigidBody* myRb = rbList[0];
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
  printf("\nExiting normally");

  return 0;
}
