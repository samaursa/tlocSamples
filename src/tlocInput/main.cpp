#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl>

#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>
#include <tlocInput/tloc_input.h>

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

math_cs::Transform& GetEntityTransformComponent(const core_cs::Entity* a_ent)
{
  core_cs::ComponentMapper<math_cs::Transform> transformComponents =
    a_ent->GetComponents(math_cs::components::transform);

  return transformComponents[0];
}

void MoveEntity(const core_cs::Entity* a_ent, const math_t::Vec2f& a_deltaPos)
{
  math_cs::Transform& transform = GetEntityTransformComponent(a_ent);

  math_cs::Transform::position_type position(transform.GetPosition());
  position[0] += a_deltaPos[0];
  position[1] += a_deltaPos[1];

  transform.SetPosition(position);
}

void MoveEntityToPosition(const core_cs::Entity* a_ent, const math_t::Vec2f& a_position)
{
  math_cs::Transform& transform = GetEntityTransformComponent(a_ent);
  transform.SetPosition(a_position.ConvertTo<math_t::Vec3f>());
}

int main()
{
  gfx_win::Window win;
  WindowCallback  winCallback;
  win.Register(&winCallback);

  const tl_int winWidth = 800;
  const tl_int winHeight = 600;

  win.Create( gfx_win::Window::graphics_mode::Properties(winWidth, winHeight),
    gfx_win::WindowSettings("tlocInput") );

  //------------------------------------------------------------------------
  // Initialize renderer
  gfx_rend::Renderer renderer;
  if (renderer.Initialize() != ErrorSuccess())
  { printf("\nRenderer failed to initialize"); return 1; }

  //------------------------------------------------------------------------
  // Creating InputManager - This manager will handle all of our HIDs during
  // its lifetime. More than one InputManager can be instantiated.
  ParamList<HWND> params;
  params.m_param1 = win.GetWindowHandle();

  input::input_mgr_i_ptr inputMgr =
    input::input_mgr_i_ptr(new input::InputManagerI(params));

  //------------------------------------------------------------------------
  // Creating a keyboard and mouse HID
  input_hid::KeyboardI* keyboard = inputMgr->CreateHID<input_hid::KeyboardI>();
  input_hid::MouseI* mouse = inputMgr->CreateHID<input_hid::MouseI>();
  input_hid::TouchSurfaceI* touchSurface =
    inputMgr->CreateHID<input_hid::TouchSurfaceI>();
  TLOC_UNUSED(touchSurface);

  //------------------------------------------------------------------------
  // All systems in the engine require an event manager and an entity manager
  core_cs::event_manager_sptr  eventMgr(new core_cs::EventManager());
  core_cs::entity_manager_sptr entityMgr(new core_cs::EntityManager(eventMgr));

  //------------------------------------------------------------------------
  // A component pool manager manages all the components in a particular
  // session/level/section.
  core_cs::ComponentPoolManager compMgr;

  //------------------------------------------------------------------------
  // To render a quad, we need a quad render system - this is a specialized
  // system to render this primitive
  gfx_cs::QuadRenderSystem quadSys(eventMgr, entityMgr);

  //------------------------------------------------------------------------
  // We cannot render anything without materials and its system
  gfx_cs::MaterialSystem matSys(eventMgr, entityMgr);

  // We need a material to attach to our entity (which we have not yet created).
  // NOTE: The quad render system expects a few shader variables to be declared
  //       and used by the shader (i.e. not compiled out). See the listed
  //       vertex and fragment shaders for more info.
  gfx_cs::Material mat;
  {
    core_io::FileIO_ReadA file
      ("../../../../../assets/tlocPassthroughVertexShader.glsl");
    if (file.Open() != ErrorSuccess())
    { printf("\nUnable to open the vertex shader"); return 1; }

    core_str::String code;
    file.GetContents(code);
    mat.SetVertexSource(code);
  }

  {
    core_io::FileIO_ReadA file
      ("../../../../../assets/tlocPassthroughFragmentShader.glsl");
    if (file.Open() != ErrorSuccess())
    { printf("\nUnable to open the fragment shader"); return 1; }

    core_str::String code;
    file.GetContents(code);
    mat.SetFragmentSource(code);
  }

  //------------------------------------------------------------------------
  // The prefab library has some prefabricated entities for us
  math_t::Rectf32 rect(math_t::Rectf32::width(0.5f),
                       math_t::Rectf32::height(0.5f));
  core_cs::Entity* ent = prefab_gfx::CreateQuad(*entityMgr.get(), compMgr, rect);
  entityMgr->InsertComponent(ent, &mat);

  //------------------------------------------------------------------------
  // All systems need to be initialized once

  quadSys.Initialize();
  matSys.Initialize();

  //------------------------------------------------------------------------
  // In order to update at a pre-defined time interval, a timer must be created
  core_time::Timer32 renderFrameTime;
  renderFrameTime.Reset();
  core_time::Timer32 inputFrameTime;
  inputFrameTime.Reset();

  //------------------------------------------------------------------------
  // Main loop
  while (win.IsValid() && !winCallback.m_endProgram)
  {
    gfx_win::WindowEvent  evt;
    while (win.GetEvent(evt))
    { }

    //------------------------------------------------------------------------
    // Update Input
    if (win.IsValid() && inputFrameTime.ElapsedMilliSeconds() > 16)
    {
      inputFrameTime.Reset();

      // This updates all HIDs attached to the InputManager. An InputManager can
      // be updated at a different interval than the main render update.
      inputMgr->Update();

      // Polling if arrow keys are down and moving the entity with a helper
      // function
      const tl_float deltaPos = 0.01f;
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::left))
      {
        MoveEntity(ent, math_t::Vec2f(-deltaPos, 0.0f));
      }
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::right))
      {
        MoveEntity(ent, math_t::Vec2f(deltaPos, 0.0f));
      }
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::up))
      {
        MoveEntity(ent, math_t::Vec2f(0.0f, deltaPos));
      }
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::down))
      {
        MoveEntity(ent, math_t::Vec2f(0.0f, -deltaPos));
      }
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::escape))
      {
        exit(0);
      }
      if (keyboard->IsKeyDown(input_hid::KeyboardEvent::h))
      {
        printf("\nCAN I HAZ CHEEZEBURGERZZ");
      }

      // Polling the mouse's absolute position, using that value to move the quad
      if (mouse->IsButtonDown(input_hid::MouseEvent::left) &&
          mouse->IsButtonDown(input_hid::MouseEvent::right))
      {
        MoveEntityToPosition(ent, math_t::Vec2f(0.0f, 0.0f));
      }
      else if (mouse->IsButtonDown(input_hid::MouseEvent::left))
      {
        tl_float sensativityX =
          2.0f / core_utils::CastNumber<tl_float>(winWidth);
        tl_float sensativityY =
          -2.0f / core_utils::CastNumber<tl_float>(winHeight);

        input_hid::MouseEvent mouseState = mouse->GetState();
        MoveEntityToPosition(ent, math_t::Vec2f(mouseState.m_X.m_abs().Value() * sensativityX,
          mouseState.m_Y.m_abs().Value() * sensativityY ));
      }

      // The Immediate mode InputManager needs to be reset to reset all
      // key/button states to their default. E.g. If user pushes a key down the
      // key will remain "down" until the user resets the InputManager.
      inputMgr->Reset();
    }

    //------------------------------------------------------------------------
    // Update the QuadRenderSystem and Window
    if (win.IsValid() && renderFrameTime.ElapsedMilliSeconds() > 16)
    {
      renderFrameTime.Reset();

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      quadSys.ProcessActiveEntities();

      win.SwapBuffers();
    }
  }

  //------------------------------------------------------------------------
  // Exiting
  printf("\nExiting normally");

}
