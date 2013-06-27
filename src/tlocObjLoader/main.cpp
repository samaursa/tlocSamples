#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocInput/tloc_input.h>
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

//------------------------------------------------------------------------
// KeyboardCallback

class KeyboardCallback
{
public:
  KeyboardCallback(core_cs::Entity* a_camera)
    : m_camera(a_camera)
  { }

  //------------------------------------------------------------------------
  // Called when a key is pressed. Currently will printf tloc's representation
  // of the key.
  bool OnKeyPress(const tl_size ,
                  const input_hid::KeyboardEvent& a_event)
  {
    math_cs::Transform* camTransform =
      m_camera->GetComponent<math_cs::Transform>();

    math_t::Mat3f32 mat = camTransform->GetOrientation();
    math_t::Vec3f32 newPos;

    if (a_event.m_keyCode == input_hid::KeyboardEvent::left)
    {
      math_t::Vec3f32 camLeft; mat.GetCol(0, camLeft);
      camLeft.Normalize();

      math_t::Vec3f32 newPos = camTransform->GetPosition() + camLeft;
      newPos.Normalize();
      newPos = newPos * camTransform->GetPosition().Length();
      camTransform->SetPosition(newPos);
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::right)
    {
      math_t::Vec3f32 camLeft; mat.GetCol(0, camLeft);
      camLeft.Negate();
      camLeft.Normalize();

      math_t::Vec3f32 newPos = camTransform->GetPosition() + camLeft;
      newPos.Normalize();
      newPos = newPos * camTransform->GetPosition().Length();
      camTransform->SetPosition(newPos);
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::up)
    {
      math_t::Vec3f32 camUp(0, 1, 0);
      camUp.Normalize();

      math_t::Vec3f32 newPos = camTransform->GetPosition() + camUp;
      newPos.Normalize();
      newPos = newPos * camTransform->GetPosition().Length();
      camTransform->SetPosition(newPos);
    }
    else if (a_event.m_keyCode == input_hid::KeyboardEvent::down)
    {
      math_t::Vec3f32 camUp(0, 1, 0);
      camUp.Negate();
      camUp.Normalize();

      math_t::Vec3f32 newPos = camTransform->GetPosition() + camUp;
      newPos.Normalize();
      newPos = newPos * camTransform->GetPosition().Length();
      camTransform->SetPosition(newPos);
    }

    math_t::Vec3f32 camDir = camTransform->GetPosition();
    camDir.Normalize();

    math_t::Vec3f32 camUp(0, 1, 0);
    math_t::Vec3f32 camLeft;
    camLeft.Cross(camUp, camDir);
    camUp.Cross(camDir, camLeft);

    mat.SetCol(0, camLeft);
    mat.SetCol(1, camUp);
    mat.SetCol(2, camDir);

    camTransform->SetOrientation(mat);

    return false;
  }

  //------------------------------------------------------------------------
  // Called when a key is released. Currently will printf tloc's representation
  // of the key.
  bool OnKeyRelease(const tl_size ,
                    const input_hid::KeyboardEvent& )
  {
    return false;
  }

  core_cs::Entity* m_camera;
};
TLOC_DEF_TYPE(KeyboardCallback);

int TLOC_MAIN(int argc, char *argv[])
{
  TLOC_UNUSED_2(argc, argv);

  gfx_win::Window win;
  WindowCallback  winCallback;

  win.Register(&winCallback);
  win.Create( gfx_win::Window::graphics_mode::Properties(500, 500),
    gfx_win::WindowSettings("tlocTexturedFan") );

  // -----------------------------------------------------------------------
  // Initialize renderer
  gfx_rend::Renderer  renderer;
  if (renderer.Initialize() != ErrorSuccess)
  { printf("\nRenderer failed to initialize"); return 1; }

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

  // Check pointers
  TLOC_ASSERT_NOT_NULL(keyboard);

  // -----------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_sptr  eventMgr(new core_cs::EventManager());
  core_cs::entity_manager_sptr entityMgr(new core_cs::EntityManager(eventMgr));

  // -----------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  core_cs::ComponentPoolManager cpoolMgr;

  // -----------------------------------------------------------------------
  // To render a mesh, we need a mesh render system - this is a specialized
  // system to render this primitive
  gfx_cs::MeshRenderSystem  meshSys(eventMgr, entityMgr);

  // -----------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem    matSys(eventMgr, entityMgr);

  // -----------------------------------------------------------------------
  // The camera's view transformations are calculated by the camera system
  gfx_cs::CameraSystem      camSys(eventMgr, entityMgr);

  // -----------------------------------------------------------------------
  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The fan render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.
  gfx_cs::Material  mat;
  {
#if defined (TLOC_OS_WIN)
    core_str::String shaderPath("/shaders/tlocTexturedMeshVS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPath("/shaders/tlocTexturedMeshVS_gl_es_2_0.glsl");
#endif

    shaderPath = GetAssetsPath() + shaderPath;
    core_io::FileIO_ReadA shaderFile(shaderPath.c_str());

    if (shaderFile.Open() != ErrorSuccess)
    { printf("\nUnable to open the vertex shader"); return 1;}

    core_str::String code;
    shaderFile.GetContents(code);
    mat.SetVertexSource(code);
  }
  {
#if defined (TLOC_OS_WIN)
    core_str::String shaderPath("/shaders/tlocTexturedMeshFS.glsl");
#elif defined (TLOC_OS_IPHONE)
    core_str::String shaderPath("/shaders/tlocTexturedMeshFS_gl_es_2_0.glsl");
#endif

    shaderPath = GetAssetsPath() + shaderPath;
    core_io::FileIO_ReadA shaderFile(shaderPath.c_str());

    if (shaderFile.Open() != ErrorSuccess)
    { printf("\nUnable to open the fragment shader"); return 1;}

    core_str::String code;
    shaderFile.GetContents(code);
    mat.SetFragmentSource(code);
  }

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
  gfx_gl::texture_object_sptr to(new gfx_gl::TextureObject());
  to->Initialize(png.GetImage());

  gfx_gl::uniform_sptr  u_to(new gfx_gl::Uniform());
  u_to->SetName("s_texture").SetValueAs(to);

  gfx_gl::shader_operator_sptr so =
    gfx_gl::shader_operator_sptr(new gfx_gl::ShaderOperator());
  so->AddUniform(u_to);

  // Finally, add the shader operator to the material
  mat.AddShaderOperator(so);

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

  core_cs::Entity* ent =
    prefab_gfx::CreateMesh(*entityMgr.get(), cpoolMgr, vertices);
  entityMgr->InsertComponent(ent, &mat);

  // -----------------------------------------------------------------------
  // Create a camera from the prefab library

  math_t::AspectRatio ar(math_t::AspectRatio::width( (tl_float)win.GetWidth()),
    math_t::AspectRatio::height( (tl_float)win.GetHeight()) );
  math_t::FOV fov(math_t::Degree(60.0f), ar, math_t::p_FOV::vertical());

  math_proj::FrustumPersp::Params params(fov);
  params.SetFar(100.0f).SetNear(1.0f);

  math_proj::FrustumPersp fr(params);
  fr.BuildFrustum();

  core_cs::Entity*
    m_cameraEnt = prefab_gfx::CreateCamera(*entityMgr.get(), cpoolMgr, fr,
                                          math_t::Vec3f(0.0f, 0.0f, 5.0f));

  meshSys.AttachCamera(m_cameraEnt);

  KeyboardCallback keyboardCallback(m_cameraEnt);
  keyboard->Register(&keyboardCallback);

  // -----------------------------------------------------------------------
  // All systems need to be initialized once

  meshSys.Initialize();
  matSys.Initialize();
  camSys.Initialize();

  // -----------------------------------------------------------------------
  // Main loop

  // Very important to enable depth testing
  glEnable(GL_DEPTH_TEST);
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    inputMgr->Update();

    camSys.ProcessActiveEntities();
    // Finally, process (render) the mesh
    meshSys.ProcessActiveEntities();

    win.SwapBuffers();
  }

  // -----------------------------------------------------------------------
  // Exiting
  printf("\nExiting normally");

  return 0;

}
