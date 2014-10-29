#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocMath/tloc_math.inl.h>
#include <tlocPrefab/tloc_prefab.h>

#include <samplesAssetsPath.h>

#include <tlocCore/smart_ptr/tloc_smart_ptr.inl.h>
#include <tlocCore/containers/tlocArray.inl.h>

using namespace tloc;

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

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
             gfx_win::WindowSettings("One Quad") );

  //------------------------------------------------------------------------
  // Initialize graphics platform
  if (gfx_gl::InitializePlatform() != ErrorSuccess)
  { TLOC_LOG_GFX_ERR() << "Graphics platform failed to initialize"; return -1; }

  // -----------------------------------------------------------------------
  // Get the default renderer
  gfx_rend::renderer_sptr renderer = win.GetRenderer();

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  //
  // NOTES:
  // It MUST be destroyed AFTER the EntityManager(s) that use its components.
  // This is because upon destruction of EntityManagers, all entities are
  // destroyed which trigger events for removal of all their components. If
  // the components are destroyed, the smart pointers will not let us dereference
  // them and will trigger an assertion.
  core_cs::component_pool_mgr_vso compMgr;

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_vso  eventMgr;
  core_cs::entity_manager_vso entityMgr( MakeArgs(eventMgr.get()) );

  //------------------------------------------------------------------------
  // To render a quad, we need a quad render system - this is a specialized
  // system to render this primitive
  gfx_cs::QuadRenderSystem  quadSys(eventMgr.get(), entityMgr.get());
  quadSys.SetRenderer(renderer);
  quadSys.SetEnabledSortingByMaterial(false);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr.get(), entityMgr.get());

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The quad render system expects a few shader variables to be declared
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
  // The prefab library has some prefabricated entities for us

  { // RGBA
    typedef gfx_med::image_vso  image_vso;
    image_vso rgba;
    rgba->Create(core_ds::MakeTuple(2, 2),
                 gfx_med::Image::color_type(1.0f, 0.9f, 0.8f, 0.7f));

    gfx_gl::texture_object_vso to;
    to->Initialize(*rgba);

    gfx_gl::uniform_vso u_to;
    u_to->SetName("s_texture").SetValueAs(*to);

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(1.5f),
                           math_t::Rectf32_c::height(1.5f));
    core_cs::entity_vptr q =
      pref_gfx::Quad(entityMgr.get(), compMgr.get()).
      TexCoords(true).Dimensions(rect).Create();

    q->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(0, 0, 0));

    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_to.get())
      .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));
  }

  { // RGB
    typedef gfx_med::image_rgb_vso  image_rgb_vso;
    image_rgb_vso rgb;
    rgb->Create(core_ds::MakeTuple(2, 2),
                gfx_med::image_rgb::color_type(0.1f, 0.1f, 1.0f));

    gfx_gl::texture_object_vso to;

    to->Initialize(*rgb);

    gfx_gl::uniform_vso u_to;
    u_to->SetName("s_texture").SetValueAs(*to);

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(1.4f),
                           math_t::Rectf32_c::height(1.4f));
    core_cs::entity_vptr q =
      pref_gfx::Quad(entityMgr.get(), compMgr.get()).
      TexCoords(true).Dimensions(rect).Create();

    q->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(0, 0, 0.1f));

    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_to.get())
      .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));
  }

#ifndef TLOC_OS_IPHONE
  {//RG
    typedef gfx_med::image_rg_vso  image_rg_vso;
    image_rg_vso rg;
    rg->Create(core_ds::MakeTuple(2, 2),
                gfx_med::image_rg::color_type(0.1f, 1.0f));

    gfx_gl::texture_object_vso to;

    to->Initialize(*rg);

    gfx_gl::uniform_vso u_to;
    u_to->SetName("s_texture").SetValueAs(*to);

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(1.3f),
                           math_t::Rectf32_c::height(1.3f));
    core_cs::entity_vptr q =
      pref_gfx::Quad(entityMgr.get(), compMgr.get()).
      TexCoords(true).Dimensions(rect).Create();

    q->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(0, 0, 0.2f));

    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_to.get())
      .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));
  }

  {//R
    typedef gfx_med::image_r_vso  image_r_vso;
    image_r_vso r;
    r->Create(core_ds::MakeTuple(2, 2),
                gfx_med::image_r::color_type(1.0f));

    gfx_gl::texture_object_vso to;

    to->Initialize(*r);

    gfx_gl::uniform_vso u_to;
    u_to->SetName("s_texture").SetValueAs(*to);

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(1.2f),
                           math_t::Rectf32_c::height(1.2f));
    core_cs::entity_vptr q =
      pref_gfx::Quad(entityMgr.get(), compMgr.get()).
      TexCoords(true).Dimensions(rect).Create();

    q->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(0, 0, 0.3f));

    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_to.get())
      .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));
  }

  { // RGBA_16
    typedef gfx_med::image_u16_rgba_vso  image_vso;
    image_vso img;
    img->Create(core_ds::MakeTuple(2, 2),
                 gfx_med::image_u16_rgba::color_type(0.9f, 0.8f, 0.7f, 0.6f));

    gfx_gl::texture_object_vso to;
    to->Initialize(*img);

    gfx_gl::uniform_vso u_to;
    u_to->SetName("s_texture").SetValueAs(*to);

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(1.1f),
                           math_t::Rectf32_c::height(1.1f));
    core_cs::entity_vptr q =
      pref_gfx::Quad(entityMgr.get(), compMgr.get()).
      TexCoords(true).Dimensions(rect).Create();

    q->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(0, 0, 0.4f));

    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_to.get())
      .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));
  }

  { // RGB_16
    typedef gfx_med::image_u16_rgb_vso  image_vso;
    image_vso img;
    img->Create(core_ds::MakeTuple(2, 2),
                 gfx_med::image_u16_rgb::color_type(0.0f, 0.0f, 0.8f));

    gfx_gl::texture_object_vso to;
    to->Initialize(*img);

    gfx_gl::uniform_vso u_to;
    u_to->SetName("s_texture").SetValueAs(*to);

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(1.0f),
                           math_t::Rectf32_c::height(1.0f));
    core_cs::entity_vptr q =
      pref_gfx::Quad(entityMgr.get(), compMgr.get()).
      TexCoords(true).Dimensions(rect).Create();

    q->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(0, 0, 0.5f));

    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_to.get())
      .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));
  }

  { // RG_16
    typedef gfx_med::image_u16_rg_vso  image_vso;
    image_vso img;
    img->Create(core_ds::MakeTuple(2, 2),
                 gfx_med::image_u16_rg::color_type(0.0f, 0.8f));

    gfx_gl::texture_object_vso to;
    to->Initialize(*img);

    gfx_gl::uniform_vso u_to;
    u_to->SetName("s_texture").SetValueAs(*to);

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(0.9f),
                           math_t::Rectf32_c::height(0.9f));
    core_cs::entity_vptr q =
      pref_gfx::Quad(entityMgr.get(), compMgr.get()).
      TexCoords(true).Dimensions(rect).Create();

    q->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(0, 0, 0.6f));

    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_to.get())
      .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));
  }

  { // R_16
    typedef gfx_med::image_u16_r_vso  image_vso;
    image_vso img;
    img->Create(core_ds::MakeTuple(2, 2),
                 gfx_med::image_u16_r::color_type(0.8f));

    gfx_gl::texture_object_vso to;
    to->Initialize(*img);

    gfx_gl::uniform_vso u_to;
    u_to->SetName("s_texture").SetValueAs(*to);

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(0.8f),
                           math_t::Rectf32_c::height(0.8f));
    core_cs::entity_vptr q =
      pref_gfx::Quad(entityMgr.get(), compMgr.get()).
      TexCoords(true).Dimensions(rect).Create();

    q->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(0, 0, 0.7f));

    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_to.get())
      .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));
  }

  { // float (target is auto selected as depth)
    typedef gfx_med::image_f32_r_vso image_vso;
    image_vso img;
    img->Create(core_ds::MakeTuple(2, 2),
                 gfx_med::image_f32_r::color_type(0.2f));

    gfx_gl::texture_object_vso to;
    to->Initialize(*img);

    gfx_gl::uniform_vso u_to;
    u_to->SetName("s_texture").SetValueAs(*to);

    math_t::Rectf32_c rect(math_t::Rectf32_c::width(0.7f),
                           math_t::Rectf32_c::height(0.7f));
    core_cs::entity_vptr q =
      pref_gfx::Quad(entityMgr.get(), compMgr.get()).
      TexCoords(true).Dimensions(rect).Create();

    q->GetComponent<math_cs::Transform>()->SetPosition(math_t::Vec3f32(0, 0, 0.8f));

    pref_gfx::Material(entityMgr.get(), compMgr.get())
      .AddUniform(u_to.get())
      .Add(q, core_io::Path(GetAssetsPath() + shaderPathVS),
              core_io::Path(GetAssetsPath() + shaderPathFS));
  }
#endif // #ifndef TLOC_OS_IPHONE

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

    renderer->ApplyRenderSettings();
    quadSys.ProcessActiveEntities();

    win.SwapBuffers();
  }

  //------------------------------------------------------------------------
  // Exiting
  TLOC_LOG_CORE_INFO() << "Existing normally from sample";

  return 0;
}
