#include "textbox.h"

using namespace widgets;

textbox::textbox(const textbox_style &style)
    : m_style(style)
    , m_font(media_pak::get().*(style.text.text_font), style.text.text_ptsize) {}

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

static int measure_cursor(const sdl::font &font, const std::string &text, int xdiff) {
    int extent, count;
    TTF_MeasureUTF8(font.get(), text.c_str(), xdiff, &extent, &count);
    return count;
}

static int get_text_width(const sdl::font &font, const std::string &text) {
    int x, y;
    TTF_SizeUTF8(font.get(), text.c_str(), &x, &y);
    return x;
}

void textbox::tick(duration_type time_elapsed) {
    m_timer += time_elapsed;
}

void textbox::render(sdl::renderer &renderer) {
    renderer.set_draw_color(m_style.background_color);
    renderer.fill_rect(m_border_rect);

    renderer.set_draw_color(m_style.border_color);
    renderer.draw_rect(m_border_rect);

    const sdl::rect m_crop{
        m_border_rect.x + m_style.margin,
        m_border_rect.y + m_style.margin,
        m_border_rect.w - m_style.margin * 2,
        m_border_rect.h - m_style.margin * 2
    };

    int linex = m_crop.x;

    if (m_tex) {
        linex = get_text_width(m_font, unicode_substring(m_value, 0, m_cursor_pos));
        if (linex < m_hscroll) {
            m_hscroll = linex;
        } else if (linex > m_hscroll + m_crop.w) {
            m_hscroll = linex - m_crop.w;
        }

        sdl::rect src_rect = m_tex.get_rect();
        sdl::rect dst_rect = src_rect;
        if (src_rect.w < m_crop.w) {
            m_hscroll = 0;
        }

        dst_rect.x = m_crop.x - m_hscroll;
        dst_rect.y = m_crop.y;

        linex += dst_rect.x;
        if (dst_rect.x < m_crop.x) {
            int diff = m_crop.x - dst_rect.x;
            src_rect.x += diff;
            src_rect.w -= diff;
            dst_rect.x += diff;
            dst_rect.w -= diff;
        }

        if (dst_rect.x + dst_rect.w > m_crop.x + m_crop.w) {
            int diff = dst_rect.x + dst_rect.w - m_crop.x - m_crop.w;
            src_rect.w -= diff;
            dst_rect.w -= diff;
        }
    
        if (focused() && m_cursor_len) {
            int linew = get_text_width(m_font, unicode_substring(m_value, m_cursor_pos, m_cursor_len));
            if (m_cursor_len < 0) {
                linew = -linew;
            }

            int min = linex;
            int max = linex + linew;
            if (max < min) {
                std::swap(min, max);
            }
            min = std::max(min, m_crop.x);
            max = std::min(max, m_crop.x + m_crop.w);

            renderer.set_draw_color(m_style.selection_color);
            renderer.fill_rect(sdl::rect{min, m_border_rect.y + 1, max - min, m_border_rect.h - 2});
        }

        SDL_RenderCopy(renderer.get(), m_tex.get_texture(renderer).get(), &src_rect, &dst_rect);
    }

    if (focused() && (m_timer % m_style.cycle_duration) < (m_style.cycle_duration / 2)) {
        renderer.set_draw_color(m_style.border_color);
        SDL_RenderDrawLine(renderer.get(), linex, m_crop.y, linex, m_crop.y + m_crop.h);
    }
}

void textbox::on_gain_focus() {
    m_timer = duration_type{0};
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
            if (m_value.empty()) {
                m_cursor_pos = 0;
            } else {
                m_cursor_pos = measure_cursor(m_font, m_value, event.button.x - (m_border_rect.x + m_style.margin - m_hscroll));
            }
            m_cursor_len = 0;
            m_mouse_down = true;
            m_timer = duration_type{0};
            return true;
        }
        return false;
    case SDL_MOUSEMOTION:
        if (m_mouse_down && !m_value.empty()) {
            int pos = m_cursor_pos;
            m_cursor_pos = measure_cursor(m_font, m_value, event.button.x - (m_border_rect.x + m_style.margin - m_hscroll));
            m_cursor_len += pos - m_cursor_pos;
            m_timer = duration_type{0};
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
                        std::string text = unicode_substring(m_value, m_cursor_pos, m_cursor_len);
                        unicode_erase_at(m_value, m_cursor_pos, m_cursor_len);
                        SDL_SetClipboardText(text.c_str());
                        if (m_cursor_len < 0) {
                            m_cursor_pos += m_cursor_len;
                        }
                        m_cursor_len = 0;
                        m_timer = duration_type{0};
                        redraw();
                    }
                }
                return true;
            case SDLK_c:
                if (event.key.keysym.mod & KMOD_CTRL) {
                    if (m_cursor_len) {
                        std::string text = unicode_substring(m_value, m_cursor_pos, m_cursor_len);
                        SDL_SetClipboardText(text.c_str());
                    }
                }
                return true;
            case SDLK_v:
                if (event.key.keysym.mod & KMOD_CTRL) {
                    if (char *clipboard = SDL_GetClipboardText()) {
                        if (m_cursor_len) {
                            unicode_erase_at(m_value, m_cursor_pos, m_cursor_len);
                            if (m_cursor_len < 0) {
                                m_cursor_pos += m_cursor_len;
                            }
                            m_cursor_len = 0;
                        }
                        unicode_append_at(m_value, clipboard, m_cursor_pos);
                        m_cursor_pos += unicode_count_chars(clipboard);
                        redraw();
                    }
                }
                return true;
            case SDLK_a:
                if (event.key.keysym.mod & KMOD_CTRL) {
                    m_cursor_pos = unicode_count_chars(m_value);
                    m_cursor_len = -m_cursor_pos;
                    m_timer = duration_type{0};
                }
                return true;
            case SDLK_BACKSPACE:
                if (!m_value.empty()) {
                    if (m_cursor_len) {
                        unicode_erase_at(m_value, m_cursor_pos, m_cursor_len);
                        if (m_cursor_len < 0) {
                            m_cursor_pos += m_cursor_len;
                        }
                        m_cursor_len = 0;
                        m_timer = duration_type{0};
                    } else if (m_cursor_pos > 0) {
                        unicode_erase_at(m_value, m_cursor_pos - 1, 1);
                        --m_cursor_pos;
                        m_timer = duration_type{0};
                    }
                    redraw();
                }
                return true;
            case SDLK_DELETE:
                if (!m_value.empty()) {
                    if (m_cursor_len) {
                        unicode_erase_at(m_value, m_cursor_pos, m_cursor_len);
                        if (m_cursor_len < 0) {
                            m_cursor_pos += m_cursor_len;
                        }
                        m_cursor_len = 0;
                        m_timer = duration_type{0};
                    } else {
                        unicode_erase_at(m_value, m_cursor_pos, 1);
                    }
                    redraw();
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
                m_timer = duration_type{0};
                return true;
            case SDLK_RIGHT:
                if (event.key.keysym.mod & KMOD_SHIFT) {
                    if (m_cursor_pos < unicode_count_chars(m_value)) {
                        ++m_cursor_pos;
                        --m_cursor_len;
                    }
                } else if (m_cursor_len > 0) {
                    m_cursor_pos += m_cursor_len;
                    m_cursor_len = 0;
                } else if (m_cursor_len < 0) {
                    m_cursor_len = 0;
                } else if (m_cursor_pos < unicode_count_chars(m_value)) {
                    ++m_cursor_pos;
                }
                m_timer = duration_type{0};
                return true;
            case SDLK_HOME:
                if (event.key.keysym.mod & KMOD_SHIFT) {
                    m_cursor_len += m_cursor_pos;
                } else {
                    m_cursor_len = 0;
                }
                m_cursor_pos = 0;
                m_timer = duration_type{0};
                return true;
            case SDLK_END: {
                int len = unicode_count_chars(m_value);
                if (event.key.keysym.mod & KMOD_SHIFT) {
                    m_cursor_len += m_cursor_pos - len;
                } else {
                    m_cursor_len = 0;
                }
                m_cursor_pos = len;
                m_timer = duration_type{0};
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
                unicode_erase_at(m_value, m_cursor_pos, m_cursor_len);
                if (m_cursor_len < 0) {
                    m_cursor_pos += m_cursor_len;
                }
                m_cursor_len = 0;
            }
            unicode_append_at(m_value, event.text.text, m_cursor_pos);
            ++m_cursor_pos;
            redraw();
            m_timer = duration_type{0};
            return true;
        }
        return false;
    default:
        return false;
    }
}