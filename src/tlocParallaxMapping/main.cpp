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
  core_str::String shaderPathVS("/shaders/tlocParallaxMapVS.glsl");
#elif defined (TLOC_OS_IPHONE)
  core_str::String shaderPathVS("/shaders/tlocOneTextureVS_gl_es_2_0.glsl");
#endif

#if defined (TLOC_OS_WIN)
  core_str::String shaderPathFS("/shaders/tlocParallaxMapFS.glsl");
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
    : base_type("Parallax Mapping")
  { }

  void DoCreateSystems()
  {
    auto_cref ecs = this->GetScene();
    ecs->AddSystem<gfx_cs::CameraSystem>();
    ecs->AddSystem<gfx_cs::ArcBallSystem>();
    auto abcSys = ecs->AddSystem<input_cs::ArcBallControlSystem>();

    GetKeyboard()->Register(abcSys.get());
    GetMouse()->Register(abcSys.get());

    GetKeyboard()->Register(this);

    ecs->AddSystem<gfx_cs::MaterialSystem>();
    m_meshSys = ecs->AddSystem<gfx_cs::MeshRenderSystem>();
    m_meshSys->SetRenderer(GetRenderer());
  };

  void DoCreateScene()
  {
    gfx_gl::TextureObject::Params toParams;
    toParams.Wrap_S<gfx_gl::p_texture_object::wrap_technique::Repeat>()
            .Wrap_T<gfx_gl::p_texture_object::wrap_technique::Repeat>();


    gfx_gl::uniform_vso u_toDiff;
    {
      auto_cref toPtr = app_res::f_resource::LoadImageAsTextureObject
        (core_io::Path(core_str::String(GetAssetsPath()) + "/images/stone_diff.jpg"), toParams);
      u_toDiff->SetName("s_texture").SetValueAs(*toPtr);
    }

    gfx_gl::uniform_vso u_toNorm;
    {
      auto_cref toPtr = app_res::f_resource::LoadImageAsTextureObject
        (core_io::Path(core_str::String(GetAssetsPath()) + "/images/stone_norm.jpg"), toParams);
      u_toNorm->SetName("s_normTexture").SetValueAs(*toPtr);
    }

    gfx_gl::uniform_vso u_toDisp;
    {
      auto_cref toPtr = app_res::f_resource::LoadImageAsTextureObject
        (core_io::Path(core_str::String(GetAssetsPath()) + "/images/stone_disp.jpg"), toParams);
      u_toDisp->SetName("s_dispTexture").SetValueAs(*toPtr);
    }

    gfx_gl::uniform_vso u_lightPos;
    {
      DoUpdateLightPos();
      u_lightPos->SetName("u_lightPos").SetValueAs(m_lightPos.get());
    }

    gfx_gl::uniform_vso u_scaleBias;
    {
      (*m_scaleBiasEnableParralax)[0] = 0.04f;
      (*m_scaleBiasEnableParralax)[1] = -0.02f;
      (*m_scaleBiasEnableParralax)[2] = 1.0f;
      DoUpdateLightPos();
      u_scaleBias->SetName("u_pScaleBias")
        .SetValueAs(m_scaleBiasEnableParralax.get());
    }

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(5.0f),
                           math_t::Rectf32_c::height(5.0f));
    auto_cref ent = GetScene()->CreatePrefab<pref_gfx::Quad>().Dimensions(rect).Create();
    {
      // quad TBN matrix is an identity matrix
      math_t::Mat3f32 tbn; tbn.MakeIdentity();
      core_conts::Array<math_t::Mat3f32>  tbnArray(4, tbn);

      gfx_gl::attributeVBO_vso  v_tbn;
      v_tbn->AddName("a_tbn").SetValueAs<gfx_gl::p_vbo::target::ArrayBuffer,
        gfx_gl::p_vbo::usage::StaticDraw>(tbnArray);

      ent->GetComponent<gfx_cs::Mesh>()->GetUserShaderOperator()->
        AddAttributeVBO(*v_tbn);
    }

    GetScene()->CreatePrefab<pref_gfx::Material>()
      .AddUniform(u_toDiff.get())
      .AddUniform(u_toNorm.get())
      .AddUniform(u_toDisp.get())
      .AddUniform(u_lightPos.get())
      .AddUniform(u_scaleBias.get())
      .Add(ent, core_io::Path(GetAssetsPath() + shaderPathVS),
      core_io::Path(GetAssetsPath() + shaderPathFS));
    {
      ent->GetComponent<gfx_cs::Material>()->
        SetEnableUniform<gfx_cs::p_material::uniforms::k_viewMatrix>();
    }
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
    ( *m_lightPos )[0] = math::Sin(Radian(lightTime.ElapsedSeconds())) * 5.0f;
    ( *m_lightPos )[1] = math::Cos(Radian(lightTime.ElapsedSeconds())) * 5.0f;
    ( *m_lightPos )[2] = 2.5f;//math::Sin( Radian(lightTime.ElapsedSeconds()) );
  }

  error_type Post_Initialize() override
  {
    DoCreateSystems();
    DoCreateScene();
    DoCreateCamera();

    TLOC_LOG_DEFAULT_DEBUG_NO_FILENAME() << "b, B to increase/decrease bias";
    TLOC_LOG_DEFAULT_DEBUG_NO_FILENAME() << "s, S to increase/decrease scale";
    TLOC_LOG_DEFAULT_DEBUG_NO_FILENAME() << "p    to enable/disable parallax";

    return base_type::Post_Initialize();
  }

  void Pre_Update(sec_type) override
  {
    DoUpdateLightPos();
  }

  event_type OnKeyPress(const tl_size, 
                        const input_hid::KeyboardEvent& a_event) override
  {
    if (a_event.m_keyCode == input_hid::KeyboardEvent::b)
    {
      if (a_event.m_modifier & input_hid::KeyboardEvent::Shift)
      { (*m_scaleBiasEnableParralax)[1] -= 0.01f; }
      else
      { (*m_scaleBiasEnableParralax)[1] += 0.01f; }

      TLOC_LOG_DEFAULT_INFO_NO_FILENAME() << "[Scale, Bias]: " << *m_scaleBiasEnableParralax;
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::s)
    {
      if (a_event.m_modifier & input_hid::KeyboardEvent::Shift)
      { (*m_scaleBiasEnableParralax)[0] -= 0.01f; }
      else
      { (*m_scaleBiasEnableParralax)[0] += 0.01f; }

      TLOC_LOG_DEFAULT_INFO_NO_FILENAME() << "[Scale, Bias]: " << *m_scaleBiasEnableParralax;
    }

    if (a_event.m_keyCode == input_hid::KeyboardEvent::p)
    {
      (*m_scaleBiasEnableParralax)[2] = 1.0f - (*m_scaleBiasEnableParralax)[2];
    }


    return core_dispatch::f_event::Continue();
  }

  gfx_cs::mesh_render_system_vptr m_meshSys;
  core_cs::entity_vptr            m_camEnt;
  math_t::vec3_f32_vso            m_lightPos;
  math_t::vec3_f32_vso            m_scaleBiasEnableParralax;
};
TLOC_DEF_TYPE(Demo);

int TLOC_MAIN(int, char *[])
{
  Demo np;
  np.Initialize(core_ds::MakeTuple(800, 600));
  np.Run();

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;

}