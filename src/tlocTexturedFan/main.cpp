#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
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
    gfx_win::WindowSettings("tlocTexturedFan") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { printf("\nGraphics platform failed to initialize"); return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  using namespace gfx_rend::p_renderer;
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  // -----------------------------------------------------------------------
  // Load the required resources. Note that these VSOs will check vptr count
  // before destruction which is why we are placing them here. We want the
  // component pool manager to be destroyed before these are destroyed.

  gfx_med::ImageLoaderPng png;
  core_io::Path path( (core_str::String(GetAssetsPath()) +
                      "/images/uv_grid_col.png").c_str() );

  if (png.Load(path) != ErrorSuccess)
  { TLOC_ASSERT(false, "Image did not load!"); }

  // gl::Uniform supports quite a few types, including a TextureObject
  gfx_gl::texture_object_vso to;
  to->Initialize(png.GetImage());
  to->Activate();

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  core_cs::component_pool_mgr_vso cpoolMgr;

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso  eventMgr;
  core_cs::entity_manager_vso entityMgr(eventMgr.get());

  //------------------------------------------------------------------------
  // To render a fan, we need a fan render system - this is a specialized
  // system to render this primitive
  gfx_cs::FanRenderSystem   fanSys(eventMgr.get(), entityMgr.get());
  fanSys.SetRenderer(renderer);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The fan render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathVS("/shaders/tlocOneTextureVS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
    core_str::String shaderPathFS("/shaders/tlocOneTextureFS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPathFS("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
#endif

  //------------------------------------------------------------------------
  // Add a texture to the material. We need:
  //  * an image
  //  * a TextureObject (preparing the image for OpenGL)
  //  * a Uniform (all textures are uniforms in shaders)
  //  * a ShaderOperator (this is what the material will take)
  //
  // The material takes in a 'MasterShaderOperator' which is the user defined
  // shader operator and over-rides any shader operators that the systems
  // may be setting. Any uniforms/shaders set in the 'MasterShaderOperator'
  // that have the same name as the one in the system will be given preference.

  gfx_gl::uniform_vso  u_to;
  u_to->SetName("s_texture").SetValueAs(*to);

  gfx_gl::shader_operator_vso so;
  so->AddUniform(*u_to);

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  math_t::Circlef32 circ(math_t::Circlef32::radius(1.0f));
  core_cs::entity_vptr q =
    prefab_gfx::Fan(entityMgr.get(), cpoolMgr.get())
    .Sides(64).Circle(circ).Create();

  // -----------------------------------------------------------------------
  // The prefab library can also create the material for us and attach it
  // to the entity

  prefab_gfx::Material(entityMgr.get(), cpoolMgr.get())
    .AddUniform(u_to.get())
    .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
            core_io::Path(GetAssetsPath() + shaderPathFS));

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  fanSys.Initialize();
  matSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    renderer->ApplyRenderSettings();
    fanSys.ProcessActiveEntities();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  printf("\nExiting normally\n");

  return 0;

}
