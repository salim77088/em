#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace nexus::math {

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using quat = glm::quat;

inline mat4 identity() { return glm::mat4(1.0f); }
inline mat4 translate(const mat4& m, const vec3& v) { return glm::translate(m, v); }
inline mat4 rotate(const mat4& m, float angle, const vec3& axis) { return glm::rotate(m, angle, axis); }
inline mat4 scale(const mat4& m, const vec3& v) { return glm::scale(m, v); }
inline mat4 perspective(float fov, float aspect, float near, float far) { return glm::perspective(fov, aspect, near, far); }
inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) { return glm::lookAt(eye, center, up); }

} // namespace nexus::math
