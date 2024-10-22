#ifndef BONE_GEOMETRY_H
#define BONE_GEOMETRY_H

#include <mmdadapter.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>
#include <map>
#include <ostream>
#include <string>
#include <vector>

class TextureToRender;

struct BoundingBox {
    BoundingBox()
        : min(glm::vec3(-std::numeric_limits<float>::max())),
          max(glm::vec3(std::numeric_limits<float>::max())) {}
    glm::vec3 min;
    glm::vec3 max;
};

struct Joint {
    Joint()
        : joint_index(-1),
          parent_index(-1),
          position(glm::vec3(0.0f)),
          init_position(glm::vec3(0.0f)) {}
    Joint(int id, glm::vec3 wcoord, int parent)
        : joint_index(id),
          parent_index(parent),
          position(wcoord),
          init_position(wcoord),
          init_rel_position(init_position) {}

    int joint_index;
    int parent_index;
    glm::vec3 position;      // position of the joint
    glm::fquat orientation;  // rotation w.r.t. initial configuration
    glm::fquat
        rel_orientation;  // rotation w.r.t. it's parent. Used for animation.
    glm::vec3 init_position;      // initial position of this joint
    glm::vec3 init_rel_position;  // initial relative position to its parent
    std::vector<int> children;
};

struct Bone {
    Bone(Joint start, Joint end) {
        this->start = &start;
        this->end = &end;
        this->index = end.joint_index;
        this->parent_index = start.joint_index;
        this->length = glm::length(end.position - start.position);
        this->direction = glm::normalize(end.position - start.position);
        this->parent = nullptr;
        this->parent_orientation_relative = glm::fquat(1.0, 0.0, 0.0, 0.0);
        this->orientation = glm::fquat(1.0, 0.0, 0.0, 0.0);
        this->root = start.joint_index;
    };

    // properities

    int index;
    int parent_index;
    int root;
    double length;

    glm::vec3 direction;

    Joint* start;
    Joint* end;

    Bone* parent;
    std::vector<Bone*> nodes;

    // matrices
    glm::mat4 start_translation;
    glm::mat4 undeformed_transform;
    glm::mat4 deformed_transform;
    glm::mat4 translation;

    glm::fquat parent_orientation_relative;
    glm::fquat orientation;

    void translate(glm::vec3 translation);
    void translateParent();
    void performAnimateTranslate(glm::vec3 diff_translation);
    void rotate(glm::fquat rotate_);
    void rotate_(glm::fquat rotate_);
    void rotateParent();
};

struct Configuration {
    std::vector<glm::vec3> trans;
    std::vector<glm::fquat> rot;

    const auto& transData() const { return trans; }
    const auto& rotData() const { return rot; }
};

struct KeyFrame {
    std::vector<glm::fquat> rel_rot;
    glm::vec3 root;
    static void interpolate(const KeyFrame& from, const KeyFrame& to, float tau,
                            KeyFrame& target);
    static void interpolateSpline(const std::vector<KeyFrame>& key_frames,
                                  float t, KeyFrame& target,
                                  const KeyFrame& from, const KeyFrame& to,
                                  float tau);
};

struct LineMesh {
    std::vector<glm::vec4> vertices;
    std::vector<glm::uvec2> indices;
};

struct Skeleton {
    std::vector<Joint> joints;

    Configuration cache;

    void refreshCache(Configuration* cache = nullptr);
    const glm::vec3* collectJointTrans() const;
    const glm::fquat* collectJointRot() const;
    void constructBone(int joint);
    void translate(glm::vec3 translation, int root);
    void animateTranslate(glm::vec3 diff_translation, int root_id);
    // FIXME: create skeleton and bone data structures
    std::vector<Bone*> bones;
};

struct Mesh {
    Mesh();
    ~Mesh();
    std::vector<glm::vec4> vertices;
    /*
     * Static per-vertex attrributes for Shaders
     */
    std::vector<int32_t> joint0;
    std::vector<int32_t> joint1;
    std::vector<float>
        weight_for_joint0;  // weight_for_joint1 can be calculated
    std::vector<glm::vec3> vector_from_joint0;
    std::vector<glm::vec3> vector_from_joint1;
    std::vector<glm::vec4> vertex_normals;
    std::vector<glm::vec4> face_normals;
    std::vector<glm::vec2> uv_coordinates;
    std::vector<glm::uvec3> faces;
    std::vector<KeyFrame> key_frames;
    std::vector<TextureToRender*> previews;
    std::vector<Material> materials;
    BoundingBox bounds;
    Skeleton skeleton;

    void loadPmd(const std::string& fn);
    int getNumberOfBones() const;
    glm::vec3 getCenter() const {
        return 0.5f * glm::vec3(bounds.min + bounds.max);
    }
    const Configuration* getCurrentQ()
        const;  // Configuration is abbreviated as Q
    void updateAnimation(float t = -1.0);
    void updateSkeleton(KeyFrame frame);
    void updateFromRel(Joint& parent);

    void constructKeyFrame();
    void delKeyFrame(int frame_id);
    void updateKeyFrame(int frame_id);
    void spaceKeyFrame(int frame_id);
    void insertKeyFrame(int frame_id);

    void saveAnimationTo(const std::string& fn);
    void loadAnimationFrom(const std::string& fn);
    void setSpline(bool x) { spline_ = x; }
    bool getSpline() { return spline_; }

   private:
    void computeBounds();
    void computeNormals();
    Configuration currentQ_;
    bool spline_ = false;
};

#endif
