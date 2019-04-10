#ifndef TEXTURE_TO_RENDER_H
#define TEXTURE_TO_RENDER_H

/*
 * This is a placeholder class for Milestone 2
 */
class TextureToRender {
   public:
    TextureToRender();
    ~TextureToRender();
    void create(int width, int height);
    void bind();
    void unbind();
    int getTexture() const { return texture_; }

   private:
    int width_, height_;
    unsigned int framebuffer_ = -1;
    unsigned int texture_ = -1;
    unsigned int depth_ = -1;
};

#endif
