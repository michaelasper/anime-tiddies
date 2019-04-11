#include "texture_to_render.h"
#include <GL/glew.h>
#include <debuggl.h>
#include <iostream>

TextureToRender::TextureToRender() {}

TextureToRender::~TextureToRender() {
    if (fb_ < 0) return;

    unbind();

    glDeleteFramebuffers(1, &fb_);
    glDeleteTextures(1, &tex_);
    glDeleteRenderbuffers(1, &dep_);

    release();
    std::cout << "Hi" << std::endl;
}

void TextureToRender::create(int width, int height) {
    w_ = width;
    h_ = height;
    // FIXME: Create the framebuffer object backed by a texture

    // create the frame buffer
    glGenFramebuffers(1, &fb_);
    glBindFramebuffer(GL_FRAMEBUFFER, fb_);

    glGenTextures(1, &tex_);

    glBindTexture(GL_TEXTURE_2D, tex_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w_, h_, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 NULL);

    glGenRenderbuffers(1, &dep_);
    glBindRenderbuffer(GL_RENDERBUFFER, dep_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w_, h_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, dep_);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           tex_, 0);

    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Failed to create framebuffer object as render target"
                  << std::endl;
    } else {
        std::cerr << "Framebuffer ready" << std::endl;
    }
    unbind();
}

void TextureToRender::bind() {
    // FIXME: Unbind the framebuffer object to GL_FRAMEBUFFER
    glBindFramebuffer(GL_FRAMEBUFFER, fb_);
    glViewport(0, 0, w_, h_);
    // glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    // glClearDepth(1.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void TextureToRender::unbind() {
    // FIXME: Unbind current framebuffer object from the render target
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // glClearColor(0.0, 0.0, 0.0, 0.0);

    // glClearDepth(1.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void TextureToRender::release() {
    if (fb_ < 0) return;

    unbind();

    glDeleteFramebuffers(1, &fb_);
    glDeleteTextures(1, &tex_);
    glDeleteRenderbuffers(1, &dep_);

    fb_ = 0;
    tex_ = 0;
    dep_ = 0;
}