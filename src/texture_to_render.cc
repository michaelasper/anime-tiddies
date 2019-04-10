#include "texture_to_render.h"
#include <GL/glew.h>
#include <debuggl.h>
#include <iostream>

TextureToRender::TextureToRender() {}

TextureToRender::~TextureToRender() {
    if (framebuffer_ < 0) return;
    unbind();
    glDeleteFramebuffers(1, &framebuffer_);
    glDeleteTextures(1, &texture_);
    glDeleteRenderbuffers(1, &depth_);
}

void TextureToRender::create(int width, int height) {
    width_ = width;
    height_ = height;

    // frame buffer stuff
    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

    // texture stuff
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width_, height_, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // the depth buffer
    glGenRenderbuffers(1, &depth_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depth_);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_, 0);
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers);

    unbind();
}

void TextureToRender::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, width_, height_);
}

void TextureToRender::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }