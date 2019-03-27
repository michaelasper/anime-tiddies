#include "bone_geometry.h"
#include <fstream>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <queue>
#include <stdexcept>
#include "config.h"
#include "texture_to_render.h"

/*
 * For debugging purpose.
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    size_t count = std::min(v.size(), static_cast<size_t>(10));
    for (size_t i = 0; i < count; ++i) os << i << " " << v[i] << "\n";
    os << "size = " << v.size() << "\n";
    return os;
}

std::ostream& operator<<(std::ostream& os, const BoundingBox& bounds) {
    os << "min = " << bounds.min << " max = " << bounds.max;
    return os;
}

const glm::vec3* Skeleton::collectJointTrans() const {
    return cache.trans.data();
}

const glm::fquat* Skeleton::collectJointRot() const { return cache.rot.data(); }

// FIXME: Implement bone animation.

void Skeleton::refreshCache(Configuration* target) {
    if (target == nullptr) target = &cache;
    target->rot.resize(joints.size());
    target->trans.resize(joints.size());
    for (size_t i = 0; i < joints.size(); i++) {
        target->rot[i] = joints[i].orientation;
        target->trans[i] = joints[i].position;
    }
}

void Skeleton::constructBone(int joint) {
    if (bones[joint] != nullptr) return;

    Joint end = joints[joint];
    if (end.parent_index == -1) {
        bones[joint] = nullptr;
        return;
    }

    Joint start = joints[end.parent_index];

    Bone* bone = new Bone(start, end);
    joints[end.parent_index].children.emplace_back(end.joint_index);

    bones[joint] = bone;

    auto trans = start.position;
    if (start.parent_index >= 0) {
        bone->parent = bones[end.parent_index];
        trans = start.position - joints[start.parent_index].position;
    }

    bone->translation = glm::mat4(1.0f);
    bone->translation[3][0] = trans.x;
    bone->translation[3][1] = trans.y;
    bone->translation[3][2] = trans.z;
    bone->start_translation = bone->translation;

    if (start.parent_index == -1) {
        bone->undeformed_transform = bone->translation;
        bone->deformed_transform =
            bone->translation *
            glm::toMat4(glm::normalize(bone->parent_orientation_relative));
        bone->root = start.joint_index;
    } else {
        bone->undeformed_transform =
            (bone->parent)->undeformed_transform * bone->translation;
        bone->deformed_transform =
            (bone->parent)->deformed_transform * bone->translation *
            glm::toMat4(glm::normalize(bone->parent_orientation_relative));
        bone->root = bone->parent->root;
    }

    if (bone->parent != nullptr) {
        bone->parent->nodes.emplace_back(bone);
    }
}

Mesh::Mesh() {}

Mesh::~Mesh() {}

void Mesh::loadPmd(const std::string& fn) {
    MMDReader mr;
    mr.open(fn);
    mr.getMesh(vertices, faces, vertex_normals, uv_coordinates);
    computeBounds();
    mr.getMaterial(materials);

    // FIXME: load skeleton and blend weights from PMD file,
    //        initialize std::vectors for the vertex attributes,
    //        also initialize the skeleton as needed
    int id = 0;
    int parent = 0;
    glm::vec3 pos;

    while (mr.getJoint(id, pos, parent)) {
        Joint joint = Joint(id, pos, parent);
        skeleton.joints.emplace_back(joint);
        id++;
    }

    skeleton.bones.resize(skeleton.joints.size());
    for (int i = 0; i < skeleton.joints.size(); ++i) {
        if (skeleton.joints[i].parent_index == -1) {
            skeleton.bones[i] = nullptr;
        }
    }

    for (uint i = 1; i < skeleton.joints.size(); i++) {
        skeleton.constructBone(i);
    }
}

int Mesh::getNumberOfBones() const { return skeleton.joints.size(); }

void Mesh::computeBounds() {
    bounds.min = glm::vec3(std::numeric_limits<float>::max());
    bounds.max = glm::vec3(-std::numeric_limits<float>::max());
    for (const auto& vert : vertices) {
        bounds.min = glm::min(glm::vec3(vert), bounds.min);
        bounds.max = glm::max(glm::vec3(vert), bounds.max);
    }
}

void Mesh::updateAnimation(float t) {
    skeleton.refreshCache(&currentQ_);
    // FIXME: Support Animation Here
}

const Configuration* Mesh::getCurrentQ() const { return &currentQ_; }
