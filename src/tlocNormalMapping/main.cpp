#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocInput/tloc_input.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocApplication/tloc_application.h>

#include <gameAssetsPath.h>

using namespace tloc;

namespace {

  // We need a material to attach to our entity (which we have not yet created).

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathVS("/shaders/tlocNormalMapVS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathFS("/shaders/tlocNormalMapFS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathFS("/shaders/tlocOneTextureFS_gl_es_2_0.glsl");
#endif

  const tl_size g_sliceCount = 10;

};

class Demo
  : public tloc::Application
{
public:
  typedef tloc::Application                 base_type;

public:
  Demo()
    : base_type("Normal Mapping")
  { }

  void DoCreateSystems()
  {
    auto_cref ecs = this->GetScene();
    ecs->AddSystem<gfx_cs::CameraSystem>();
    ecs->AddSystem<gfx_cs::ArcBallSystem>();
    auto abcSys = ecs->AddSystem<input_cs::ArcBallControlSystem>();

    GetKeyboard()->Register(abcSys.get());
    GetMouse()->Register(abcSys.get());

    ecs->AddSystem<gfx_cs::MaterialSystem>();
    m_meshSys = ecs->AddSystem<gfx_cs::MeshRenderSystem>();
    m_meshSys->SetRenderer(GetRenderer());
  };

  void DoCreateScene()
  {
    gfx_gl::uniform_vso u_toDiff;
    {
      auto_cref brickDiffTOPtr = app_res::f_resource::LoadImageAsTextureObject
        (core_io::Path(core_str::String(GetAssetsPath()) + "/images/cushion_diff.jpg"));
      u_toDiff->SetName("s_texture").SetValueAs(*brickDiffTOPtr);
    }

    gfx_gl::uniform_vso u_toNorm;
    {
      auto_cref brickNormTOPtr = app_res::f_resource::LoadImageAsTextureObject
      (core_io::Path(core_str::String(GetAssetsPath()) + "/images/cushion_norm.jpg"));
      u_toNorm->SetName("s_normTexture").SetValueAs(*brickNormTOPtr);
    }

    gfx_gl::uniform_vso u_lightPos;
    {
      DoUpdateLightPos();
      u_lightPos->SetName("u_lightPos").SetValueAs(m_lightPos.get());
    }

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(5.0f), 
                           math_t::Rectf32_c::height(5.0f));
    auto ent = GetScene()->CreatePrefab<pref_gfx::Quad>().Dimensions(rect).Create();
    {
      // quad TBN matrix is an identity matrix
      math_t::Mat3f32 tbn; tbn.MakeIdentity();
      core_conts::Array<math_t::Mat3f32>  tbnArray(4, tbn);

      gfx_gl::attributeVBO_vso  v_tbn;
      v_tbn->AddName("a_vertTBN").SetValueAs<gfx_gl::p_vbo::target::ArrayBuffer, 
        gfx_gl::p_vbo::usage::StaticDraw>(tbnArray);

      ent->GetComponent<gfx_cs::Mesh>()->GetUserShaderOperator()->
        AddAttributeVBO(*v_tbn);
    }

    GetScene()->CreatePrefab<pref_gfx::Material>()
      .AddUniform(u_toDiff.get())
      .AddUniform(u_toNorm.get())
      .AddUniform(u_lightPos.get())
      .Add(ent, core_io::Path(GetAssetsPath() + shaderPathVS),
                core_io::Path(GetAssetsPath() + shaderPathFS));
  }

  void DoCreateCamera()
  {
    m_camEnt = GetScene()->CreatePrefab<pref_gfx::Camera>()
      .Near(0.1f).Far(100.0f)
      .Position(math_t::Vec3f(0.0f, 0.0f, 5.0f))
      .Create(GetWindow()->GetDimensions());

    GetScene()->CreatePrefab<pref_gfx::ArcBall>().Add(m_camEnt);
    GetScene()->CreatePrefab<pref_input::ArcBallControl>()
      .GlobalMultiplier(math_t::Vec2f32(0.01f, 0.01f)).Add(m_camEnt);

    m_meshSys->SetCamera(m_camEnt);
  }

  void DoUpdateLightPos()
  {
    static core_time::Timer lightTime;
    using namespace math_t;
    (*m_lightPos)[0] = math::Sin( Radian(lightTime.ElapsedSeconds()) ) * 5.0f;
    (*m_lightPos)[1] = math::Cos( Radian(lightTime.ElapsedSeconds()) ) * 5.0f;
    (*m_lightPos)[2] = 1.0f;//math::Sin( Radian(lightTime.ElapsedSeconds()) );
  }

  error_type Post_Initialize() override
  {
    DoCreateSystems();
    DoCreateScene();
    DoCreateCamera();

    return base_type::Post_Initialize();
  }

  void Pre_Update(sec_type) override
  {
    DoUpdateLightPos();
  }

  gfx_cs::mesh_render_system_vptr m_meshSys;
  core_cs::entity_vptr            m_camEnt;
  math_t::vec3_f32_vso            m_lightPos;
};

int TLOC_MAIN(int , char *[])
{
  Demo np;
  np.Initialize(core_ds::MakeTuple(800, 600));
  np.Run();

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}
