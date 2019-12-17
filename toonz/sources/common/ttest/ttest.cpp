

#include "ttest.h"
#include "tsystem.h"
#include "tfilepath_io.h"

#include "tlevel_io.h"
#include "tfx.h"
#include "tparam.h"
#include "tpalette.h"
#include "ttoonzimage.h"
#include "trasterimage.h"
#include "tstroke.h"
#include "tcolorstyles.h"

#include <set>
#include <sstream>

#if defined(LINUX)
#include <typeinfo>
#endif

namespace {

class TTestTable {
  typedef std::map<std::string, TTest *> Table;
  typedef std::set<std::string> SkipTable;
  Table m_table;
  SkipTable m_skipTable;
  TTestTable(){};

public:
  static TTestTable *table() {
    static TTestTable *theTable = 0;
    if (!theTable) theTable     = new TTestTable;
    return theTable;
  }

  void add(const std::string &str, TTest *test) {
    assert(test);
    if (m_table.find(str) != m_table.end()) {
      std::cout << "*error* Duplicate test name '" << str << "'" << std::endl;
      assert(0);
      return;
    }
    m_table[str] = test;
  }

  void skip(const std::string &str) { m_skipTable.insert(str); }

  void run(const std::string &str) {
    if (m_skipTable.find(str) != m_skipTable.end()) return;
    Table::iterator it = m_table.find(str);
    if (it == m_table.end())
      std::cout << "*error* test '" << str << "' not found" << std::endl;
    else {
      std::cout << "\nVerifying " << str << " ... " << std::endl;
      it->second->before();
      it->second->test();
      it->second->after();
      std::cout << "OK" << std::endl;
    }
  }

  void runAll() {
    int count = 0;
    std::cout << "Running all tests:" << std::endl;
    Table::iterator it;
    for (it = m_table.begin(); it != m_table.end(); ++it) {
      run(it->first);
      ++count;
    }
    std::cout << count << " test(s) launched" << std::endl;
  }

  void runType(std::string type) {
    int count = 0;
    std::cout << "Running " << type << " tests:" << std::endl;
    Table::iterator it;
    for (it = m_table.begin(); it != m_table.end(); ++it) {
      std::string str(it->first);
      int i = str.find("_");
      if (i <= 0) continue;
      std::string subStr(str, 0, i);
      if (subStr == type) run(it->first);
      ++count;
    }
    std::cout << count << " test(s) launched" << std::endl;
  }
};
}

//------------------------------------------------------------

TTest::TTest(const std::string &testName) {
  TTestTable::table()->add(testName, this);
}

//------------------------------------------------------------

TTest::~TTest() {}

//------------------------------------------------------------

void TTest::setInstanceCount() {
  m_levelInstanceCount       = TLevel::getInstanceCount();
  m_imageInstanceCount       = TImage::getInstanceCount();
  m_rasterInstanceCount      = TRaster::getInstanceCount();
  m_imageReaderInstanceCount = TImageReader::getInstanceCount();
  m_imageWriterInstanceCount = TImageWriter::getInstanceCount();
  m_levelReaderInstanceCount = TLevelReader::getInstanceCount();
  m_levelWriterInstanceCount = TLevelWriter::getInstanceCount();
  m_paramInstanceCount       = TParam::getInstanceCount();
  m_fxInstanceCount          = TFx::getInstanceCount();
}

//------------------------------------------------------------

void TTest::verifyInstanceCount() {
  assert(m_levelInstanceCount == TLevel::getInstanceCount());
  assert(m_imageInstanceCount == TImage::getInstanceCount());

  // assert(m_rasterInstanceCount==TRaster::getInstanceCount());
  // commentata da gmt: in qualche test viene creata un
  // offline gl context con annesso raster che ovviamente non
  // viene liberato.

  assert(m_imageReaderInstanceCount == TImageReader::getInstanceCount());
  assert(m_imageWriterInstanceCount == TImageWriter::getInstanceCount());
  assert(m_levelReaderInstanceCount == TLevelReader::getInstanceCount());
  assert(m_levelWriterInstanceCount == TLevelWriter::getInstanceCount());
  assert(m_paramInstanceCount == TParam::getInstanceCount());
  assert(m_fxInstanceCount == TFx::getInstanceCount());
}

//------------------------------------------------------------

void TTest::runTests(std::string name) {
  TFilePath testFile = getTestFile(name);

  std::vector<std::string> testType;
  if (name == "verif_image") {
    testType.push_back("write");
    testType.push_back("read");
  }
  Tifstream is(testFile);
  if (!is) {
    std::cout << "*error* test file '" << testFile << "' not found"
              << std::endl;
    return;
  }
  std::cout << "Test file : '" << testFile << "'" << std::endl;
  char buffer[1024];
  while (is.getline(buffer, sizeof(buffer))) {
    std::istringstream ss(buffer);
    while (ss) {
      std::string s;
      ss >> s;
      if (s == "") continue;
      if (s[0] == '#' || s[0] == '!') break;
      // Verifica tutti i test
      if (s == "all") {
        TTestTable::table()->runAll();
        continue;
      }
      // Verifica una parte dei test che ha come nome iniziale testType[i]
      int i;
      bool isTypeFound = false;
      for (i = 0; i < (int)testType.size(); i++)
        if (s == testType[i]) {
          isTypeFound = true;
          TTestTable::table()->runType(testType[i]);
          break;
        }
      if (isTypeFound) continue;
      if (s[0] == '~')
        TTestTable::table()->skip(s.substr(1));
      else  // Verifica il test di nome s
        TTestTable::table()->run(s);
    }
  }
}

//================================================================================
// Utility
//--------------------------------------------------------------------------------

TFilePath getTestFile(std::string name) {
  TFilePath testFile;

  TFilePath parentDir = TSystem::getBinDir().getParentDir();
#ifndef _WIN32
  parentDir = parentDir.getParentDir();
#endif

  TFilePath relativePath;
#ifndef NDEBUG
  relativePath = parentDir + TFilePath("projects\\test\\") + TFilePath(name);
#endif
  if (name == "verify_tnzcore")
    testFile = relativePath + TFilePath(name).withType("txt");
  else if (name == "verify_toonzlib")
    testFile = relativePath + TFilePath(name).withType("txt");
  else if (name == "verify_image")
    testFile = relativePath + TFilePath(name).withType("txt");
  else
    testFile = parentDir + TFilePath(name).withType("txt");

  return testFile;
}

//--------------------------------------------------------------------------------

/*!
*  Return true if \b ra raster and \b rb raster are equal, with an error
*  less than \b err; return false otherwise.
*  To verify rasters parity check pixel raster; if rasters are different is
*	 showed an error message.
*
*  \b err max variance allowed to value pixels.
*
*  OSS.: Usually \b ra is check image and \b rb reference image.
*/
int areEqual(TRasterP ra, TRasterP rb, double err) {
  if (!ra && !rb) return true;
  int y, x;
  if (typeid(ra) != typeid(rb)) return false;

  if (!ra || !rb) return false;
  if (ra->getSize() != rb->getSize()) return false;
  const int rowSize = ra->getRowSize();

  TRaster32P ra32     = ra;
  TRaster64P ra64     = ra;
  TRasterGR8P raGR8   = ra;
  TRasterCM32P raCM32 = ra;

  if (ra32) {
    TRaster32P rb32 = rb;
    if (!rb32) return 2;
    for (y = 0; y < ra->getLy(); y++)
      for (x = 0; x < ra->getLx(); x++) {
        TPixel32 *apix = (TPixel32 *)ra->getRawData(x, y);
        TPixel32 *bpix = (TPixel32 *)rb->getRawData(x, y);
        int rA         = apix->r;
        int gA         = apix->g;
        int bA         = apix->b;
        int mA         = apix->m;

        int rB = bpix->r;
        int gB = bpix->g;
        int bB = bpix->b;
        int mB = bpix->m;
        if (!areAlmostEqual(double(rA), double(rB), err) ||
            !areAlmostEqual(double(gA), double(gB), err) ||
            !areAlmostEqual(double(bA), double(bB), err) ||
            !areAlmostEqual(double(mA), double(mB), err)) {
          std::cout << "MISMATCH: x=" << x << " y=" << y << ". Pixel a = ("
                    << (int)rA << "," << (int)gA << "," << (int)bA << ","
                    << (int)mA << ")"
                    << ". Pixel b = (" << (int)rB << "," << (int)gB << ","
                    << (int)bB << "," << (int)mB << ")" << std::endl;
          return false;
        }
      }
  } else if (ra64) {
    TRaster64P rb64 = rb;
    if (!rb64) return 2;
    for (y = 0; y < ra->getLy(); y++)
      for (x = 0; x < ra->getLx(); x++) {
        TPixel64 *apix = (TPixel64 *)ra->getRawData(x, y);
        TPixel64 *bpix = (TPixel64 *)rb->getRawData(x, y);
        int rA         = apix->r;
        int gA         = apix->g;
        int bA         = apix->b;
        int mA         = apix->m;

        int rB = bpix->r;
        int gB = bpix->g;
        int bB = bpix->b;
        int mB = bpix->m;
        if (!areAlmostEqual(double(rA), double(rB), err) ||
            !areAlmostEqual(double(gA), double(gB), err) ||
            !areAlmostEqual(double(bA), double(bB), err) ||
            !areAlmostEqual(double(mA), double(mB), err)) {
          std::cout << "MISMATCH: x=" << x << " y=" << y << ". Pixel a = ("
                    << (int)rA << "," << (int)gA << "," << (int)bA << ","
                    << (int)mA << ")"
                    << ". Pixel b = (" << (int)rB << "," << (int)gB << ","
                    << (int)bB << "," << (int)mB << ")" << std::endl;
          return false;
        }
      }
  } else if (raGR8) {
    TRasterGR8P rbGR8 = rb;
    if (!rbGR8) return 2;
    for (y = 0; y < ra->getLy(); y++)
      for (x = 0; x < ra->getLx(); x++) {
        TPixelGR8 *apix = (TPixelGR8 *)ra->getRawData(x, y);
        TPixelGR8 *bpix = (TPixelGR8 *)rb->getRawData(x, y);
        int rA          = apix->value;
        int rB          = bpix->value;
        if (!areAlmostEqual(double(rA), double(rB), err)) {
          std::cout << "MISMATCH: x=" << x << " y=" << y << ". Pixel a = ("
                    << (int)rA << ")"
                    << ". Pixel b = (" << (int)rB << ")" << std::endl;
          return false;
        }
      }
  } else if (raCM32) {
    TRasterCM32P rbCM32 = rb;
    if (!rbCM32) return 2;
    for (y = 0; y < ra->getLy(); y++)
      for (x = 0; x < ra->getLx(); x++) {
        TPixelCM32 *apix = (TPixelCM32 *)ra->getRawData(x, y);
        TPixelCM32 *bpix = (TPixelCM32 *)rb->getRawData(x, y);
        int rAink        = apix->getInk();
        int rAtone       = apix->getTone();
        int rAPaint      = apix->getPaint();

        int rBink   = bpix->getInk();
        int rBtone  = bpix->getTone();
        int rBPaint = bpix->getPaint();

        if (!areAlmostEqual(double(rAink), double(rBink), err) ||
            !areAlmostEqual(double(rAtone), double(rBtone), err) ||
            !areAlmostEqual(double(rAPaint), double(rBPaint), err)) {
          std::cout << "MISMATCH: x=" << x << " y=" << y
                    << ". Pixel a = (Ink: " << (int)rAink
                    << ",Tone: " << (int)rAtone << ",Paint: " << (int)rBPaint
                    << ")"
                    << ". Pixel b = (Ink: " << (int)rBink
                    << ",Tone: " << (int)rBtone << ",Paint: " << (int)rBPaint
                    << ")" << std::endl;
          return false;
        }
      }
  } else
    return 2;
  return true;
}

//-----------------------------------------------------------------------------
/*!
*  Return true if \b va vector image and \b vb vector image are equal, with an
*	 error less than \b err; return false otherwise.
*  To verify vector image parity check stroke count, control point count and
*	 control point position; if vector image are different is showed an
*error message.
*
*  \b err max percentage variance, related to save box, allowed to control point
*					position.
*
*  OSS.: Usually \b va is check image and \b vb reference image.
*/
int areEqual(TVectorImageP va, TVectorImageP vb, double err) {
  if (!va && !vb) return true;
  int aStrokeCount = va->getStrokeCount();
  int bStrokeCount = vb->getStrokeCount();
  if (aStrokeCount != bStrokeCount) {
    std::cout << "MISMATCH: image stroke count = " << aStrokeCount
              << ". Reference image stroke count = " << bStrokeCount << "."
              << std::endl;
    return false;
  }

  TRectD aRect = va->getBBox();
  double lxErr = 0.00001;
  double lyErr = 0.00001;
  if (err != 0) {
    lxErr = (aRect.getLx() * err) * 0.01;
    lyErr = (aRect.getLy() * err) * 0.01;
  }
  TRectD bRect = vb->getBBox();
  if (!areAlmostEqual(bRect.getLx(), aRect.getLx(), lxErr) ||
      !areAlmostEqual(bRect.getLy(), aRect.getLy(), lyErr)) {
    std::cout << "MISMATCH: different save box rect." << std::endl;
    return false;
  }

  int i;
  for (i = 0; i < aStrokeCount; i++) {
    TStroke *aStroke = va->getStroke(i);
    TStroke *bStroke = vb->getStroke(i);
    int aStrCPCount  = aStroke->getControlPointCount();
    int bStrCPCount  = bStroke->getControlPointCount();
    if (aStrCPCount != bStrCPCount) {
      std::cout << "MISMATCH: image stroke " << i
                << "_mo control point count = " << aStrCPCount
                << ". Reference image stroke " << i
                << "_mo control point count = " << bStrCPCount << "."
                << std::endl;
      return false;
    }

    int j;
    for (j = 0; j <= aStrCPCount; j++) {
      TThickPoint aControlPoint = aStroke->getControlPoint(j);
      TThickPoint bControlPoint = bStroke->getControlPoint(j);
      if (!areAlmostEqual(aControlPoint.x, bControlPoint.x, lxErr)) {
        std::cout << "MISMATCH: different control point x position."
                  << std::endl;
        return false;
      }
      if (!areAlmostEqual(aControlPoint.y, bControlPoint.y, lyErr)) {
        std::cout << "MISMATCH: different control point y position."
                  << std::endl;
        return false;
      }
      if (!areAlmostEqual(aControlPoint.thick, bControlPoint.thick)) {
        std::cout << "MISMATCH: different control point thickness."
                  << std::endl;
        return false;
      }
    }
  }
  /* Non controllo i punti di controllo!!!
  int aStrCPCount = aStroke->getControlPointCount();
  int bStrCPCount = bStroke->getControlPointCount();
  if(aStrCPCount != bStrCPCount)
  {
          cout << "MISMATCH: image stroke " << i << "_mo control point count = "
  << aStrCPCount
                           << ". Reference image stroke " << i << "_mo control
  point count = " << bStrCPCount
                           << "." << endl;
          return false;
  }

  int j;
  for(j=0; j<=aStrCPCount; j++)
  {
          TThickPoint aControlPoint = aStroke->getControlPoint(j);
          TThickPoint bControlPoint = bStroke->getControlPoint(j);
          if(!areAlmostEqual(aControlPoint.x, bControlPoint.x, lxErr))
          {
                  cout << "MISMATCH: different control point x position." <<
  endl;
                  return false;
          }
          if(!areAlmostEqual(aControlPoint.y, bControlPoint.y, lyErr))
          {
                  cout << "MISMATCH: different control point y position." <<
  endl;
                  return false;
          }
          if(!areAlmostEqual(aControlPoint.thick, bControlPoint.thick))
          {
                  cout << "MISMATCH: different control point thickness." <<
  endl;
                  return false;
          }
  }*/
  return true;
}

//-----------------------------------------------------------------------------
/*!
*  Return true if images \b a and \b b are equal, with an error
*  less than \b err; return false otherwise.
*
*  If images are rasters recall
*  \b bool areEqual(TRasterP ra, TRasterP rb)
*  else if image are vectorImage recall
*  \b bool areEqual(TVectorImageP ra, TVectorImageP rb)
*/
int areEqual(TImageP a, TImageP b, double err) {
  if (!a && !b) return true;
  if (!a || !b) return false;
  if (a->getType() != b->getType()) return false;

  TRasterImageP ra(a);
  TRasterImageP rb(b);
  if ((!ra && rb) || (ra && !rb)) return false;
  if (ra && rb) return areEqual(ra->getRaster(), rb->getRaster(), err);

  TToonzImageP ta(a);
  TToonzImageP tb(b);
  if ((!ta && tb) || (ta && !tb)) return false;
  if (ta && tb) return areEqual(ta->getRaster(), tb->getRaster(), err);

  TVectorImageP va(a);
  TVectorImageP vb(b);
  if (va && vb) return areEqual(va, vb, err);

  return false;
}

//-----------------------------------------------------------------------------

bool areEqual(const TPalette *paletteA, const TPalette *paletteB) {
  if (paletteA->getStyleCount() != paletteB->getStyleCount() ||
      paletteA->getPageCount() != paletteB->getPageCount()) {
    std::cout << "PALETTE style count MISMATCH" << std::endl;
    return false;
  }
  int i;
  for (i = 0; i < paletteA->getStyleCount(); i++) {
    TColorStyle *styleA = paletteA->getStyle(i);
    TColorStyle *styleB = paletteB->getStyle(i);
    if (styleA->getMainColor() == styleB->getMainColor()) continue;
    std::cout << "PALETTE style MISMATCH" << std::endl;
    return false;
  }
  return true;
}

//--------------------------------------------------------------------------------

bool areEqual(TLevelP la, TLevelP lb) {
  if (la->getFrameCount() != lb->getFrameCount()) {
    std::cout << "FRAME COUNT MISMATCH" << std::endl;
    return false;
  }
  if (!areEqual(la->getPalette(), lb->getPalette())) return false;

  TLevel::Iterator ait = la->begin(), bit = lb->begin();
  for (; ait != la->end(); ait++, bit++) {
    assert(bit != lb->end());
    if (!areEqual(ait->second, bit->second)) return false;
  }
  return true;
}
