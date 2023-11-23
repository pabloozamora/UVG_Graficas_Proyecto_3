#pragma once
#include "../box.h"
#include <cmath>

class Gold : public Cube
{
public:
  Gold(const glm::vec3 &minBound, const glm::vec3 &maxBound, const Material &mat)
      : Cube(minBound, maxBound, mat) {}

  Intersect rayIntersect(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection) const override
  {
    Intersect intersect = Cube::rayIntersect(rayOrigin, rayDirection);

    // Misma textura para todas las caras
    Color c = loadTexture(std::abs(intersect.point.x - minBound.x), std::abs(intersect.point.y - minBound.y), "gold");
    intersect.color = c;
    intersect.hasColor = true;

    return intersect;

    return intersect;
  };
};
