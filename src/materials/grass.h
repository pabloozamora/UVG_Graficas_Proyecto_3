#pragma once
#include "../cube.h"
#include <cmath>

class Grass : public Cube
{
public:
  Grass(const glm::vec3 &minBound, const glm::vec3 &maxBound, const Material &mat)
      : Cube(minBound, maxBound, mat) {}

  Intersect rayIntersect(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection) const override
  {
    Intersect intersect = Cube::rayIntersect(rayOrigin, rayDirection);

    const float epsilon = 0.0001;
    if (glm::abs(intersect.point.y - maxBound.y) < epsilon)
    {
    // Añadir textura arriba grama
      Color c = loadTexture(std::abs(intersect.point.x - minBound.x), std::abs(intersect.point.z - minBound.z), "grass");

      intersect.color = c;
      intersect.hasColor = true;
    }
    else if (glm::abs(intersect.point.z - maxBound.z) < epsilon || glm::abs(intersect.point.z - minBound.z) < epsilon){
      // Tierra z
      Color c = loadTexture(std::abs(intersect.point.x - minBound.x), std::abs(intersect.point.y - minBound.y), "grass_side");

      intersect.color = c;
      intersect.hasColor = true;
    }
    else if (glm::abs(intersect.point.x - minBound.x) < epsilon || glm::abs(intersect.point.x - maxBound.x) < epsilon){
      // Tierra x
      Color c = loadTexture(std::abs(intersect.point.z - minBound.z), std::abs(intersect.point.y - minBound.y), "grass_side");

      intersect.color = c;
      intersect.hasColor = true;
    }

    return intersect;
  };
};
