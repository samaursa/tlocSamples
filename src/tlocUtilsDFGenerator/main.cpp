#include <tlocCore/tloc_core.h>
#include <tlocCore/tloc_core.inl.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocMath/tloc_math.inl.h>
#include <tlocPrefab/tloc_prefab.h>
#include <3rdParty/Core/CL/include/optionparser.h>

#include <gameAssetsPath.h>

#include <tlocCore/smart_ptr/tloc_smart_ptr.inl.h>
#include <tlocCore/containers/tlocArray.inl.h>

#include <omp.h>

using namespace tloc;

namespace {

  core_io::Path g_inFile("invalid_filename");
  core_io::Path g_outFile("sdf_out.png");

  tl_int g_numOpenMPThreads = 4;
};

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

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

gfx_t::Color
EncodeDistanceToColor(tl_float a_distance)
{
  return gfx_t::Color(a_distance, 0.0f, 0.0f, 1.0f);
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

gfx_t::Color
GetAverageColorFromImg(const gfx_med::image_sptr& a_charImg, tl_int currRow, tl_int currCol,
tl_float widthRatio, tl_float heightRatio)
{
  const tl_float startingRow = core::Clamp(currRow - ( widthRatio - 1.0f ), 0.0f, (tl_float) a_charImg->GetWidth());
  const tl_float startingCol = core::Clamp(currCol - ( heightRatio - 1.0f ), 0.0f, (tl_float) a_charImg->GetHeight());

  const tl_float endingRow = core::Clamp(currRow + ( widthRatio - 1.0f ), 0.0f, (tl_float) a_charImg->GetWidth());
  const tl_float endingCol = core::Clamp(currCol + ( heightRatio - 1.0f ), 0.0f, (tl_float) a_charImg->GetHeight());

  tl_int avgColor = a_charImg->GetPixel(currRow, currCol)[0];

  tl_int avgCount = 0;

  for (tl_int row = (tl_int)startingRow; row < (tl_int)endingRow; ++row)
  {
    for (tl_int col = (tl_int)startingCol; col < (tl_int)endingCol; ++col)
    {
      if (row == currRow && col == currCol)
      { continue; }

      avgColor += a_charImg->GetPixel(row, col)[0];
      avgCount++;
    }
  }

  if (avgCount > 0)
  { avgColor = avgColor / (tl_int) ( avgCount + 1 ); }

  auto avgColorU8 = core_utils::CastNumber<u8>(avgColor);

  return gfx_t::Color(avgColorU8, avgColorU8, avgColorU8, avgColorU8);
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

gfx_med::image_sptr
GetSDFFromCharImage(gfx_med::image_sptr a_charImg, gfx_t::Dimension2 a_sdfDim, 
                    tl_int a_kernelSize, bool a_invert = false,
                    gfx_t::Color a_inCol = gfx_t::Color(240, 240, 240, 255), 
                    gfx_t::Color a_outCol = gfx_t::Color(0, 0, 0, 255))
{
  const tl_int kernelSize = a_kernelSize;

  auto sdfImg = core_sptr::MakeShared<gfx_med::Image>();
  sdfImg->Create(a_sdfDim, gfx_t::Color::COLOR_BLACK);

  const auto imgWidth = core_utils::CastNumber<tl_int>(a_charImg->GetWidth());
  const auto imgHeight = core_utils::CastNumber<tl_int>(a_charImg->GetHeight());

  const auto sdfImgWidth = core_utils::CastNumber<tl_int>(sdfImg->GetWidth());
  const auto sdfImgHeight = core_utils::CastNumber<tl_int>(sdfImg->GetHeight());

  const auto widthRatio = (tl_float) imgWidth / (tl_float) sdfImgWidth;
  const auto heightRatio = (tl_float) imgHeight / (tl_float) sdfImgHeight;

  const auto outColor = a_outCol;
  const auto inColor = a_inCol;

  printf("\n0%%|                                                                                                    |100%%");

  tl_int dashCount = 0;

#pragma omp parallel for shared(dashCount) num_threads(g_numOpenMPThreads)
  for (tl_int row = 0; row < sdfImgWidth; row++)
  {
    for (tl_int col = 0; col < sdfImgHeight; col++)
    {
#pragma omp atomic
      dashCount++;

      const auto imgRow = (tl_int) ( (tl_float) row * widthRatio );
      const auto imgCol = (tl_int) ( (tl_float) col * heightRatio );
      auto currCol = GetAverageColorFromImg(a_charImg, imgRow, imgCol, widthRatio, heightRatio);

      if (a_invert) { currCol = gfx_t::Color::COLOR_WHITE - currCol; }

      const bool isInColor = currCol[0] >= inColor[0];

      const auto kernelSizef32 = (tl_float)kernelSize;
      auto  disToEdge = kernelSizef32;

      auto  vecToPixel = isInColor ? math_t::Vec2f32(-kernelSizef32) : math_t::Vec2f32(kernelSizef32);
      bool  disFromInside = false;

      for (tl_int kRow = -kernelSize; kRow <= kernelSize; ++kRow)
      {
        for (tl_int kCol = -kernelSize; kCol <= kernelSize; ++kCol)
        {
          if (imgRow + kRow < 0 || imgCol + kCol < 0) { continue; }
          if (imgRow + kRow >= imgWidth || imgCol + kCol >= imgHeight) { continue; }
          //if (kRow == 0 && kCol == 0) { continue; }

          const auto destRow = core::Clamp(imgRow + kRow, 0, imgWidth - 1);
          const auto destCol = core::Clamp(imgCol + kCol, 0, imgHeight - 1);
          const auto destColor = GetAverageColorFromImg(a_charImg, destRow, destCol, widthRatio, heightRatio);

          const auto xDisInPixels = math::Abs(kRow);
          const auto yDisInPixels = math::Abs(kCol);

          using math_utils::Pythagoras;
          auto p = Pythagoras(Pythagoras::base((tl_float) xDisInPixels),
                              Pythagoras::opposite((tl_float) yDisInPixels));
          auto eucDis = p.GetSide<Pythagoras::hypotenuse>();

          // we want a circular kernel
          if (eucDis > kernelSize)
          {
            //eucDis = kernelSize;
            continue;
          }

          // if currCol is white (inside of the font color)
          if (isInColor)
          {
            disFromInside = true;

            // if the destination color is NOT white
            if (destColor[0] < inColor[0])
            { 
              if (eucDis < disToEdge)
              {
                vecToPixel[0] = (tl_float) kRow;
                vecToPixel[1] = (tl_float) kCol;
              }
              disToEdge = core::tlMin(eucDis, disToEdge);
            }
          }
          else // we consider anything not pure white to be black
          {
            disFromInside = false;

            // if the destination color is white
            if (destColor[0] >= inColor[0])
            { 
              if (eucDis < disToEdge)
              {
                vecToPixel[0] = (tl_float) kRow;
                vecToPixel[1] = (tl_float) kCol;
              }
              disToEdge = core::tlMin(eucDis, disToEdge);
            }
          }
        }
      }

      using namespace math;

      disToEdge = disFromInside ? -disToEdge : disToEdge;
      auto vec = math_t::Vec4f32(vecToPixel, disToEdge, kernelSizef32);
      auto sdfColor = gfx_t::f_color::Encode(vec, MakeRangef<f32, p_range::Inclusive>().Get(-kernelSizef32, kernelSizef32));

      sdfImg->SetPixel(row, col, gfx_t::Color(sdfColor));
    }

#pragma omp critical
    {
      const f32 total = (f32)sdfImgWidth * (f32)sdfImgHeight;
      const tl_int numDashes = (tl_int) ( (f32) dashCount * 100.0f / total );

      core_str::String dashes;
      for (tl_int i = 0; i < numDashes; ++i)
      { dashes += "-"; }

      for (tl_int i = numDashes; i < 100; ++i)
      { dashes += " "; }

      printf("\r0%%|%s|100%%", dashes.c_str());
    }
  }

  return sdfImg;
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

struct Arg : public option::Arg
{
  static void printError(const char* msg1, const option::Option& opt, const char* msg2)
  { TLOC_LOG_DEFAULT_ERR_NO_FILENAME() << msg1 << opt.name << msg2; }

  static option::ArgStatus Unknown(const option::Option& option, bool msg)
  {
    if (msg) printError("Unknown option '", option, "'");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Required(const option::Option& option, bool msg)
  {
    if (option.arg != 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires an argument");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
  {
    if (option.arg != 0 && option.arg[0] != 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires a non-empty argument");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Numeric(const option::Option& option, bool msg)
  {
    char* endptr = 0;
    if (option.arg != 0 && strtol(option.arg, &endptr, 10)) { };
    if (endptr != option.arg && *endptr == 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires a numeric argument");
    return option::ARG_ILLEGAL;
  }
};

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

#define TLOC_COMMAND_LINE_SKIP_PROGRAM_NAME(_argc_, _argv_)\
  _argc_ = _argc_ > 0 ? --_argc_ : _argc_;\
  _argv_ = _argc_ > 0 ? ++_argv_ : _argv_

enum optionIndex { UNKNOWN = 0, HELP, THREADS, IN_FILE, OUT_FILE, INV_COL, SAFE, SDF_WIDTH, KERNEL_SIZE};
const option::Descriptor usage[] = 
{
  { UNKNOWN, 0, "", ""           , Arg::Unknown   , "\nUSAGE: tlocUtilsDFGenerator [options]\n\n"
                                                    "Options:" },
  { HELP, 0, "", "help"          , Arg::None      , "  \t--help  \tPrint usage and exit." },
  { THREADS, 0, "", "threads"    , Arg::Numeric   , "  \t--threads \tNumber of OpenMP threads to use."},
  { IN_FILE, 0, "i", "input"     , Arg::Required  , "  -i <filename>, \t--input=<filename> \tInput file for DF generation." },
  { INV_COL, 0, "n", "negate"    , Arg::None      , "  -n \tInverses the incoming image." },
  { OUT_FILE, 0, "o", "output"   , Arg::Required  , "  -o <filename>, \t--output=<filename> \tOutput file with DF (PNG)." },
  { SAFE, 0, "s", "safe"         , Arg::None      , "  -s, \tDisallows overwriting existing files." },
  { SDF_WIDTH, 0, "w", "width"   , Arg::Numeric   , "  -w, \tWidth of the final SDF image. Height is calculated from ratio of source image." },
  { KERNEL_SIZE, 0, "k", "kernel", Arg::Numeric   , "  -k, \tDF is calculated upto this many pixels (in radius) from the current pixel." },
  { 0, 0, 0, 0, 0, 0 }
};

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

int TLOC_MAIN(int argc, char *argv[])
{
  core::memory::tracking::DoDisableTracking();

  TLOC_COMMAND_LINE_SKIP_PROGRAM_NAME(argc, argv);
  option::Stats   stats(usage, argc, argv);
  option::Option* options = new option::Option[stats.options_max];
  option::Option* buffer  = new option::Option[stats.buffer_max];
  option::Parser  parse(usage, argc, argv, options, buffer);

  if (parse.error()) 
  { 
    option::printUsage(TLOC_LOG_DEFAULT_INFO_NO_FILENAME(), usage);
    return 1;
  }

  if (argc == 0 || options[HELP] || options[UNKNOWN])
  {
    option::printUsage(TLOC_LOG_DEFAULT_INFO_NO_FILENAME(), usage);
    return 0;
  }

  if (options[THREADS])
  { g_numOpenMPThreads = atoi(options[THREADS].arg); }

  if (options[IN_FILE])
  {
    auto opt = options[IN_FILE].arg;
    g_inFile = core_io::Path(core_str::String(opt));

    if (g_inFile.FileExists() == false)
    {
      TLOC_LOG_DEFAULT_ERR_NO_FILENAME() << "File " << g_inFile << " does not exist";
      return 1;
    }
  }

  if (options[OUT_FILE])
  {
    auto opt = options[OUT_FILE].arg;
    g_outFile = core_io::Path(core_str::String(opt));

    if (g_outFile.FileExists() && options[SAFE])
    {
      TLOC_LOG_DEFAULT_ERR_NO_FILENAME() << "File " << g_outFile << " already exists.";
      return 1;
    }
  }
  else
  { g_outFile = core_io::Path(core_str::Format("%s_sdf.png", g_inFile.GetFileNameWithoutExtension().c_str()) ); }

  // load the image
  gfx_med::ImageLoaderPng il;
  if (il.Load(g_inFile) == ErrorFailure)
  {
    TLOC_LOG_DEFAULT_ERR_NO_FILENAME() << "Could not load " << g_inFile;
    return 1;
  }

  auto_cref imgPtr = il.GetImage();

  auto sdfDim = imgPtr->GetDimensions(); 
  if (options[SDF_WIDTH])
  {
    auto width = atoi(options[SDF_WIDTH].arg);
    sdfDim = 
      gfx_t::f_dimension::ModifyAndKeepRatioX(imgPtr->GetDimensions(), width);
  }

  tl_int kernelSize = 10;
  if (options[KERNEL_SIZE])
  { kernelSize = atoi(options[KERNEL_SIZE].arg); }

  TLOC_LOG_DEFAULT_INFO_NO_FILENAME() << "Generating SDF image from " 
    << g_inFile << " and saving to " << g_outFile << " with dimensions " << sdfDim;

  bool invert = options[INV_COL] != nullptr;

  core_time::Timer sdfTimer;
  auto_cref outImg = GetSDFFromCharImage(imgPtr, sdfDim, kernelSize, invert);
  printf("\nTime to calculate SDF: %f sec", sdfTimer.ElapsedSeconds());

  gfx_med::f_image_loader::SaveImage(*outImg, g_outFile);

  return 0;
}
