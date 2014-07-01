#ifndef ZINTPOINT_H
#define ZINTPOINT_H

#include <vector>

/*!
 * \brief The class of 3D points with integer coordinates
 */
class ZIntPoint
{
public:
  ZIntPoint();
  ZIntPoint(int x, int y, int z);

  inline int getX() const { return m_x; }
  inline int getY() const { return m_y; }
  inline int getZ() const { return m_z; }

  void set(int x, int y, int z);
  /*!
   * \brief Set a point from an array.
   *
   * Nothing is done if the size of \a pt is not 3.
   */
  void set(const std::vector<int> &pt);

  inline void setX(int x) { m_x = x; }
  inline void setY(int y) { m_y = y; }
  inline void setZ(int z) { m_z = z; }

  const int& operator[] (int index) const;
  int& operator[] (int index);

  /*!
   * \brief Comparison.
   *
   * Compare order: z, y, x
   */
  bool operator < (const ZIntPoint &pt) const;
  bool operator == (const ZIntPoint &pt) const;

  ZIntPoint operator - () const;

  friend ZIntPoint operator + (const ZIntPoint &pt1, const ZIntPoint &pt2);
  friend ZIntPoint operator + (const ZIntPoint &pt1, int v);
  friend ZIntPoint operator - (const ZIntPoint &pt1, const ZIntPoint &pt2);
  /*!
   * \brief Coordinate-wise ivision
   *
   * It returns (0, 0, 0) if \a pt2 has a 0 coordinate value.
   */
  friend ZIntPoint operator / (const ZIntPoint &pt1, const ZIntPoint &pt2);

  std::string toString() const;

public:
  int m_x;
  int m_y;
  int m_z;
};

#endif // ZINTPOINT_H
