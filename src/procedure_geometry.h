#ifndef PROCEDURE_GEOMETRY_H
#define PROCEDURE_GEOMETRY_H

#include <glm/glm.hpp>
#include <vector>

struct LineMesh;

void create_floor(std::vector<glm::vec4>& floor_vertices,
                  std::vector<glm::uvec3>& floor_faces);
void create_bone_mesh(LineMesh& bone_mesh);
void create_cylinder_mesh(LineMesh& cylinder_mesh);
void create_axes_mesh(LineMesh& axes_mesh);
void create_quads(std::vector<glm::vec4>& quad_vertices,
                  std::vector<glm::uvec3>& quad_faces,
                  std::vector<glm::vec2>& quad_indices);

#endif
