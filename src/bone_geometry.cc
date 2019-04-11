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

    for (int i = 0; i < bones.size(); i++) {
        if (bones[i] != nullptr) {
            joints[bones[i]->parent_index].orientation = bones[i]->orientation;
            joints[bones[i]->parent_index].rel_orientation =
                bones[i]->parent_orientation_relative;
        }
    }

    for (int i = 0; i < joints.size(); i++) {
        if (joints[i].parent_index != -1) {
            joints[i].position =
                glm::vec3(bones[i]->deformed_transform *
                          glm::inverse(bones[i]->undeformed_transform) *
                          glm::vec4(joints[i].init_position, 1.0));
        } else {
            int root = joints[i].children[0];
            joints[i].position = glm::vec3(bones[root]->translation[3][0],
                                           bones[root]->translation[3][1],
                                           bones[root]->translation[3][2]);
        }
    }
    for (int i = 0; i < joints.size(); i++) {
        target->rot[i] = joints[i].orientation;
        target->trans[i] = joints[i].position;
    }
}

void Skeleton::translate(glm::vec3 translation, int root) {
    Joint joint = joints[root];
    for (int id : joint.children) {
        bones[id]->translate(translation);
    }
}

void Skeleton::animateTranslate(glm::vec3 diff_translation, int root_id) {
    Joint root = joints[root_id];
    for (int i = 0; i < root.children.size(); i++) {
        bones[root.children[i]]->performAnimateTranslate(diff_translation);
    }
}

void Bone::performAnimateTranslate(glm::vec3 diff_translation) {
    translation = start_translation;
    translation[3][0] += diff_translation.x;
    translation[3][1] += diff_translation.y;
    translation[3][2] += diff_translation.z;
    deformed_transform =
        translation * glm::toMat4(glm::normalize(parent_orientation_relative));
    for (int i = 0; i < nodes.size(); i++) {
        nodes[i]->translateParent();
    }
}

void Bone::translate(glm::vec3 translation) {
    this->translation[3][0] += 10.0f * translation.x;
    this->translation[3][1] += 10.0f * translation.y;
    this->translation[3][2] += 10.0f * translation.z;
    this->deformed_transform =
        this->translation *
        glm::toMat4(glm::normalize(this->parent_orientation_relative));

    for (int i = 0; i < nodes.size(); i++) {
        nodes[i]->translateParent();
    }
}

void Bone::translateParent() {
    this->deformed_transform =
        this->parent->deformed_transform * this->translation *
        glm::toMat4(glm::normalize(this->parent_orientation_relative));
    for (int i = 0; i < nodes.size(); i++) {
        nodes[i]->translateParent();
    }
}

void Bone::rotate(glm::fquat rotate_) {
    if (parent != nullptr)
        for (int i = 0; i < parent->nodes.size(); i++)
            parent->nodes[i]->rotate_(rotate_);
    else
        this->rotate_(rotate_);
}

void Bone::rotate_(glm::fquat rotate_) {
    glm::mat4 pre_parent = glm::toMat4(this->parent_orientation_relative);
    glm::mat4 pre_orientation = glm::toMat4(this->orientation);

    glm::mat4 update_parent = glm::toMat4(rotate_) * pre_parent;
    glm::mat4 update_orientation = glm::toMat4(rotate_) * pre_orientation;

    this->parent_orientation_relative =
        glm::normalize(glm::toQuat(update_parent));
    this->orientation = glm::normalize(glm::toQuat(update_orientation));

    this->deformed_transform *= glm::inverse(pre_parent) * update_parent;

    for (int i = 0; i < nodes.size(); i++) {
        nodes[i]->rotateParent();
    }
}

void Bone::rotateParent() {
    this->deformed_transform = this->parent->deformed_transform * translation *
                               glm::toMat4(this->parent_orientation_relative);
    glm::mat4 update_orientation =
        glm::toMat4(this->parent->orientation) *
        glm::toMat4(this->parent_orientation_relative);
    this->orientation = glm::normalize(glm::toQuat(update_orientation));

    for (int i = 0; i < nodes.size(); i++) {
        nodes[i]->rotateParent();
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
    if (start.parent_index > -1) {
        bone->parent = bones[end.parent_index];
        trans = start.position - joints[start.parent_index].position;
    }

    bone->translation = glm::mat4(1.0f);
    bone->translation[3][0] = trans.x;
    bone->translation[3][1] = trans.y;
    bone->translation[3][2] = trans.z;
    bone->start_translation = bone->translation;
    bone->undeformed_transform = bone->translation;

    if (start.parent_index == -1) {
        bone->deformed_transform =
            bone->translation *
            glm::toMat4(glm::normalize(bone->parent_orientation_relative));
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

void KeyFrame::interpolate(const KeyFrame& from, const KeyFrame& to, float tau,
                           KeyFrame& target) {
    auto rel_rot_from = from.rel_rot;
    auto rel_rot_to = to.rel_rot;

    target.rel_rot.resize(rel_rot_from.size());

    for (int i = 0; i < target.rel_rot.size(); i++)
        target.rel_rot[i] = glm::fastMix(rel_rot_from[i], rel_rot_to[i], tau);

    target.root = (1 - tau) * from.root + tau * to.root;
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

    skeleton.bones.resize(getNumberOfBones());
    for (int i = 0; i < getNumberOfBones(); ++i) {
        if (skeleton.joints[i].parent_index == -1) {
            skeleton.bones[i] = nullptr;
        }
    }

    for (int i = 1; i < getNumberOfBones(); i++) {
        skeleton.constructBone(i);
    }

    std::vector<SparseTuple> weights;
    mr.getJointWeights(weights);

    for (SparseTuple& sparse_tuple : weights) {
        int v_id = sparse_tuple.vid;
        joint0.emplace_back(sparse_tuple.jid0);
        joint1.emplace_back(sparse_tuple.jid1);
        weight_for_joint0.emplace_back(sparse_tuple.weight0);
        vector_from_joint0.emplace_back(
            glm::vec3(vertices[v_id]) -
            skeleton.joints[sparse_tuple.jid0].position);
        vector_from_joint1.emplace_back(
            glm::vec3(vertices[v_id]) -
            skeleton.joints[sparse_tuple.jid1].position);
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

void Mesh::updateSkeleton(KeyFrame frame) {
    for (int i = 0; i < getNumberOfBones(); i++) {
        skeleton.joints[i].rel_orientation = frame.rel_rot[i];
    }

    for (int i = 0; i < getNumberOfBones(); i++) {
        if (skeleton.joints[i].parent_index == -1) {
            skeleton.animateTranslate(frame.root,
                                      skeleton.joints[i].joint_index);
            skeleton.joints[i].orientation = skeleton.joints[i].rel_orientation;
            updateFromRel(skeleton.joints[i]);
        }
    }
}

void Mesh::updateFromRel(Joint& parent) {
    for (int child_id : parent.children) {
        Joint& child = skeleton.joints[child_id];
        child.orientation = child.rel_orientation * parent.orientation;

        Bone* bone = skeleton.bones[child_id];
        bone->parent_orientation_relative = parent.rel_orientation;
        bone->orientation = parent.orientation;

        if (parent.parent_index == -1) {
            bone->deformed_transform =
                bone->translation *
                glm::toMat4(bone->parent_orientation_relative);
        } else {
            bone->deformed_transform =
                bone->parent->deformed_transform * bone->translation *
                glm::toMat4(bone->parent_orientation_relative);
        }

        updateFromRel(child);
    }
}

void Mesh::constructKeyFrame() {
    KeyFrame frame;
    for (int i = 0; i < getNumberOfBones(); i++) {
        frame.rel_rot.emplace_back(skeleton.joints[i].rel_orientation);
    }
    frame.root = skeleton.joints[0].position - skeleton.joints[0].init_position;
    key_frames.emplace_back(frame);
    if (key_frames.size() == 1) {
        for (int i = 1; i < skeleton.bones.size(); i++) {
            skeleton.bones[i]->start_translation =
                skeleton.bones[i]->translation;
        }
    }
}

void Mesh::updateKeyFrame(int frame_id) {
    if (frame_id < 0) return;
    KeyFrame frame;
    for (int i = 0; i < skeleton.joints.size(); i++)
        frame.rel_rot.emplace_back(skeleton.joints[i].rel_orientation);

    frame.root = skeleton.joints[0].position - skeleton.joints[0].init_position;
    key_frames[frame_id] = frame;
}

void Mesh::delKeyFrame(int frame_id) {
    key_frames.erase(key_frames.begin() + frame_id);
}

void Mesh::spaceKeyFrame(int frame_id) {
    updateSkeleton(key_frames[frame_id]);
    skeleton.refreshCache(&currentQ_);
}

glm::fquat splineQuat(glm::fquat q0, glm::fquat q1, glm::fquat q2,
                      glm::fquat q3, float tau) {
    glm::fquat s1 = q1 * glm::exp((glm::log(glm::inverse(q1) * q2) +
                                   glm::log(glm::inverse(q1) * q0)) /
                                  (-4.0f));
    glm::fquat s2 = q2 * glm::exp((glm::log(glm::inverse(q2) * q3) +
                                   glm::log(glm::inverse(q2) * q1)) /
                                  (-4.0f));
    glm::fquat temp_1 = glm::mix(q1, q2, tau);
    glm::fquat temp_2 = glm::mix(s1, s2, tau);
    glm::fquat result = glm::mix(temp_1, temp_2, 2.0f * tau * (1.0f - tau));
    return result;
}

glm::fquat calculateSplineQuat(const std::vector<glm::fquat>& bone_rels,
                               float t) {
    int id_0 = glm::clamp<int>(t - 1, 0, bone_rels.size() - 1);
    int id_1 = glm::clamp<int>(t, 0, bone_rels.size() - 1);
    int id_2 = glm::clamp<int>(t + 1, 0, bone_rels.size() - 1);
    int id_3 = glm::clamp<int>(t + 2, 0, bone_rels.size() - 1);
    float tau = t - floor(t);
    return splineQuat(bone_rels[id_0], bone_rels[id_1], bone_rels[id_2],
                      bone_rels[id_3], tau);
    // return glm::fquat(1.0,0.0,0.0,0.0);
}

void KeyFrame::interpolateSpline(const std::vector<KeyFrame>& key_frames,
                                 float t, KeyFrame& target,
                                 const KeyFrame& from, const KeyFrame& to,
                                 float tau) {
    for (int i = 0; i < key_frames[0].rel_rot.size(); i++) {
        std::vector<glm::fquat> bone_rels;
        for (int f_id = 0; f_id < key_frames.size(); f_id++) {
            bone_rels.emplace_back(key_frames[f_id].rel_rot[i]);
        }
        glm::fquat spline_quat = calculateSplineQuat(bone_rels, t);
        target.rel_rot.emplace_back(spline_quat);
    }

    target.root = (1 - tau) * from.root + tau * to.root;
}

void Mesh::insertKeyFrame(int frame_id) {
    KeyFrame frame;
    for (int i = 0; i < skeleton.joints.size(); i++)
        frame.rel_rot.emplace_back(skeleton.joints[i].rel_orientation);

    frame.root = skeleton.joints[0].position - skeleton.joints[0].init_position;

    key_frames.insert(key_frames.begin() + frame_id, frame);

    if (frame_id == 0) {
        for (int i = 1; i < skeleton.bones.size(); i++) {
            skeleton.bones[i]->start_translation =
                skeleton.bones[i]->translation;
        }
    }
}

void Mesh::updateAnimation(float t) {
    skeleton.refreshCache(&currentQ_);

    int frame_id = floor(t);
    if (t != -1.0 && frame_id + 1 < (int)key_frames.size()) {
        float tao = t - frame_id;
        KeyFrame frame;
        if (spline_)
            KeyFrame::interpolateSpline(key_frames, t, frame,
                                        key_frames[frame_id],
                                        key_frames[frame_id + 1], tao);
        else
            KeyFrame::interpolate(key_frames[frame_id],
                                  key_frames[frame_id + 1], tao, frame);

        updateSkeleton(frame);
    }
}

const Configuration* Mesh::getCurrentQ() const { return &currentQ_; }
