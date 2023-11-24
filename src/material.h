#pragma once

#include "color.h"

struct Material {
  Color diffuse;
  float albedo; // de 0 a 1 qué tanto emite el color difuso (base)
  float specularAlbedo; // qué tanto refleja el color de la luz
  float specularCoefficient; // qué tanto se difuminan los highlights de la luz (10 se difuminan poco, 0.1 se difuminan bastante)
  float reflectivity;
  float transparency;
  float refractionIndex;
};
