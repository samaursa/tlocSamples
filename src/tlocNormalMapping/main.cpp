#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocApplication/tloc_application.h>

#include <gameAssetsPath.h>

using namespace tloc;

namespace {

  // We need a material to attach to our entity (which we have not yet created).

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathVS("/shaders/tlocOneTextureRotVS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathFS("/shaders/tlocOne3dTextureRotFS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathFS("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
#endif

  const tl_size g_sliceCount = 10;

};

class NormalMapping
  : tloc::Application
{
public:
  typedef tloc::Application                 base_type;

public:
  NormalMapping()
    : base_type("Normal Mapping")
  { }

  void DoCreateSystems()
  {
    auto_cref ecs = this->GetScene();
    ecs->AddSystem<gfx_cs::MeshRenderSystem>();
  };

  error_type Post_Initialize()
  {
    //DoCreateSystems();

    //auto_cref brickDiffTOPtr = app_res::f_resource::LoadImageAsTextureObject
    //  (core_io::Path(core_str::String(GetAssetsPath()) + "/images/brick_diff.jpg"));
    //auto_cref brickNormTOPtr = app_res::f_resource::LoadImageAsTextureObject
    //  (core_io::Path(core_str::String(GetAssetsPath()) + "/images/brick_norm.jpg"));
  }

  //gfx_cs::mesh_render_system_vptr m_meshSys;
};

int TLOC_MAIN(int , char *[])
{
  Application normalMapping("Normal Mapping");
  normalMapping.Initialize(core_ds::MakeTuple(800, 600));
  normalMapping.Run();

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
