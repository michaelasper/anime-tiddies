#include <fstream>
#include <glm/gtx/io.hpp>
#include <iostream>
#include <unordered_map>
#include "bone_geometry.h"
#include "json.hpp"
#include "texture_to_render.h"

using json = nlohmann::json;
/*
 * Placeholder functions for Milestone 2
 */

void Mesh::saveAnimationTo(const std::string& fn) {
    json json_save;
    for (int i = 0; i < key_frames.size(); i++) {
        json a_frame = json::array();
        json rotation = json::array();
        json translation = json::array();
        for (int j = 0; j < getNumberOfBones(); j++) {
            json rot_joint = json::array();
            rot_joint.emplace_back(key_frames[i].rel_rot[j].x);
            rot_joint.emplace_back(key_frames[i].rel_rot[j].y);
            rot_joint.emplace_back(key_frames[i].rel_rot[j].z);
            rot_joint.emplace_back(key_frames[i].rel_rot[j].w);
            rotation.emplace_back(rot_joint);
        }
        translation.emplace_back(key_frames[i].root.x);
        translation.emplace_back(key_frames[i].root.y);
        translation.emplace_back(key_frames[i].root.z);
        a_frame.emplace_back(rotation);
        a_frame.emplace_back(translation);
        json_save.emplace_back(a_frame);
    }
    std::ofstream o(fn);
    o << std::setw(4) << json_save << std::endl;
}

void Mesh::loadAnimationFrom(const std::string& fn) {
    std::ifstream i(fn);
    json json_load;
    i >> json_load;
    for (int i = 0; i < json_load.size(); i++) {
        json a_frame = json_load[i];
        json rotation = a_frame[0];
        json translation = a_frame[1];
        KeyFrame frame;
        frame.root = glm::vec3(translation[0], translation[1], translation[2]);
        for (int j = 0; j < rotation.size(); j++) {
            json rot_joint = rotation[j];
            glm::fquat rel_rotation = glm::fquat(rot_joint[3], rot_joint[0],
                                                 rot_joint[1], rot_joint[2]);
            frame.rel_rot.emplace_back(rel_rotation);
        }
        key_frames.emplace_back(frame);
    }
    updateSkeleton(key_frames[0]);
    skeleton.refreshCache(&currentQ_);
}
