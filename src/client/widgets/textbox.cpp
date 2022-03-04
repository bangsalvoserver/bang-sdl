#include "textbox.h"

using namespace widgets;

textbox::textbox(const textbox_style &style) : m_style (style) {}

inline bool is_first_utf8_char(char c) {
    return (c & 0xc0) != 0x80;
}

static int unicode_count_chars(std::string_view str) {
    size_t count = 0;
    for (auto c = str.begin(); c != str.end(); ++c) {
        count += is_first_utf8_char(*c);
    }
    return count;
}

static void unicode_append_at(std::string &str, std::string_view str2, int pos) {
    auto c = str.begin();
    for (size_t count = 0; c != str.end(); ++c) {
        count += is_first_utf8_char(*c);
        if (count > pos) {
            break;
        }
    }
    str.insert(c - str.begin(), str2);
}

static auto unicode_iterator_pair(const std::string &str, int pos, int len) {
    if (len < 0) {
        pos += len;
        len = -len;
    }
    auto begin = str.begin();
    for (int count = 0; begin != str.end(); ++begin) {
        count += is_first_utf8_char(*begin);
        if (count > pos) {
            break;
        }
    }
    auto end = begin;
    for (int count = 0; end != str.end(); ++end) {
        count += is_first_utf8_char(*end);
        if (count > len) {
            break;
        }
    }
    return std::pair{begin, end};
}

static void unicode_erase_at(std::string &str, int pos, int len) {
    auto [begin, end] = unicode_iterator_pair(str, pos, len);
    str.erase(begin, end);
}

static std::string unicode_substring(const std::string &str, int pos, int len) {
    auto [begin, end] = unicode_iterator_pair(str, pos, len);
    return std::string{begin, end};
}

static void unicode_resize(std::string &str, int size) {
    size_t count = 0;
    for (auto c = str.begin(); c != str.end(); ++c) {
        count += is_first_utf8_char(*c);
        if (count > size) {
            str.resize(c - str.begin());
            break;
        }
    }
}

static int measure_cursor(const sdl::font &font, const std::string &text, int xdiff) {
    int extent, count;
    TTF_MeasureUTF8(font.get(), text.c_str(), xdiff, &extent, &count);
    return count;
}

static int get_cursor_pos(const sdl::font &font, std::string text, int cursor_pos) {
    int x, y;
    unicode_resize(text, cursor_pos);
    TTF_SizeUTF8(font.get(), text.c_str(), &x, &y);
    return x;
}

static int get_cursor_len(const sdl::font &font, const std::string &text, int cursor_pos, int cursor_len) {
    int x, y;
    std::string resized = unicode_substring(text, cursor_pos, cursor_len);
    TTF_SizeUTF8(font.get(), resized.c_str(), &x, &y);
    return x;
}

void textbox::render(sdl::renderer &renderer) {
    renderer.set_draw_color(m_style.background_color);
    renderer.fill_rect(m_border_rect);

    renderer.set_draw_color(m_style.border_color);
    renderer.draw_rect(m_border_rect);

    int linex = m_border_rect.x + m_style.margin;
    int linew = 0;

    if (m_tex) {
        m_tex.set_point(sdl::point{m_border_rect.x + m_style.margin, m_border_rect.y + m_style.margin});
        m_tex.render_cropped(renderer, sdl::rect{
            m_border_rect.x + m_style.margin,
            m_border_rect.y + m_style.margin,
            m_border_rect.w - 2 * m_style.margin,
            m_border_rect.h
        });

        linex = m_tex.get_rect().x + get_cursor_pos(m_tex.get_font(), m_tex.m_value, m_cursor_pos);
        if (m_cursor_len) {
            linew = get_cursor_len(m_tex.get_font(), m_tex.m_value, m_cursor_pos, m_cursor_len);
        }
    }
    
    if (linew) {
        renderer.set_draw_color(sdl::rgba(0x00ffff80));
        if (m_cursor_len > 0) {
            renderer.fill_rect(sdl::rect{linex, m_border_rect.y, linew, m_border_rect.h});
        } else {
            renderer.fill_rect(sdl::rect{linex - linew, m_border_rect.y, linew, m_border_rect.h});
        }
    }

    if (focused() && (m_ticks++ % 50) < 25 && linex > m_border_rect.x && linex < m_border_rect.x + m_border_rect.w) {
        renderer.set_draw_color(sdl::rgb(0x0));
        SDL_RenderDrawLine(renderer.get(), linex, m_border_rect.y + m_style.margin, linex, m_border_rect.y + m_border_rect.h - m_style.margin);
    }
}

void textbox::on_gain_focus() {
    m_ticks = 0;
    SDL_StartTextInput();
}

void textbox::on_lose_focus() {
    SDL_StopTextInput();
}

bool textbox::handle_event(const sdl::event &event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT && sdl::point_in_rect(sdl::point{event.button.x, event.button.y}, m_border_rect)) {
            set_focus(this);
            if (m_tex.m_value.empty()) {
                m_cursor_pos = 0;
            } else {
                m_cursor_pos = measure_cursor(m_tex.get_font(), m_tex.m_value, event.button.x - m_tex.get_rect().x);
            }
            m_cursor_len = 0;
            m_mouse_down = true;
            return true;
        }
        return false;
    case SDL_MOUSEMOTION:
        if (m_mouse_down && !m_tex.m_value.empty()) {
            int pos = m_cursor_pos;
            m_cursor_pos = measure_cursor(m_tex.get_font(), m_tex.m_value, event.button.x - m_tex.get_rect().x);
            m_cursor_len += pos - m_cursor_pos;
            return true;
        }
        return false;
    case SDL_MOUSEBUTTONUP:
        m_mouse_down = false;
        return false;
    case SDL_KEYDOWN:
        if (focused() && !m_mouse_down) {
            switch (event.key.keysym.sym) {
            case SDLK_x:
                if (event.key.keysym.mod & KMOD_CTRL) {
                    if (m_cursor_len) {
                        std::string text = unicode_substring(m_tex.m_value, m_cursor_pos, m_cursor_len);
                        unicode_erase_at(m_tex.m_value, m_cursor_pos, m_cursor_len);
                        SDL_SetClipboardText(text.c_str());
                        if (m_cursor_len < 0) {
                            m_cursor_pos += m_cursor_len;
                        }
                        m_cursor_len = 0;
                        m_tex.redraw();
                    }
                }
                return true;
            case SDLK_c:
                if (event.key.keysym.mod & KMOD_CTRL) {
                    if (m_cursor_len) {
                        std::string text = unicode_substring(m_tex.m_value, m_cursor_pos, m_cursor_len);
                        SDL_SetClipboardText(text.c_str());
                    }
                }
                return true;
            case SDLK_v:
                if (event.key.keysym.mod & KMOD_CTRL) {
                    if (char *clipboard = SDL_GetClipboardText()) {
                        if (m_cursor_len) {
                            unicode_erase_at(m_tex.m_value, m_cursor_pos, m_cursor_len);
                            if (m_cursor_len < 0) {
                                m_cursor_pos += m_cursor_len;
                            }
                            m_cursor_len = 0;
                        }
                        unicode_append_at(m_tex.m_value, clipboard, m_cursor_pos);
                        m_cursor_pos += unicode_count_chars(clipboard);
                        m_tex.redraw();
                    }
                }
                return true;
            case SDLK_a:
                if (event.key.keysym.mod & KMOD_CTRL) {
                    m_cursor_pos = 0;
                    m_cursor_len = unicode_count_chars(m_tex.m_value);
                }
                return true;
            case SDLK_BACKSPACE:
                if (!m_tex.m_value.empty()) {
                    if (m_cursor_len) {
                        unicode_erase_at(m_tex.m_value, m_cursor_pos, m_cursor_len);
                        if (m_cursor_len < 0) {
                            m_cursor_pos += m_cursor_len;
                        }
                        m_cursor_len = 0;
                    } else if (m_cursor_pos > 0) {
                        unicode_erase_at(m_tex.m_value, m_cursor_pos - 1, 1);
                        --m_cursor_pos;
                    }
                    m_tex.redraw();
                }
                return true;
            case SDLK_DELETE:
                if (!m_tex.m_value.empty()) {
                    if (m_cursor_len) {
                        unicode_erase_at(m_tex.m_value, m_cursor_pos, m_cursor_len);
                        if (m_cursor_len < 0) {
                            m_cursor_pos += m_cursor_len;
                        }
                        m_cursor_len = 0;
                    } else {
                        unicode_erase_at(m_tex.m_value, m_cursor_pos, 1);
                    }
                    m_tex.redraw();
                }
                return true;
            case SDLK_LEFT:
                if (event.key.keysym.mod & KMOD_SHIFT) {
                    if (m_cursor_pos > 0) {
                        ++m_cursor_len;
                        --m_cursor_pos;
                    }
                } else if (m_cursor_len > 0) {
                    m_cursor_len = 0;
                } else if (m_cursor_len < 0) {
                    m_cursor_pos += m_cursor_len;
                    m_cursor_len = 0;
                } else if (m_cursor_pos > 0) {
                    --m_cursor_pos;
                }
                return true;
            case SDLK_RIGHT:
                if (event.key.keysym.mod & KMOD_SHIFT) {
                    if (m_cursor_pos < unicode_count_chars(m_tex.m_value)) {
                        ++m_cursor_pos;
                        --m_cursor_len;
                    }
                } else if (m_cursor_len > 0) {
                    m_cursor_pos += m_cursor_len;
                    m_cursor_len = 0;
                } else if (m_cursor_len < 0) {
                    m_cursor_len = 0;
                } else if (m_cursor_pos < unicode_count_chars(m_tex.m_value)) {
                    ++m_cursor_pos;
                }
                return true;
            case SDLK_HOME:
                if (event.key.keysym.mod & KMOD_SHIFT) {
                    m_cursor_len += m_cursor_pos;
                } else {
                    m_cursor_len = 0;
                }
                m_cursor_pos = 0;
                return true;
            case SDLK_END: {
                int len = unicode_count_chars(m_tex.m_value);
                if (event.key.keysym.mod & KMOD_SHIFT) {
                    m_cursor_len += m_cursor_pos - len;
                } else {
                    m_cursor_len = 0;
                }
                m_cursor_pos = len;
                return true;
            }
            case SDLK_RETURN:
                if (on_enter) {
                    on_enter();
                    return true;
                }
            }
        }
        return false;
    case SDL_TEXTINPUT:
        if (focused() && !m_mouse_down && event.text.text[0] != '\n') {
            if (m_cursor_len) {
                unicode_erase_at(m_tex.m_value, m_cursor_pos, m_cursor_len);
                if (m_cursor_len < 0) {
                    m_cursor_pos += m_cursor_len;
                }
                m_cursor_len = 0;
            }
            unicode_append_at(m_tex.m_value, event.text.text, m_cursor_pos);
            ++m_cursor_pos;
            m_tex.redraw();
            return true;
        }
        return false;
    default:
        return false;
    }
}