#include "gui.h"
#include <debuggl.h>
#include <jpegio.h>
#include <algorithm>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include "bone_geometry.h"
#include "config.h"

namespace Cylinder {
// FIXME: Implement a function that performs proper
//        ray-cylinder intersection detection
// TIPS: The implement is provided by the ray-tracer starter code.

const double RAY_EPSILON = 0.00000001;

glm::dvec3 at(glm::dvec3 p, glm::dvec3 d, double t) { return p + t * d; }

bool intersectBody(glm::dvec3 p, glm::dvec3 d, int &T) {
    double x0 = p.x;
    double y0 = p.y;
    double x1 = d.x;
    double y1 = d.y;

    double a = x1 * x1 + y1 * y1;
    double b = 2.0 * (x0 * x1 + y0 * y1);
    double c = x0 * x0 + y0 * y0 - 1.0;

    if (0.0 == a) {
        // This implies that x1 = 0.0 and y1 = 0.0, which further
        // implies that the ray is aligned with the body of the cylinder,
        // so no intersection.
        return false;
    }

    double discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0) {
        return false;
    }

    discriminant = sqrt(discriminant);

    double t2 = (-b + discriminant) / (2.0 * a);

    if (t2 <= RAY_EPSILON) {
        return false;
    }

    double t1 = (-b - discriminant) / (2.0 * a);

    if (t1 > RAY_EPSILON) {
        // Two intersections.
        glm::dvec3 P = at(p, d, t1);
        double z = P[2];
        if (z >= 0.0 && z <= 1.0) {
            // It's okay.
            // i.setT(t1);
            T = t1;
            // i.setN(glm::normalize(glm::dvec3(P[0], P[1], 0.0)));
            return true;
        }
    }

    glm::dvec3 P = at(p, d, t2);
    double z = P[2];
    if (z >= 0.0 && z <= 1.0) {
        // i.setT(t2);
        T = t2;
        glm::dvec3 normal(P[0], P[1], 0.0);
        // In case we are _inside_ the _uncapped_ cone, we need to flip the
        // normal. Essentially, the cone in this case is a double-sided surface
        // and has _2_ normals
        // if (!capped && glm::dot(normal, r.getDirection()) > 0) normal =
        // -normal;

        // i.setN(glm::normalize(normal));
        return true;
    }

    return false;
}

bool intersectCaps(glm::dvec3 p, glm::dvec3 d, int &T) {
    double pz = p.z;
    double dz = d.z;

    if (0.0 == dz) {
        return false;
    }

    double t1;
    double t2;

    if (dz > 0.0) {
        t1 = (-pz) / dz;
        t2 = (1.0 - pz) / dz;
    } else {
        t1 = (1.0 - pz) / dz;
        t2 = (-pz) / dz;
    }

    if (t2 < RAY_EPSILON) {
        return false;
    }

    if (t1 >= RAY_EPSILON) {
        glm::dvec3 P(at(p, d, t1));
        if ((P[0] * P[0] + P[1] * P[1]) <= 1.0) {
            // i.seatT(t1);
            T = t1;
            if (dz > 0.0) {
                // Intersection with cap at z = 0.
                // i.setN(glm::dvec3(0.0, 0.0, -1.0));
            } else {
                // i.setN(glm::dvec3(0.0, 0.0, 1.0));
            }
            return true;
        }
    }

    glm::dvec3 P(at(p, d, t2));
    if ((P[0] * P[0] + P[1] * P[1]) <= 1.0) {
        // i.setT(t2);
        T = t2;
        if (dz > 0.0) {
            // Intersection with interior of cap at z = 1.
            // i.setN(glm::dvec3(0.0, 0.0, 1.0));
        } else {
            // i.setN(glm::dvec3(0.0, 0.0, -1.0));
        }
        return true;
    }

    return false;
}

bool intersectLocal(glm::dvec3 p, glm::dvec3 d, int &T) {
    // FIXME: check these suspicious initialization.
    // i.setObject(this);
    // i.setMaterial(this->getMaterial());

    if (intersectCaps(p, d, T)) {
        int tt;
        if (intersectBody(p, d, tt)) {
            if (tt < T) {
                T = tt;
            }
        }
        return true;
    } else {
        return intersectBody(p, d, T);
    }
}

}  // namespace Cylinder

GUI::GUI(GLFWwindow *window, int view_width, int view_height,
         int preview_height)
    : window_(window), preview_height_(preview_height) {
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, KeyCallback);
    glfwSetCursorPosCallback(window_, MousePosCallback);
    glfwSetMouseButtonCallback(window_, MouseButtonCallback);
    glfwSetScrollCallback(window_, MouseScrollCallback);

    glfwGetWindowSize(window_, &window_width_, &window_height_);
    if (view_width < 0 || view_height < 0) {
        view_width_ = window_width_;
        view_height_ = window_height_;
    } else {
        view_width_ = view_width;
        view_height_ = view_height;
    }
    float aspect_ = static_cast<float>(view_width_) / view_height_;
    projection_matrix_ =
        glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
}

GUI::~GUI() {}

void GUI::assignMesh(Mesh *mesh) {
    mesh_ = mesh;
    center_ = mesh_->getCenter();
}

void GUI::keyCallback(int key, int scancode, int action, int mods) {
#if 0
	if (action != 2)
		std::cerr << "Key: " << key << " action: " << action << std::endl;
#endif
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window_, GL_TRUE);
        return;
    }
    if (key == GLFW_KEY_J && action == GLFW_RELEASE) {
        unsigned char *pixels;
        int screen_info[4];

        // get the width/height of the window
        glGetIntegerv(GL_VIEWPORT, screen_info);
        pixels = new unsigned char[screen_info[2] * screen_info[3] * 3];

        // Read in pixel data
        glReadPixels(0, 0, screen_info[2], screen_info[3], GL_RGB,
                     GL_UNSIGNED_BYTE, pixels);

        SaveJPEG("out.jpg", screen_info[2], screen_info[3], pixels);

        std::cout << "Saved to out.jpg!" << std::endl;
    }
    if (key == GLFW_KEY_S && (mods & GLFW_MOD_CONTROL)) {
        if (action == GLFW_RELEASE) mesh_->saveAnimationTo("animation.json");
        return;
    }

    if (mods == 0 && captureWASDUPDOWN(key, action)) return;
    if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) {
        float roll_speed;
        if (key == GLFW_KEY_RIGHT)
            roll_speed = -roll_speed_;
        else
            roll_speed = roll_speed_;
        // FIXME: actually roll the bone here

        if (current_bone_ == -1) {
            return;
        }

        Bone *bone = mesh_->skeleton.bones[current_bone_];
        glm::fquat rotate_ =
            glm::toQuat(glm::rotate(roll_speed, bone->direction));
        bone->rotate(rotate_);
        mesh_->updateAnimation();
    } else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
        fps_mode_ = !fps_mode_;
    } else if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_RELEASE) {
        current_bone_--;
        current_bone_ += mesh_->getNumberOfBones();
        current_bone_ %= mesh_->getNumberOfBones();
    } else if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_RELEASE) {
        current_bone_++;
        current_bone_ += mesh_->getNumberOfBones();
        current_bone_ %= mesh_->getNumberOfBones();
    } else if (key == GLFW_KEY_T && action != GLFW_RELEASE) {
        transparent_ = !transparent_;
    } else if (key == GLFW_KEY_I && action != GLFW_RELEASE) {
        translate_ = !translate_;
    }

    // FIXME: implement other controls here.
}

void GUI::mousePosCallback(double mouse_x, double mouse_y) {
    last_x_ = current_x_;
    last_y_ = current_y_;
    current_x_ = mouse_x;
    current_y_ = window_height_ - mouse_y;
    float delta_x = current_x_ - last_x_;
    float delta_y = current_y_ - last_y_;
    if (sqrt(delta_x * delta_x + delta_y * delta_y) < 1e-15) return;
    if (mouse_x > view_width_) return;
    glm::vec3 mouse_direction =
        glm::normalize(glm::vec3(delta_x, delta_y, 0.0f));
    glm::vec2 mouse_start = glm::vec2(last_x_, last_y_);
    glm::vec2 mouse_end = glm::vec2(current_x_, current_y_);
    glm::uvec4 viewport = glm::uvec4(0, 0, view_width_, view_height_);

    bool drag_camera =
        drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_RIGHT;
    bool drag_bone = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_LEFT;

    if (drag_camera) {
        glm::vec3 axis =
            glm::normalize(orientation_ * glm::vec3(mouse_direction.y,
                                                    -mouse_direction.x, 0.0f));
        orientation_ = glm::mat3(glm::rotate(rotation_speed_, axis) *
                                 glm::mat4(orientation_));
        tangent_ = glm::column(orientation_, 0);
        up_ = glm::column(orientation_, 1);
        look_ = glm::column(orientation_, 2);
    } else if (drag_bone && current_bone_ != -1) {
        // FIXME: Handle bone rotation

        if (!translate_) {
            Bone *bone = mesh_->skeleton.bones[current_bone_];
            glm::vec3 start = glm::unProject(glm::vec3(mouse_start, 0.0f),
                                             view_matrix_ * model_matrix_,
                                             projection_matrix_, viewport);
            glm::vec3 end = glm::unProject(glm::vec3(mouse_end, 0.0f),
                                           view_matrix_ * model_matrix_,
                                           projection_matrix_, viewport);
            glm::vec3 delta = glm::normalize(glm::cross(end - start, look_));

            glm::mat4 rotate = glm::rotate(rotation_speed_, delta);
            glm::fquat rotate_ = glm::normalize(glm::toQuat(rotate));
            bone->rotate(rotate_);
            mesh_->updateAnimation();
        } else {
            Bone *bone = mesh_->skeleton.bones[current_bone_];
            int root = bone->root;
            glm::vec3 start = glm::unProject(glm::vec3(mouse_start, 0.0f),
                                             view_matrix_ * model_matrix_,
                                             projection_matrix_, viewport);
            glm::vec3 end = glm::unProject(glm::vec3(mouse_end, 0.0f),
                                           view_matrix_ * model_matrix_,
                                           projection_matrix_, viewport);
            glm::vec3 delta = end - start;
            mesh_->skeleton.translate(delta, root);
            mesh_->updateAnimation();
        }
        return;
    }

    // FIXME: highlight bones that have been moused over
    current_bone_ = -1;
    int T = -1;
    int prev = 1000000;

    // Convert the position of the mouse cursor in screen coordinates to a ray
    // in world coordinates.
    // Normalized Device Coordinates
    float x = (2.0f * mouse_x) / window_width_ - 1.0f;
    float y = 1.0f - (2.0f * mouse_y) / window_height_;
    float z = 1.0f;

    // 4d homogeneous coordinates
    glm::vec4 r_clip = glm::vec4(x, y, -z, 1.0f);

    // 4d eye coordinates
    glm::vec4 r_eye = glm::inverse(projection_matrix_) * r_clip;
    r_eye = glm::vec4(r_eye[0], r_eye[1], -1.0f, 0.0f);

    // 4d world cooridnates
    glm::vec3 r_world = glm::vec3(glm::inverse(view_matrix_) * r_eye);
    r_world = glm::normalize(r_world);

    // std::cout << r_world[0] << " " << r_world[1] << " " << r_world[2]
    //          << std::endl;

    std::vector<Bone *> bones = mesh_->skeleton.bones;

    for (auto bone : bones) {
        T = -1;
        if (bone != nullptr) {
            glm::mat4 ori = bone_transform_index(bone->index);
            glm::vec3 p = eye_;
            glm::vec3 d = r_world;
            glm::mat4 scale = glm::mat4(bone->length);
            /*
            p = glm::vec3(glm::inverse(bone->deformed_transform) *
                          glm::vec4(p, 1));
            d = glm::vec3(glm::inverse(bone->deformed_transform) *
                          glm::vec4(d, 0));
            */

            p = glm::vec3(glm::inverse(ori) * glm::vec4(p, 1));
            d = glm::vec3(glm::inverse(ori) * glm::vec4(d, 0));

            bool val = Cylinder::intersectLocal(p, d, T);

            if (val) {
                if (T >= 0 && T < prev) {
                    current_bone_ = bone->index;
                    prev = T;
                }
            }
        }
    }
    std::cout << "current bone:" << current_bone_ << std::endl;
}

glm::mat4 GUI::bone_transform() {
    int index = current_bone_;
    return bone_transform_index(index);
}

glm::mat4 GUI::bone_transform_index(int index) {
    if (index == 0) {
        return glm::mat4(1.0f);
    }
    Bone *bone = mesh_->skeleton.bones[index];

    auto alignment = glm::mat4(1.0);
    alignment[0][0] = bone->direction[0];
    alignment[0][1] = bone->direction[1];
    alignment[0][2] = bone->direction[2];

    glm::vec3 y;
    if (bone->direction.x != 0) {
        y = glm::normalize(
            glm::cross(bone->direction, glm::vec3(0.0, 1.0, 0.0)));
    } else {
        y = glm::normalize(
            glm::cross(bone->direction, glm::vec3(1.0, 0.0, 0.0)));
    }

    auto z = glm::normalize(glm::cross(bone->direction, y));

    alignment[1][0] = y[0];
    alignment[1][1] = y[1];
    alignment[1][2] = y[2];
    alignment[2][0] = z[0];
    alignment[2][1] = z[1];
    alignment[2][2] = z[2];

    return bone->deformed_transform * alignment *
           glm::scale(glm::vec3(bone->length, 0.25, 0.25));
}

void GUI::mouseButtonCallback(int button, int action, int mods) {
    if (current_x_ <= view_width_) {
        drag_state_ = (action == GLFW_PRESS);
        current_button_ = button;
        return;
    }
    // FIXME: Key Frame Selection
}

void GUI::mouseScrollCallback(double dx, double dy) {
    if (current_x_ < view_width_) return;
    // FIXME: Mouse Scrolling
}

void GUI::updateMatrices() {
    // Compute our view, and projection matrices.
    if (fps_mode_)
        center_ = eye_ + camera_distance_ * look_;
    else
        eye_ = center_ - camera_distance_ * look_;

    view_matrix_ = glm::lookAt(eye_, center_, up_);
    light_position_ = glm::vec4(eye_, 1.0f);

    aspect_ = static_cast<float>(view_width_) / view_height_;
    projection_matrix_ =
        glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
    model_matrix_ = glm::mat4(1.0f);
}

MatrixPointers GUI::getMatrixPointers() const {
    MatrixPointers ret;
    ret.projection = &projection_matrix_;
    ret.model = &model_matrix_;
    ret.view = &view_matrix_;
    return ret;
}

bool GUI::setCurrentBone(int i) {
    if (i < 0 || i >= mesh_->getNumberOfBones()) return false;
    current_bone_ = i;
    return true;
}

float GUI::getCurrentPlayTime() const { return 0.0f; }

bool GUI::captureWASDUPDOWN(int key, int action) {
    if (key == GLFW_KEY_W) {
        if (fps_mode_)
            eye_ += zoom_speed_ * look_;
        else
            camera_distance_ -= zoom_speed_;
        return true;
    } else if (key == GLFW_KEY_S) {
        if (fps_mode_)
            eye_ -= zoom_speed_ * look_;
        else
            camera_distance_ += zoom_speed_;
        return true;
    } else if (key == GLFW_KEY_A) {
        if (fps_mode_)
            eye_ -= pan_speed_ * tangent_;
        else
            center_ -= pan_speed_ * tangent_;
        return true;
    } else if (key == GLFW_KEY_D) {
        if (fps_mode_)
            eye_ += pan_speed_ * tangent_;
        else
            center_ += pan_speed_ * tangent_;
        return true;
    } else if (key == GLFW_KEY_DOWN) {
        if (fps_mode_)
            eye_ -= pan_speed_ * up_;
        else
            center_ -= pan_speed_ * up_;
        return true;
    } else if (key == GLFW_KEY_UP) {
        if (fps_mode_)
            eye_ += pan_speed_ * up_;
        else
            center_ += pan_speed_ * up_;
        return true;
    }
    return false;
}

// Delegrate to the actual GUI object.
void GUI::KeyCallback(GLFWwindow *window, int key, int scancode, int action,
                      int mods) {
    GUI *gui = (GUI *)glfwGetWindowUserPointer(window);
    gui->keyCallback(key, scancode, action, mods);
}

void GUI::MousePosCallback(GLFWwindow *window, double mouse_x, double mouse_y) {
    GUI *gui = (GUI *)glfwGetWindowUserPointer(window);
    gui->mousePosCallback(mouse_x, mouse_y);
}

void GUI::MouseButtonCallback(GLFWwindow *window, int button, int action,
                              int mods) {
    GUI *gui = (GUI *)glfwGetWindowUserPointer(window);
    gui->mouseButtonCallback(button, action, mods);
}

void GUI::MouseScrollCallback(GLFWwindow *window, double dx, double dy) {
    GUI *gui = (GUI *)glfwGetWindowUserPointer(window);
    gui->mouseScrollCallback(dx, dy);
}
