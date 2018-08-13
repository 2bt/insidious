#include "edit.hpp"
#include "track_edit.hpp"
#include "gui.hpp"
#include "player.hpp"
#include <algorithm>

namespace {

bool     m_track_select_active;
bool     m_track_select_allow_nil;
uint8_t* m_track_select_value;

uint8_t  m_track          = 1;
int      m_clavier_offset = 36;
int      m_track_page     = 0;

Track    m_copy_track;

void enter_track_select(uint8_t& dst, bool allow_nil) {
    m_track_select_active = true;
    m_track_select_value = &dst;
    m_track_select_allow_nil = allow_nil;
}


} // namespace


void enter_track_select(uint8_t& dst) {
    enter_track_select(dst, true);
}

void enter_track_select() {
    enter_track_select(m_track, false);
}

void    select_track(uint8_t t) { m_track = t; }
uint8_t selected_track() { return m_track; }



void sprint_track_id(char* dst, int nr) {
    if (nr == 0) {
        dst[0] = dst[1] == ' ';
    }
    else {
        int x = (nr - 1) / 21;
        int y = (nr - 1) % 21;
        dst[0] = '0' + x + (x > 9) * 7;
        dst[1] = '0' + y + (y > 9) * 7;
    }
    dst[2] = '\0';
}


bool draw_track_select() {
    if (!m_track_select_active) return false;

    gfx::font(FONT_DEFAULT);
    auto widths = calculate_column_widths({ -1, -1 });
    gui::min_item_size({ widths[0], 88 });
    if (gui::button("Cancel")) {
        m_track_select_active = false;
    }
    gui::same_line();
    if (m_track_select_allow_nil) {
        gui::min_item_size({ widths[1], 88 });
        if (gui::button("Clear")) {
            m_track_select_active = false;
            *m_track_select_value = 0;
        }
    }
    else {
        gui::padding({});
    }

    gui::separator();

    gfx::font(FONT_MONO);
    widths = calculate_column_widths(std::vector<int>(12, -1));
    int track_nr = *m_track_select_value;

    for (int y = 0; y < 21; ++y) {
        for (int x = 0; x < 12; ++x) {
            if (x > 0) gui::same_line();
            int n = x * 21 + y + 1;

            gui::min_item_size({ widths[x], 65 });
            char str[3] = "  ";
            sprint_track_id(str, n);
            if (gui::button(str, n == track_nr)) {
                *m_track_select_value = n;
                m_track_select_active = false;
            }
        }
    }

    return true;
}


enum {
    PAGE_LENGTH = 16
};


void draw_track_view() {

    auto widths = calculate_column_widths({ 88, 88, 88, -1, 88, 88 });

    // track select
    gfx::font(FONT_MONO);
    gui::min_item_size({ 88, 88 });
    if (gui::button("-")) m_track = std::max(1, m_track - 1);
    char str[3];
    sprint_track_id(str, m_track);
    gui::min_item_size({ 88, 88 });
    gui::same_line();
    if (gui::button(str)) enter_track_select();
    gui::min_item_size({ 88, 88 });
    gui::same_line();
    if (gui::button("+")) m_track = std::min<int>(TRACK_COUNT, m_track + 1);

    assert(m_track > 0);
    Track& track = player::song().tracks[m_track - 1];

    gui::same_line();
    gui::padding({ widths[3], 0 });

    // copy & paste
    gfx::font(FONT_MONO);
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("C")) m_copy_track = track;
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("P")) track = m_copy_track;



    // clavier slider
    gui::separator();
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 65 });
    gui::drag_int("", "", m_clavier_offset, 0, 96 - CLAVIER_WIDTH, CLAVIER_WIDTH);

    gui::same_line();
    Vec c1 = gui::cursor() + Vec(-65 - gui::PADDING, 65 + gui::PADDING * 2 + gui::SEPARATOR_WIDTH);
    gui::next_line();

    int player_row = player::row();
    gfx::font(FONT_MONO);
    for (int i = 0; i < PAGE_LENGTH; ++i) {
        if (i % 4 == 0) gui::separator();

        int row_nr = m_track_page * PAGE_LENGTH + i;
        bool highlight = row_nr == player_row;
        Track::Row& row = track.rows[row_nr];

        char str[4];

        // instrument
        edit::sprint_inst_effect_id(str, row.instrument);
        gui::min_item_size({ 65, 65 });
        if (highlight) gui::highlight();
        if (gui::button(str)) {
            if (row.instrument > 0) row.instrument = 0;
            else row.instrument = edit::selected_instrument();
        }
        if (row.instrument > 0 && gui::hold()) {
            edit::select_instrument(row.instrument);
        }

        // effect
        edit::sprint_inst_effect_id(str, row.effect);
        gui::same_line();
        gui::min_item_size({ 65, 65 });
        if (highlight) gui::highlight();
        if (gui::button(str)) {
            if (row.effect > 0) row.effect = 0;
            else row.effect = edit::selected_effect();
        }
        if (row.effect > 0 && gui::hold()) {
            edit::select_effect(row.effect);
        }


        // note
        str[0] = str[1] = str[2] = ' ';
        if (row.note == 255) {
            str[0] = str[1] = str[2] = '-';
        }
        else if (row.note > 0) {
            str[0] = "CCDDEFFGGAAB"[(row.note - 1) % 12];
            str[1] = "-#-#--#-#-#-"[(row.note - 1) % 12];
            str[2] = '0' + (row.note - 1) / 12;
        }
        str[3] = '\0';
        gui::min_item_size({ 0, 65 });
        gui::same_line();
        if (highlight) gui::highlight();
        if (gui::button(str)) {
            if (row.note == 0) row.note = 255;
            else {
                row = {};
            }
        }

        // clavier
        gui::same_line();
        gui::separator();
        int w = gfx::screensize().x - gui::cursor().x - gui::PADDING * 4 - 65 - gui::SEPARATOR_WIDTH;
        gui::min_item_size({ w, 65 });
        if (gui::clavier(row.note, m_clavier_offset, highlight)) {
            if (row.note == 0) row = {};
            else if (row.instrument == 0 && row.effect == 0) {
                row.instrument = edit::selected_instrument();
                row.effect     = edit::selected_effect();
            }
        }

        gui::same_line();
        gui::separator();
        gui::next_line();
    }

    // track pages
    Vec c2 = gui::cursor();
    gui::cursor(c1);
    gui::min_item_size({ 65, c2.y - c1.y - gui::PADDING });
    gui::vertical_drag_int(m_track_page, 0, TRACK_LENGTH / PAGE_LENGTH - 1);
    gui::cursor(c2);


    // cache
    gui::separator();
    edit::draw_instrument_cache();
    gui::separator();
    edit::draw_effect_cache();
    gui::separator();
}

