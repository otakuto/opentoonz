

#include "tgl.h"

#include "tconvert.h"
#include "tofflinegl.h"
#include "trop.h"
#include "timage_io.h"
#include "tcurves.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QGLWidget>
#include <QPainter>
#include <QPen>

#ifndef __sgi
#ifdef _WIN32
#include <cstdlib>
#include <GL/glut.h>
#elif defined(LINUX)
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif
#endif

#if defined(MACOSX) || defined(LINUX)
#include <QGLContext>
#endif

//#include "tthread.h"

#undef SCALE_BY_GLU
//#undef NEW_DRAW_TEXT

//-----------------------------------------------------------------------------

namespace {

// GLUquadric*  localDisk=0;

/*
  Find the number of slices in function of radius size.
  \par radius of circle
  \par size of pixel
  \ret number of division to obtain a circle
  */
int computeSlices(double radius, double pixelSize = 1.0) {
  if (radius < 0) return 2;

  double thetaStep;
  double temp = pixelSize * 0.5 / radius;

  if (fabs(1.0 - temp) <= 1)
    thetaStep = acos(1.0 - temp);
  else
    thetaStep = M_PI_4;

  assert(thetaStep != 0.0);

  int numberOfSlices = (int)(M_2PI / thetaStep);

  return numberOfSlices != 0 ? numberOfSlices : 2;
}
}  // end of unnamed namespace

//-----------------------------------------------------------------------------

double tglGetPixelSize2() {
  double mat[16];
  glMatrixMode(GL_MODELVIEW);
  glGetDoublev(GL_MODELVIEW_MATRIX, mat);

  double det                      = fabs(mat[0] * mat[5] - mat[1] * mat[4]);
  if (det < TConsts::epsilon) det = TConsts::epsilon;
  return 1.0 / det;
}

//-----------------------------------------------------------------------------

void tglDrawText(const TPointD &p, const std::string &s) {
  tglDrawText(p, QString::fromStdString(s));
}

//-----------------------------------------------------------------------------

void tglDrawText(const TPointD &p, const QString &s) {
  static int devPixRatio = QApplication::desktop()->devicePixelRatio();
  QFont font("Arial", 18);
  font.setPixelSize(13 * devPixRatio);
  QFontMetrics fm(font);
  QRect textRect = fm.boundingRect(s);

  int mrg = 3 * devPixRatio;

  textRect.moveTo(10 * devPixRatio + mrg, 2 * devPixRatio + mrg);

  QSize size(textRect.width() + textRect.left() + mrg,
             textRect.bottom() + mrg + 3 * devPixRatio);

  QImage image(size.width(), size.height(), QImage::Format_ARGB32);
  image.fill(Qt::transparent);

  QPainter painter(&image);
  painter.setPen(Qt::black);
  painter.setFont(font);
  painter.drawText(textRect, Qt::TextDontClip, s);

  QImage texture = QGLWidget::convertToGLFormat(image);

  glRasterPos2d(p.x, p.y);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDrawPixels(texture.width(), texture.height(), GL_RGBA, GL_UNSIGNED_BYTE,
               texture.bits());
  glDisable(GL_BLEND);
}

//-----------------------------------------------------------------------------

void tglDrawSegment(const TPointD &p1, const TPointD &p2) {
  glBegin(GL_LINES);
  tglVertex(p1);
  tglVertex(p2);
  glEnd();
}

//-----------------------------------------------------------------------------

void tglDrawCircle(const TPointD &center, double radius) {
  if (radius <= 0) return;

  double pixelSize = 1;
  int slices       = 60;

  if (slices <= 0) slices = computeSlices(radius, pixelSize) >> 1;

  double step  = M_PI / slices;
  double step2 = 2.0 * step;

  double cos_t, sin_t, cos_ts, sin_ts, t;

  glPushMatrix();
  glTranslated(center.x, center.y, 0.0);
  glBegin(GL_LINES);

  cos_t = radius /* *1.0*/;
  sin_t = 0.0;
  for (t = 0; t + step < M_PI_2; t += step2) {
    cos_ts = radius * cos(t + step);
    sin_ts = radius * sin(t + step);

    glVertex2f(cos_t, sin_t);
    glVertex2f(cos_ts, sin_ts);

    glVertex2f(-cos_t, sin_t);
    glVertex2f(-cos_ts, sin_ts);

    glVertex2f(-cos_t, -sin_t);
    glVertex2f(-cos_ts, -sin_ts);

    glVertex2f(cos_t, -sin_t);
    glVertex2f(cos_ts, -sin_ts);

    cos_t = cos_ts;
    sin_t = sin_ts;
  }

  cos_ts = 0.0;
  sin_ts = radius /* *1.0*/;

  glVertex2f(cos_t, sin_t);
  glVertex2f(cos_ts, sin_ts);

  glVertex2f(-cos_t, sin_t);
  glVertex2f(-cos_ts, sin_ts);

  glVertex2f(-cos_t, -sin_t);
  glVertex2f(-cos_ts, -sin_ts);

  glVertex2f(cos_t, -sin_t);
  glVertex2f(cos_ts, -sin_ts);

  glEnd();
  glPopMatrix();
}

//-----------------------------------------------------------------------------

void tglDrawDisk(const TPointD &c, double r) {
  if (r <= 0) return;

  double pixelSize = 1;
  int slices       = 60;

  if (slices <= 0) slices = computeSlices(r, pixelSize) >> 1;

  glPushMatrix();
  glTranslated(c.x, c.y, 0.0);
  GLUquadric *quadric = gluNewQuadric();
  gluDisk(quadric, 0, r, slices, 1);
  gluDeleteQuadric(quadric);
  glPopMatrix();
}

//-----------------------------------------------------------------------------

void tglDrawRect(const TRectD &rect) {
  glBegin(GL_LINE_LOOP);
  tglVertex(rect.getP00());
  tglVertex(rect.getP10());
  tglVertex(rect.getP11());
  tglVertex(rect.getP01());
  glEnd();
}

//-----------------------------------------------------------------------------

void tglFillRect(const TRectD &rect) {
  glBegin(GL_POLYGON);
  tglVertex(rect.getP00());
  tglVertex(rect.getP10());
  tglVertex(rect.getP11());
  tglVertex(rect.getP01());
  glEnd();
}
//-----------------------------------------------------------------------------

void tglRgbOnlyColorMask() {
  tglMultColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
  tglEnableBlending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//-----------------------------------------------------------------------------

void tglAlphaOnlyColorMask() {
  tglMultColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
  tglEnableBlending(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

//-----------------------------------------------------------------------------

void tglEnableBlending(GLenum src, GLenum dst) {
  glEnable(GL_BLEND);
  glBlendFunc(src, dst);
}

//-----------------------------------------------------------------------------

void tglEnableLineSmooth(bool enable, double lineSize) {
  if (enable) {
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(lineSize);
  } else
    glDisable(GL_LINE_SMOOTH);
}

//-----------------------------------------------------------------------------

void tglEnablePointSmooth(double pointSize) {
  glEnable(GL_BLEND);
  glPointSize(pointSize);
}

//-----------------------------------------------------------------------------

void tglGetColorMask(GLboolean &red, GLboolean &green, GLboolean &blue,
                     GLboolean &alpha) {
  GLboolean channels[4];
  glGetBooleanv(GL_COLOR_WRITEMASK, &channels[0]);
  red = channels[0], green = channels[1], blue = channels[2],
  alpha = channels[3];
}

//-----------------------------------------------------------------------------

void tglMultColorMask(GLboolean red, GLboolean green, GLboolean blue,
                      GLboolean alpha) {
  GLboolean channels[4];
  glGetBooleanv(GL_COLOR_WRITEMASK, &channels[0]);
  glColorMask(red && channels[0], green && channels[1], blue && channels[2],
              alpha && channels[3]);
}

//============================================================================

void tglDraw(const TCubic &cubic, int precision, GLenum pointOrLine) {
  CHECK_ERRORS_BY_GL;
  assert(pointOrLine == GL_POINT || pointOrLine == GL_LINE);
  float ctrlPts[4][3];

  ctrlPts[0][0] = cubic.getP0().x;
  ctrlPts[0][1] = cubic.getP0().y;
  ctrlPts[0][2] = 0.0;

  ctrlPts[1][0] = cubic.getP1().x;
  ctrlPts[1][1] = cubic.getP1().y;
  ctrlPts[1][2] = 0.0;

  ctrlPts[2][0] = cubic.getP2().x;
  ctrlPts[2][1] = cubic.getP2().y;
  ctrlPts[2][2] = 0.0;

  ctrlPts[3][0] = cubic.getP3().x;
  ctrlPts[3][1] = cubic.getP3().y;
  ctrlPts[3][2] = 0.0;

  CHECK_ERRORS_BY_GL;
  glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 4, &ctrlPts[0][0]);
  CHECK_ERRORS_BY_GL;
  glEnable(GL_MAP1_VERTEX_3);
  CHECK_ERRORS_BY_GL;
  glMapGrid1f(precision, 0.0, 1.0);
  CHECK_ERRORS_BY_GL;
  glEvalMesh1(pointOrLine, 0, precision);
  CHECK_ERRORS_BY_GL;
}

//-----------------------------------------------------------------------------

void tglDraw(const TRectD &rect, const std::vector<TRaster32P> &textures,
             bool blending) {
  double pixelSize2 = tglGetPixelSize2();
  // level e' la minore potenza di 2 maggiore di sqrt(pixelSize2)
  unsigned int level = 1;
  while (pixelSize2 * level * level <= 1.0) level <<= 1;

  unsigned int texturesCount       = (int)textures.size();
  if (level > texturesCount) level = texturesCount;

  level = texturesCount - level;

  tglDraw(rect, textures[level], blending);
}

//-------------------------------------------------------------------

void tglDraw(const TRectD &rect, const TRaster32P &tex, bool blending) {
  CHECK_ERRORS_BY_GL;
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  if (blending) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  unsigned int texWidth  = 1;
  unsigned int texHeight = 1;

  while (texWidth < (unsigned int)tex->getLx()) texWidth = texWidth << 1;

  while (texHeight < (unsigned int)tex->getLy()) texHeight = texHeight << 1;

  double lwTex = 1.0;
  double lhTex = 1.0;

  TRaster32P texture;
  unsigned int texLx = (unsigned int)tex->getLx();
  unsigned int texLy = (unsigned int)tex->getLy();

  if (texWidth != texLx || texHeight != texLy) {
    texture = TRaster32P(texWidth, texHeight);
    texture->fill(TPixel32(0, 0, 0, 0));
    texture->copy(tex);
    lwTex                  = (texLx) / (double)(texWidth);
    lhTex                  = (texLy) / (double)(texHeight);
    if (lwTex > 1.0) lwTex = 1.0;
    if (lhTex > 1.0) lhTex = 1.0;
  } else
    texture = tex;
  GLenum fmt =
#if defined(TNZ_MACHINE_CHANNEL_ORDER_BGRM)
      GL_BGRA_EXT;
#elif defined(TNZ_MACHINE_CHANNEL_ORDER_MBGR)
      GL_ABGR_EXT;
#elif defined(TNZ_MACHINE_CHANNEL_ORDER_RGBM)
      GL_RGBA;
#elif defined(TNZ_MACHINE_CHANNEL_ORDER_MRGB)
      GL_BGRA;
#else
//   Error  PLATFORM NOT SUPPORTED
#error "unknown channel order!"
#endif

  // Generate a texture id and bind it.
  GLuint texId;
  glGenTextures(1, &texId);

  glBindTexture(GL_TEXTURE_2D, texId);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, texture->getWrap());

  texture->lock();
  glTexImage2D(GL_TEXTURE_2D, 0, 4, texWidth, texHeight, 0, fmt,
#ifdef TNZ_MACHINE_CHANNEL_ORDER_MRGB
               GL_UNSIGNED_INT_8_8_8_8_REV,
#else
               GL_UNSIGNED_BYTE,
#endif
               texture->getRawData());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glEnable(GL_TEXTURE_2D);

  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

  double rectLx = rect.getLx();
  double rectLy = rect.getLy();

  tglColor(TPixel32(0, 0, 0, 0));

  glPushMatrix();

  glTranslated(rect.x0, rect.y0, 0.0);
  glBegin(GL_POLYGON);

  glTexCoord2d(0, 0);
  tglVertex(TPointD(0.0, 0.0));

  glTexCoord2d(lwTex, 0);
  tglVertex(TPointD(rectLx, 0.0));

  glTexCoord2d(lwTex, lhTex);
  tglVertex(TPointD(rectLx, rectLy));

  glTexCoord2d(0, lhTex);
  tglVertex(TPointD(0.0, rectLy));

  glEnd();
  glDisable(GL_TEXTURE_2D);

  glPopMatrix();
  glPopAttrib();

  // Delete texture
  glDeleteTextures(1, &texId);

  texture->unlock();
}

//-----------------------------------------------------------------------------

void tglBuildMipmaps(std::vector<TRaster32P> &rasters,
                     const TFilePath &filepath) {
  assert(rasters.size() > 0);
  TRop::ResampleFilterType resampleFilter = TRop::ClosestPixel;
  TRasterP ras;
  TImageReader::load(filepath, ras);
  int rasLx = ras->getLx();
  int rasLy = ras->getLy();

  int lx = 1;
  while (lx < rasLx) lx <<= 1;

  int ly = 1;
  while (ly < rasLy) ly <<= 1;

  TRaster32P ras2(lx, ly);
  double sx = (double)lx / (double)ras->getLx();
  double sy = (double)ly / (double)ras->getLy();
#ifndef SCALE_BY_GLU
  TRop::resample(ras2, ras, TScale(sx, sy), resampleFilter);
#else
  ras->lock();
  gluScaleImage(GL_RGBA, ras->getLx(), ras->getLy(), GL_UNSIGNED_BYTE,
                ras->getRawData(), lx, ly, GL_UNSIGNED_BYTE,
                ras2->getRawData());
  ras->unlock();
#endif

  rasters[0] = ras2;
  int ras2Lx = ras2->getLx();
  int ras2Ly = ras2->getLy();
  for (int i = 1; i < (int)rasters.size(); ++i) {
    lx >>= 1;
    ly >>= 1;
    if (lx < 1) lx = 1;
    if (ly < 1) ly = 1;
    rasters[i]     = TRaster32P(lx, ly);
    sx             = (double)lx / (double)ras2Lx;
    sy             = (double)ly / (double)ras2Ly;
    rasters[i]     = TRaster32P(lx, ly);
#ifndef SCALE_BY_GLU
    TRop::resample(rasters[i], ras2, TScale(sx, sy), resampleFilter);
#else
    ras2->lock();
    gluScaleImage(GL_RGBA, ras->getLx(), ras->getLy(), GL_UNSIGNED_BYTE,
                  ras2->getRawData(), lx, ly, GL_UNSIGNED_BYTE,
                  rasters[i]->getRawData());
    ras2->unlock();
#endif
  }
}

//-----------------------------------------------------------------------------
// Forse si potrebbe togliere l'ifdef ed usare QT
#if defined(_WIN32)

TGlContext tglGetCurrentContext() {
  return std::make_pair(wglGetCurrentDC(), wglGetCurrentContext());
}

void tglMakeCurrent(TGlContext context) {
  wglMakeCurrent(context.first, context.second);
}

void tglDoneCurrent(TGlContext) { wglMakeCurrent(NULL, NULL); }

#elif defined(LINUX) || defined(__sgi) || defined(MACOSX)

TGlContext tglGetCurrentContext() {
  return reinterpret_cast<TGlContext>(
      const_cast<QGLContext *>(QGLContext::currentContext()));

  // (Daniele) I'm not sure why QGLContext::currentContext() returns
  // const. I think it shouldn't, and guess (hope) this is safe...
}

void tglMakeCurrent(TGlContext context) {
  if (context)
    reinterpret_cast<QGLContext *>(context)->makeCurrent();
  else
    tglDoneCurrent(tglGetCurrentContext());
}

void tglDoneCurrent(TGlContext context) {
  if (context) reinterpret_cast<QGLContext *>(context)->doneCurrent();
}

#else
#error "unknown platform!"
#endif

//-----------------------------------------------------------------------------
//  End Of File
//-----------------------------------------------------------------------------
