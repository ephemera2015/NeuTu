#ifndef Z3DCAMERAUTILS_H
#define Z3DCAMERAUTILS_H

#include "zpoint.h"
#include "z3dcamera.h"

namespace Z3DCameraUtils
{
  // Set the frustum to just enclose the bounding sphere.  The result is a tighter fit than
  // available with Z3DCamera::resetCamera(const ZBBox<glm::dvec3>& bound, ResetOption options),
  // whose input would be a box around the sphere, and which then would put asphere around the box.

  void resetCamera(ZPoint pos, double radius, Z3DCamera &camera);

  // Zoom the camera so that the view just shows the specified vertices.  Assumes that the camera
  // starts zoomed out so the all the vertices are visible.

  void tightenZoom(const std::vector<std::vector<glm::vec3>> &vertices,
                   Z3DCamera &camera);

};

#endif // Z3DCAMERAUTILS_H
