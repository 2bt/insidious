#include "gfx.hpp"
#include <cstdarg>

#define DOWNSCALE


namespace gfx {
namespace {


SDL_Window*   m_window     = nullptr;
SDL_Renderer* m_renderer   = nullptr;
//Vec           m_screensize = { 1080, 1920 - 115 };
Vec           m_screensize = { 1080, 1920 };
FontID        m_font       = FONT_DEFAULT;


} // namespace


SDL_Renderer* renderer() { return m_renderer; }
SDL_Window*   window() { return m_window; }
Vec const&    screensize() { return m_screensize; }
void font(FontID font) { m_font = font; }
void present() { SDL_RenderPresent(m_renderer); }


bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;

    m_window = SDL_CreateWindow("fakesid",
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
//                                m_screensize.x / 2, m_screensize.y / 2, 0);
                                m_screensize.x, m_screensize.y,
                                SDL_WINDOW_RESIZABLE);
    if (!m_window) return false;

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) return false;

#ifdef DOWNSCALE
    SDL_RenderSetLogicalSize(m_renderer, m_screensize.x, m_screensize.y);
#endif

    if (!resource::init()) return false;

    return true;
}


void free() {
    resource::free();
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}


bool process_event(const SDL_Event& e) {
    switch (e.type) {
#ifndef DOWNSCALE
    case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            m_screensize = { e.window.data1, e.window.data2 };
        }
        return true;
#endif
    default:
        return false;
    }
}


void clear() {
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
}


void render(TextureID tex, SDL_Rect const& src, SDL_Rect const& dst, int flip) {
    if (flip == 0) {
        SDL_RenderCopy(m_renderer, resource::texture(tex), &src, &dst);
    }
    else {
        SDL_RenderCopyEx(m_renderer,
                         resource::texture(tex),
                         &src,
                         &dst,
                         0,
                         nullptr,
                         (SDL_RendererFlip) flip);
    }
}


int glyph_width(char c) {
    FontSpec const& spec = resource::font_spec(m_font);
    int i = glm::clamp((int) c, 16, 127);
    return c < 32 ? spec.width - 1 : spec.spacing[i - 32];
}


Vec text_size(char const* str) {
    FontSpec const& spec = resource::font_spec(m_font);
    Vec size = { 0, spec.height };
    int width = 0;
    while (int c = *str++) {
        if (c == '\n') {
            size.y += spec.height;
            width = 0;
            continue;
        }
        width += glyph_width(c);
        size.x = std::max(size.x, width);
    }
    return size;
}


void print(Vec const& pos, char const* str) {
    FontSpec const& spec = resource::font_spec(m_font);
    SDL_Rect src = { 0, 0, 0, spec.height };
    SDL_Rect dst = { pos.x, pos.y, 0, spec.height };
    while (int c = *str++) {
        if (c == '\n') {
            dst.x = pos.x;
            dst.y += spec.height;
            continue;
        }
        int i = glm::clamp(c, 16, 127);
        int width = glyph_width(c);
        src.w     = dst.w = c < 32 ? spec.width : spec.spacing[i - 32];
        src.x = i % 16 * spec.width;
        src.y = i / 16 * spec.height;
        render(spec.texture, src, dst);
        dst.x += width;
    }
}


void printf(Vec const& pos, char const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char line[256] = {};
    vsnprintf(line, 256, fmt, args);
    va_end(args);
    print(pos, line);
}


void color(SDL_Color color) {
    FontSpec const& spec = resource::font_spec(m_font);
    SDL_Texture* t = resource::texture(spec.texture);
    SDL_SetTextureColorMod(t, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(t, color.a);
}

void clip_rectangle(Vec const& pos, Vec const& size) {
    SDL_Rect rect = { pos.x, pos.y, size.x, size.y };
    SDL_RenderSetClipRect(m_renderer, &rect);
}


void clear_clip_rectangle() {
    SDL_RenderSetClipRect(m_renderer, nullptr);
}


void rectangle(Vec const& pos, Vec const& size) {
    FontSpec const& spec = resource::font_spec(m_font);
    SDL_Rect src = { 0, 0, 1, 1 };
    SDL_Rect dst = { pos.x, pos.y, size.x, size.y };
    render(spec.texture, src, dst);
}

void rectangle(Vec const& pos, Vec const& size, int style) {
    if (style == 0) rectangle(pos, size);

    // tab style 8 - 16
    int style_bottom = style;
    if (style >= 8) {
        style -= 8;
        style_bottom = style / 4 * 4;
    }

    // size must be >= { 64, 64 }
    Vec t = glm::min(Vec(32), size / 2);
    FontSpec const& spec = resource::font_spec(m_font);

    SDL_Rect src = { 36 * style, 0, t.x, t.y };
    SDL_Rect dst = { pos.x, pos.y, t.x, t.y };
    render(spec.texture, src, dst);
    dst.x = pos.x + size.x - t.x;
    render(spec.texture, src, dst, SDL_FLIP_HORIZONTAL);
    dst.y = pos.y + size.y - t.y;
    src.x = 36 * style_bottom;
    render(spec.texture, src, dst, SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
    dst.x = pos.x;
    render(spec.texture, src, dst, SDL_FLIP_VERTICAL);

    src = { 36 * style + 31, 0, 1, t.y };
    dst = { pos.x + t.x, pos.y, size.x - t.x * 2, t.y };
    render(spec.texture, src, dst);
    dst = { pos.x + t.x, pos.y + size.y - t.y, size.x - t.x * 2, t.y };
    render(spec.texture, src, dst, SDL_FLIP_VERTICAL);

    src = { 36 * style, 31, t.x, 1 };
    dst = { pos.x, pos.y + t.y, t.x, size.y - t.y * 2 };
    render(spec.texture, src, dst);
    dst = { pos.x + size.x - t.x, pos.y + t.y, t.x, size.y - t.y * 2 };
    render(spec.texture, src, dst, SDL_FLIP_HORIZONTAL);


    if (style < 4) {
        rectangle(pos + t, size - t * 2);
    }
}


} // namespace
