#include <GL/glew.h>

#include "bone_geometry.h"
#include "config.h"
#include "gui.h"
#include "procedure_geometry.h"
#include "render_pass.h"
#include "texture_to_render.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <debuggl.h>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>

int window_width = 1280;
int window_height = 720;
int main_view_width = 960;
int main_view_height = 720;
int preview_width = (window_width - main_view_width);  // 320
int preview_height = preview_width / 4 * 3;            // 320 / 4 * 3 = 240
int preview_bar_width = preview_width;
int preview_bar_height = main_view_height;
const std::string window_title = "Animation";

const char* vertex_shader =
#include "shaders/default.vert"
    ;

const char* blending_shader =
#include "shaders/blending.vert"
    ;

const char* geometry_shader =
#include "shaders/default.geom"
    ;

const char* fragment_shader =
#include "shaders/default.frag"
    ;

const char* floor_fragment_shader =
#include "shaders/floor.frag"
    ;

const char* bone_vertex_shader =
#include "shaders/bone.vert"
    ;

const char* bone_fragment_shader =
#include "shaders/bone.frag"
    ;

const char* cylinder_vertex_shader =
#include "shaders/cylinder.vert"
    ;

const char* cylinder_fragment_shader =
#include "shaders/cylinder.frag"
    ;

const char* preview_vertex_shader =
#include "shaders/preview.vert"
    ;

const char* preview_fragment_shader =
#include "shaders/preview.frag"
    ;

// FIXME: Add more shaders here.

void ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error: " << description << "\n";
}

GLFWwindow* init_glefw() {
    if (!glfwInit()) exit(EXIT_FAILURE);
    glfwSetErrorCallback(ErrorCallback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE,
                   GL_FALSE);  // Disable resizing, for simplicity
    glfwWindowHint(GLFW_SAMPLES, 4);
    auto ret = glfwCreateWindow(window_width, window_height,
                                window_title.data(), nullptr, nullptr);
    CHECK_SUCCESS(ret != nullptr);
    glfwMakeContextCurrent(ret);
    glewExperimental = GL_TRUE;
    CHECK_SUCCESS(glewInit() == GLEW_OK);
    glGetError();  // clear GLEW's error for it
    glfwSwapInterval(1);
    const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
    const GLubyte* version = glGetString(GL_VERSION);    // version as a string
    std::cout << "Renderer: " << renderer << "\n";
    std::cout << "OpenGL version supported:" << version << "\n";

    return ret;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Input model file is missing" << std::endl;
        std::cerr << "Usage: " << argv[0] << " <PMD file>" << std::endl;
        return -1;
    }
    GLFWwindow* window = init_glefw();
    GUI gui(window, main_view_width, main_view_height, preview_height);

    std::vector<glm::vec4> floor_vertices;
    std::vector<glm::uvec3> floor_faces;
    create_floor(floor_vertices, floor_faces);

    LineMesh cylinder_mesh;
    LineMesh axes_mesh;

    // FIXME: we already created meshes for cylinders. Use them to render
    //        the cylinder and axes if required by the assignment.
    create_cylinder_mesh(cylinder_mesh);
    create_axes_mesh(axes_mesh);

    Mesh mesh;
    mesh.loadPmd(argv[1]);
    std::cout << "Loaded object  with  " << mesh.vertices.size()
              << " vertices and " << mesh.faces.size() << " faces.\n";

    glm::vec4 mesh_center = glm::vec4(0.0f);
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        mesh_center += mesh.vertices[i];
    }
    mesh_center /= mesh.vertices.size();

    int if_show_border = 0;
    int if_show_cursor = 1;
    int sampler = -1;
    std::vector<glm::vec4> quad_vertices;
    std::vector<glm::uvec3> quad_faces;
    std::vector<glm::vec2> quad_indices;
    create_quads(quad_vertices, quad_faces, quad_indices);

    /*
     * GUI object needs the mesh object for bone manipulation.
     */
    gui.assignMesh(&mesh);

    glm::vec4 light_position = glm::vec4(0.0f, 100.0f, 0.0f, 1.0f);
    MatrixPointers mats;  // Define MatrixPointers here for lambda to capture
    float get_frame_shift = 0.0;

    // FIXME: add more lambdas for data_source if you want to use RenderPass.
    //        Otherwise, do whatever you like here
    std::function<const glm::mat4*()> model_data = [&mats]() {
        return mats.model;
    };
    std::function<glm::mat4()> view_data = [&mats]() { return *mats.view; };
    std::function<glm::mat4()> proj_data = [&mats]() {
        return *mats.projection;
    };
    std::function<glm::mat4()> identity_mat = []() { return glm::mat4(1.0f); };
    std::function<glm::vec3()> cam_data = [&gui]() { return gui.getCamera(); };
    std::function<glm::vec4()> lp_data = [&light_position]() {
        return light_position;
    };
    std::function<glm::mat4()> bone_data = [&gui]() {
        return gui.bone_transform();
    };

    glm::mat4 ortho_matrix = glm::mat4(1.0f);
    std::function<glm::mat4()> orthomat_data = [&ortho_matrix]() {
        return ortho_matrix;
    };

    std::function<int()> sampler_data = [&sampler]() { return sampler; };
    std::function<int()> show_border_data = [&if_show_border]() {
        return if_show_border;
    };

    std::function<float()> frame_shift_data = [&gui]() {
        return gui.getFrameShift();
    };

    int num_preview = mesh.key_frames.size();
    std::function<int()> num_preview_data = [&num_preview]() {
        return num_preview;
    };

    std::function<float()> bar_frame_shift_data = [&get_frame_shift]() {
        return get_frame_shift;
    };

    auto std_model =
        std::make_shared<ShaderUniform<const glm::mat4*>>("model", model_data);
    auto bone_transform = make_uniform("bone_transform", bone_data);
    auto floor_model = make_uniform("model", identity_mat);
    auto std_view = make_uniform("view", view_data);
    auto std_camera = make_uniform("camera_position", cam_data);
    auto std_proj = make_uniform("projection", proj_data);
    auto std_light = make_uniform("light_position", lp_data);

    // for preview
    auto frame_shift = make_uniform("frame_shift", frame_shift_data);
    auto bar_frame_shift =
        make_uniform("bar_frame_shift", bar_frame_shift_data);
    auto show_border = make_uniform("show_border", show_border_data);
    auto orthomat = make_uniform("orthomat", orthomat_data);
    auto sampler_2D = make_uniform("sampler", sampler_data);
    auto number_preview = make_uniform("number_preview", num_preview_data);

    std::function<float()> alpha_data = [&gui]() {
        static const float transparet = 0.5;  // Alpha constant goes here
        static const float non_transparet = 1.0;
        if (gui.isTransparent())
            return transparet;
        else
            return non_transparet;
    };
    auto object_alpha = make_uniform("alpha", alpha_data);

    std::function<std::vector<glm::vec3>()> trans_data = [&mesh]() {
        return mesh.getCurrentQ()->transData();
    };
    std::function<std::vector<glm::fquat>()> rot_data = [&mesh]() {
        return mesh.getCurrentQ()->rotData();
    };
    auto joint_trans = make_uniform("joint_trans", trans_data);
    auto joint_rot = make_uniform("joint_rot", rot_data);
    // FIXME: define more ShaderUniforms for RenderPass if you want to use it.
    //        Otherwise, do whatever you like here

    // Floor render pass
    RenderDataInput floor_pass_input;
    floor_pass_input.assign(0, "vertex_position", floor_vertices.data(),
                            floor_vertices.size(), 4, GL_FLOAT);
    floor_pass_input.assignIndex(floor_faces.data(), floor_faces.size(), 3);
    RenderPass floor_pass(
        -1, floor_pass_input,
        {vertex_shader, geometry_shader, floor_fragment_shader},
        {floor_model, std_view, std_proj, std_light}, {"fragment_color"});

    // PMD Model render pass
    // FIXME: initialize the input data at Mesh::loadPmd
    std::vector<glm::vec2>& uv_coordinates = mesh.uv_coordinates;
    RenderDataInput object_pass_input;
    object_pass_input.assign(0, "jid0", mesh.joint0.data(), mesh.joint0.size(),
                             1, GL_INT);
    object_pass_input.assign(1, "jid1", mesh.joint1.data(), mesh.joint1.size(),
                             1, GL_INT);
    object_pass_input.assign(2, "w0", mesh.weight_for_joint0.data(),
                             mesh.weight_for_joint0.size(), 1, GL_FLOAT);
    object_pass_input.assign(3, "vector_from_joint0",
                             mesh.vector_from_joint0.data(),
                             mesh.vector_from_joint0.size(), 3, GL_FLOAT);
    object_pass_input.assign(4, "vector_from_joint1",
                             mesh.vector_from_joint1.data(),
                             mesh.vector_from_joint1.size(), 3, GL_FLOAT);
    object_pass_input.assign(5, "normal", mesh.vertex_normals.data(),
                             mesh.vertex_normals.size(), 4, GL_FLOAT);
    object_pass_input.assign(6, "uv", uv_coordinates.data(),
                             uv_coordinates.size(), 2, GL_FLOAT);
    // TIPS: You won't need vertex position in your solution.
    //       This only serves the stub shader.
    object_pass_input.assign(7, "vert", mesh.vertices.data(),
                             mesh.vertices.size(), 4, GL_FLOAT);
    object_pass_input.assignIndex(mesh.faces.data(), mesh.faces.size(), 3);
    object_pass_input.useMaterials(mesh.materials);
    RenderPass object_pass(-1, object_pass_input,
                           {blending_shader, geometry_shader, fragment_shader},
                           {std_model, std_view, std_proj, std_light,
                            std_camera, object_alpha, joint_trans, joint_rot},
                           {"fragment_color"});

    // stuff for preview
    // RenderPass object for preview
    RenderDataInput preview_pass_input;
    preview_pass_input.assign(0, "vertex_position", quad_vertices.data(),
                              quad_vertices.size(), 4, GL_FLOAT);
    preview_pass_input.assign(1, "tex_coord_in", quad_indices.data(),
                              quad_indices.size(), 2, GL_FLOAT);
    preview_pass_input.assignIndex(quad_faces.data(), quad_faces.size(), 3);
    RenderPass preview_pass(
        -1, preview_pass_input,
        {preview_vertex_shader, nullptr, preview_fragment_shader},
        {orthomat, frame_shift, show_border}, {"fragment_color"});
    // Setup the render pass for drawing bones
    // FIXME: You won't see the bones until Skeleton::joints were properly
    //        initialized
    std::vector<int> bone_vertex_id;
    std::vector<glm::uvec2> bone_indices;
    for (int i = 0; i < (int)mesh.skeleton.joints.size(); i++) {
        bone_vertex_id.emplace_back(i);
    }
    for (const auto& joint : mesh.skeleton.joints) {
        if (joint.parent_index < 0) continue;
        bone_indices.emplace_back(joint.joint_index, joint.parent_index);
    }
    RenderDataInput bone_pass_input;
    bone_pass_input.assign(0, "jid", bone_vertex_id.data(),
                           bone_vertex_id.size(), 1, GL_UNSIGNED_INT);
    bone_pass_input.assignIndex(bone_indices.data(), bone_indices.size(), 2);
    RenderPass bone_pass(-1, bone_pass_input,
                         {bone_vertex_shader, nullptr, bone_fragment_shader},
                         {std_model, std_view, std_proj, joint_trans},
                         {"fragment_color"});

    RenderDataInput cylinder_pass_input;
    cylinder_pass_input.assign(0, "vertex_position",
                               cylinder_mesh.vertices.data(),
                               cylinder_mesh.vertices.size(), 4, GL_FLOAT);
    cylinder_pass_input.assignIndex(cylinder_mesh.indices.data(),
                                    cylinder_mesh.indices.size(), 2);
    RenderPass cylinder_pass(
        -1, cylinder_pass_input,
        {cylinder_vertex_shader, nullptr, cylinder_fragment_shader},
        {std_model, std_view, std_proj, bone_transform}, {"fragment_color"});

    // FIXME: Create the RenderPass objects for bones here.
    //        Otherwise do whatever you like.

    float aspect = 0.0f;
    std::cout << "center = " << mesh.getCenter() << "\n";

    bool draw_floor = true;
    bool draw_skeleton = true;
    bool draw_object = true;
    bool draw_cylinder = true;

    if (argc >= 3) {
        mesh.loadAnimationFrom(argv[2]);
    }

    while (!glfwWindowShouldClose(window)) {
        // Setup some basic window stuff.
        glfwGetFramebufferSize(window, &window_width, &window_height);
        // std::cout << window_height << std::endl;
        glViewport(0, 0, main_view_width * 2, main_view_height * 2);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LESS);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glCullFace(GL_BACK);

        gui.updateMatrices();
        mats = gui.getMatrixPointers();

        if (gui.isPlaying()) {
            std::stringstream title;
            float cur_time = gui.getCurrentPlayTime();
            title << window_title << " Playing: " << std::setprecision(2)
                  << std::setfill('0') << std::setw(6) << cur_time << " s";
            glfwSetWindowTitle(window, title.str().data());
            mesh.updateAnimation(cur_time);
        } else if (gui.isPoseDirty()) {
            mesh.updateAnimation();
            gui.clearPose();
        }

        if (gui.isCreatingFrame()) {
            TextureToRender* texture = new TextureToRender();
            texture->create(preview_width * 2, preview_height * 2);
            texture->bind();
            if (draw_floor) {
                floor_pass.setup();
                CHECK_GL_ERROR(glDrawElements(
                    GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
            }
            if (draw_object) {
                object_pass.setup();
                int mid = 0;
                while (object_pass.renderWithMaterial(mid)) mid++;
            }

            mesh.previews.emplace_back(texture);
            // std::cout << texture->getTexture() << std::endl;
            texture->unbind();
            gui.setCreateFrame(false);
        }

        if (gui.isDeletingFrame()) {
            TextureToRender* texture = mesh.previews[gui.getCurrentFrame()];
            mesh.previews.erase(mesh.previews.begin() + gui.getCurrentFrame());
            delete texture;
            gui.setDelFrame(false);
        }

        if (gui.isUpdatingFrame()) {
            TextureToRender* texture = new TextureToRender();
            texture->create(preview_width, preview_height);
            texture->bind();

            if (draw_floor) {
                floor_pass.setup();
                // Draw our triangles.
                CHECK_GL_ERROR(glDrawElements(
                    GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
            }

            // Draw the model
            if (draw_object) {
                object_pass.setup();
                int mid = 0;
                while (object_pass.renderWithMaterial(mid)) mid++;
            }
            TextureToRender* texture_prev =
                mesh.previews[gui.getCurrentFrame()];
            delete texture_prev;
            mesh.previews[gui.getCurrentFrame()] = texture;
            texture->unbind();
            gui.setUpdateFrame(false);
        }

        if (gui.isInsertingFrame()) {
            TextureToRender* texture = new TextureToRender();
            texture->create(preview_width, preview_height);
            texture->bind();

            if (draw_floor) {
                floor_pass.setup();
                // Draw our triangles.
                CHECK_GL_ERROR(glDrawElements(
                    GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
            }

            // Draw the model
            if (draw_object) {
                object_pass.setup();
                int mid = 0;
                while (object_pass.renderWithMaterial(mid)) mid++;
            }

            std::vector<TextureToRender*>::iterator iterator =
                mesh.previews.begin();
            mesh.previews.insert(iterator + gui.getCurrentFrame(), texture);
            texture->unbind();
            gui.setInsertFrame(false);
        }

        int current_bone = gui.getCurrentBone();

        // Draw bones first.
        if (draw_skeleton && gui.isTransparent()) {
            bone_pass.setup();
            CHECK_GL_ERROR(glDrawElements(GL_LINES, bone_indices.size() * 2,
                                          GL_UNSIGNED_INT, 0));
        }
        draw_cylinder = (current_bone != -1 && gui.isTransparent());

        if (draw_cylinder) {
            cylinder_pass.setup();
            CHECK_GL_ERROR(glDrawElements(GL_LINES,
                                          cylinder_mesh.indices.size() * 2,
                                          GL_UNSIGNED_INT, 0));
        }

        if (draw_floor) {
            floor_pass.setup();
            CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3,
                                          GL_UNSIGNED_INT, 0));
        }

        // Draw the model
        if (draw_object) {
            object_pass.setup();
            int mid = 0;
            while (object_pass.renderWithMaterial(mid)) mid++;
        }

        for (int i = 0; i < (int)mesh.previews.size(); i++) {
            glViewport(main_view_width * 2,
                       (main_view_height - (i + 1) * preview_height +
                        gui.getFrameShift()) *
                           2,
                       preview_width * 2, preview_height * 2);
            sampler = mesh.previews[i]->getTexture();

            preview_pass.setup();
            glBindTexture(GL_TEXTURE_2D, sampler);
            CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, quad_faces.size() * 3,
                                          GL_UNSIGNED_INT, 0));
        }
        glViewport(0, 0, main_view_width * 2, main_view_height * 2);
        num_preview = mesh.key_frames.size();
        get_frame_shift = gui.getFrameShift();
        // CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, quad_faces.size() * 3,
        //   GL_UNSIGNED_INT, 0));
        // Poll and swap.
        glfwPollEvents();
        glfwSwapBuffers(window);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
