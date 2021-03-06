#include "edit.hpp"
#include "track_view.hpp"
#include "gui.hpp"
#include "player.hpp"
#include <algorithm>

namespace {


struct Cache {
    Cache() {
        for (int i = 0; i < SIZE; ++i) {
            entries[i].age  = i;
            entries[i].data = i + 1;
        }
    }
    enum { SIZE = 12 };
    struct Entry {
        uint32_t age;
        int      data;
        bool operator<(Entry const& rhs) const {
            return data < rhs.data;
        }
    };
    std::array<Entry, SIZE> entries;
    void insert(int data) {
        Entry* f = &entries[0];
        for (Entry& e : entries) {
            if (e.data == data) e.age = -1;
            else ++e.age;
            if (e.age > f->age) {
                f = &e;
            }
        }
        f->data = data;
        f->age = 0;
        std::sort(entries.begin(), entries.end());
    }
};


void draw_cache(Cache& cache, int& data) {
    gfx::font(FONT_MONO);
    auto widths = calculate_column_widths(std::vector<int>(Cache::SIZE, -1));
    for (int i = 0; i < Cache::SIZE; ++i) {
        if (i) gui::same_line();
        auto const& e = cache.entries[i];
        char str[2];
        sprint_inst_effect_id(str, e.data);
        gui::min_item_size({ widths[i], BUTTON_BIG });
        if (gui::button(str, e.data == data)) {
            data = e.data;
            cache.insert(data);
        }
    }
}



uint8_t  m_track          = 1;
int      m_clavier_offset = 36;
int      m_track_page     = 0;

Track    m_copy_track;

Cache    m_inst_cache;
Cache    m_effect_cache;
int      m_instrument = 1;
int      m_effect     = 1;


std::array<bool, TRACK_COUNT> m_empty_tracks;
bool                          m_track_select_allow_nil;
uint8_t*                      m_track_select_value;


void draw_track_select() {

    gfx::font(FONT_DEFAULT);
    auto widths = m_track_select_allow_nil ? calculate_column_widths({ -1, -1 }) : calculate_column_widths({ -1 });
    gui::min_item_size({ widths[0], BUTTON_BIG });
    if (gui::button("Cancel")) {
        edit::set_popup(nullptr);
    }
    gui::same_line();
    if (m_track_select_allow_nil) {
        gui::min_item_size({ widths[1], BUTTON_BIG });
        if (gui::button("Clear")) {
            edit::set_popup(nullptr);
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

            gui::min_item_size({ widths[x], BUTTON_SMALL });
            char str[3] = "  ";
            sprint_track_id(str, n);

            if (!m_empty_tracks[n - 1]) gui::highlight();
            if (gui::button(str, n == track_nr)) {
                *m_track_select_value = n;
                edit::set_popup(nullptr);
            }
        }
    }
}


void enter_track_select(uint8_t& dst, bool allow_nil) {
    edit::set_popup(draw_track_select);
    m_track_select_value = &dst;
    m_track_select_allow_nil = allow_nil;

    Song const& song = player::song();
    for (int i = 0; i < (int) m_empty_tracks.size(); ++i) {
        m_empty_tracks[i] = true;
        for (Track::Row const& row : song.tracks[i].rows) {
            if (row.note || row.instrument || row.effect) {
                m_empty_tracks[i] = false;
                break;
            }
        }
    }
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
int     selected_instrument() { return m_instrument; }
void    select_instrument(int i) {
    m_instrument = i;
    m_inst_cache.insert(i);
}
int     selected_effect() { return m_effect; }
void    select_effect(int e) {
    m_effect = e;
    m_effect_cache.insert(e);
}


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


void sprint_inst_effect_id(char* dst, int nr) {
    dst[0] = " "
        "ABCDEFGHIJKLMNOPQRSTUVWX"
        "YZ@0123456789#$*+=<>!?^~"[nr];
    dst[1] = '\0';
}


void draw_instrument_cache() {
    draw_cache(m_inst_cache, m_instrument);
}


void draw_effect_cache() {
    draw_cache(m_effect_cache, m_effect);
}


void draw_track_view() {

    auto widths = calculate_column_widths({ BUTTON_BIG, BUTTON_BIG, BUTTON_BIG, -1, BUTTON_BIG, BUTTON_BIG });

    // track select
    gfx::font(FONT_MONO);
    gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
    if (gui::button("\x1b")) m_track = std::max(1, m_track - 1);
    char str[3];
    sprint_track_id(str, m_track);
    gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
    gui::same_line();
    if (gui::button(str)) enter_track_select();
    gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
    gui::same_line();
    if (gui::button("\x1c")) m_track = std::min<int>(TRACK_COUNT, m_track + 1);


    assert(m_track > 0);
    Song& song = player::song();
    Track& track = song.tracks[m_track - 1];

    gui::same_line();
    gui::padding({ widths[3], 0 });

    // copy & paste
    gfx::font(FONT_MONO);
    gui::same_line();
    gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
    if (gui::button("\x1d")) m_copy_track = track;
    gui::same_line();
    gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
    if (gui::button("\x1e")) track = m_copy_track;
    gui::separator();

    // cache
    draw_instrument_cache();
    gui::separator();
    draw_effect_cache();
    gui::separator();


    // clavier slider
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, BUTTON_SMALL });
    gui::drag_int("", "", m_clavier_offset, 0, 96 - CLAVIER_WIDTH, CLAVIER_WIDTH);

    gui::same_line();
    Vec c1 = gui::cursor() + Vec(-BUTTON_SMALL - gui::PADDING, BUTTON_SMALL + gui::PADDING * 2 + gui::SEPARATOR_WIDTH);
    gui::next_line();

    enum {
        PAGE_LENGTH = 16
    };

    int player_row = player::row();
    gfx::font(FONT_MONO);
    for (int i = 0; i < PAGE_LENGTH; ++i) {
        if (i % 4 == 0) gui::separator();

        int row_nr = m_track_page * PAGE_LENGTH + i;
        if (row_nr >= song.track_length) {
            gui::padding({ 0, BUTTON_SMALL });
            continue;
        }

        bool highlight = row_nr == player_row;
        Track::Row& row = track.rows[row_nr];

        char str[4];

        // instrument
        sprint_inst_effect_id(str, row.instrument);
        gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
        if (highlight) gui::highlight();
        if (gui::button(str)) {
            if (row.instrument > 0) row.instrument = 0;
            else row.instrument = selected_instrument();
        }
        if (row.instrument > 0 && gui::hold()) {
            select_instrument(row.instrument);
        }

        // effect
        sprint_inst_effect_id(str, row.effect);
        gui::same_line();
        gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
        if (highlight) gui::highlight();
        if (gui::button(str)) {
            if (row.effect > 0) row.effect = 0;
            else row.effect = selected_effect();
        }
        if (row.effect > 0 && gui::hold()) {
            select_effect(row.effect);
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
        gui::min_item_size({ 0, BUTTON_SMALL });
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
        int w = gfx::screensize().x - gui::cursor().x - gui::PADDING * 4 - BUTTON_SMALL - gui::SEPARATOR_WIDTH;
        gui::min_item_size({ w, BUTTON_SMALL });
        uint8_t old_note = row.note;
        if (gui::clavier(row.note, m_clavier_offset, highlight)) {
            if (row.note == 0) row = {};
            else if (old_note == 0) {
                row.instrument = selected_instrument();
                row.effect     = selected_effect();
            }
        }

        gui::same_line();
        gui::separator();
        gui::next_line();
    }

    // track pages
    Vec c2 = gui::cursor();
    gui::cursor(c1);
    gui::min_item_size({ BUTTON_SMALL, c2.y - c1.y - gui::PADDING });
    gui::vertical_drag_int(m_track_page, 0, (song.track_length + PAGE_LENGTH - 1) / PAGE_LENGTH - 1);
    gui::cursor(c2);
    gui::separator();


}


