#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

#include <tlocCore/memory/tlocLinkMe.cpp>

#include <cstring>

using namespace tloc;

//------------------------------------------------------------------------
// Temporary code to get proper asset path on platforms

#if defined(TLOC_OS_WIN)

const char* GetAssetPath()
{
  static const char* assetPath = "../../../../../assets/";
  return assetPath;
}
#elif defined(TLOC_OS_IPHONE)
const char* GetAssetPath()
{
  static char assetPath[1024];
  strcpy(assetPath, [[[NSBundle mainBundle] resourcePath]
                     cStringUsingEncoding:[NSString defaultCStringEncoding]]);
  strcat(assetPath, "/assets/");

  return assetPath;
}
#endif

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
             gfx_win::WindowSettings("tlocSimpleQuad") );
  //------------------------------------------------------------------------
  // Initialize renderer
  gfx_rend::Renderer  renderer;
  if (renderer.Initialize() != ErrorSuccess())
  { printf("\nRenderer failed to initialize"); return 1; }

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::EventManager     eventMgr;
  core_cs::EntityManager    entityMgr(&eventMgr);

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  core_cs::ComponentPoolManager compMgr;

  //------------------------------------------------------------------------
  // To render a quad, we need a quad render system - this is a specialized
  // system to render this primitive
  gfx_cs::QuadRenderSystem  quadSys(&eventMgr, &entityMgr);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(&eventMgr, &entityMgr);

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The quad render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.
  gfx_cs::Material  mat;
  {
#if defined (TLOC_OS_WIN)
    core_str::String shaderPath("/tlocPassthroughVertexShader.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPath("/tlocPassthroughVertexShader_gl_es_2_0.glsl");
#endif
    shaderPath = GetAssetPath() + shaderPath;
    core_io::FileIO_ReadA shaderFile(shaderPath.c_str());
    
    if (shaderFile.Open() != ErrorSuccess())
    { printf("\nUnable to open the vertex shader"); return 1;}

    core_str::String code;
    shaderFile.GetContents(code);
    mat.SetVertexSource(code);
  }
  {
#if defined (TLOC_OS_WIN)
    core_str::String shaderPath("/tlocPassthroughFragmentShader.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPath("/tlocPassthroughFragmentShader_gl_es_2_0.glsl");
#endif
    shaderPath = GetAssetPath() + shaderPath;
    core_io::FileIO_ReadA shaderFile(shaderPath.c_str());

    if (shaderFile.Open() != ErrorSuccess())
    { printf("\nUnable to open the fragment shader"); return 1;}

    core_str::String code;
    shaderFile.GetContents(code);
    mat.SetFragmentSource(code);
  }

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us

  math_t::Rectf32 rect(math_t::Rectf32::width(0.5f),
                       math_t::Rectf32::height(0.5f));
  core_cs::Entity* q = prefab_gfx::CreateQuad(entityMgr, compMgr, rect);
  entityMgr.InsertComponent(q, &mat);

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  quadSys.Initialize();
  matSys.Initialize();

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    // Finally, process the quad
    quadSys.ProcessActiveEntities();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  printf("\nExiting normally");

  return 0;
}