#include "song_edit.hpp"
#include "track_edit.hpp"
#include "edit.hpp"
#include "gui.hpp"
#include "player.hpp"
#include <algorithm>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

enum {
    PAGE_LENGTH = 16
};

namespace {

int  m_song_scroll    = 0;
int  m_block          = 0;

} // namespace


int get_selected_block() { return m_block; }

void draw_song_view() {
    Song& song = player::song();


    // tempo and swing
    auto widths = calculate_column_widths({ -9, -5 });
    gui::min_item_size({ widths[0], 0 });
    gui::drag_int("Tempo", "%X", song.tempo, 4, 12);
    gui::same_line();
    gui::min_item_size({ widths[1], 0 });
    gui::drag_int("Swing", "%X", song.swing, 0, 4);
    gui::separator();

    // mute buttons
    widths = calculate_column_widths({ 88, gui::SEPARATOR_WIDTH, -1, -1, -1, -1, gui::SEPARATOR_WIDTH, 65 });
    gui::padding({ widths[0], 65 });
    gui::same_line();
    gui::separator();
    gui::same_line();
    gfx::font(FONT_DEFAULT);
    char str[] = "Voice .";
    for (int c = 0; c < CHANNEL_COUNT; ++c) {
        str[6] = '1' + c;
        gui::min_item_size({ widths[c + 2], 65 });
        bool a = player::is_channel_active(c);
        if (gui::button(str, a)) {
            player::set_channel_active(c, !a);
        }
        gui::same_line();
    }

    gui::separator();

    // prepare scrollbar
    int max_scroll = std::max<int>(PAGE_LENGTH, song.table_length) - PAGE_LENGTH;
    if (m_song_scroll > max_scroll) m_song_scroll = max_scroll;
    Vec c1 = gui::cursor() + Vec(0, 65 + gui::PADDING * 2 + gui::SEPARATOR_WIDTH);

    gui::padding({ 65, 0 });
    gui::separator();

    auto& table = song.table;

    gfx::font(FONT_MONO);
    int player_block = player::block();
    for (int i = 0; i < PAGE_LENGTH; ++i) {
        int block_nr = m_song_scroll + i;
        bool highlight = block_nr == player_block;

        sprintf(str, "%02X", block_nr);
        gui::min_item_size({ widths[0], 65 });
        if (highlight) gui::highlight();
        if (gui::button(str, block_nr == m_block)) {
            m_block = block_nr;
            if (!edit::is_playing()) player::block(m_block);
        }
        gui::same_line();
        gui::separator();
        if (block_nr >= song.table_length) {
            for (int c = 0; c < CHANNEL_COUNT; ++c) {
                gui::same_line();
                gui::min_item_size({ widths[c + 2], 65 });
                gui::padding({});
            }
        }
        else {

            Song::Block& block = table[block_nr];
            for (int c = 0; c < CHANNEL_COUNT; ++c) {
                gui::same_line();
                char str[3];
                sprint_track_id(str, block[c]);
                gui::min_item_size({ widths[c + 2], 65 });
                if (highlight) gui::highlight();
                if (gui::button(str)) {
                    enter_track_select(block[c]);
                }
                if (block[c] > 0 && gui::hold()) {
                    gui::block_touch();
                    select_track(block[c]);
                    edit::set_view(VIEW_TRACK);
                }
            }
        }

        gui::same_line();
        gui::separator();
        gui::next_line();

    }

    // song scrollbar
    Vec c2 = gui::cursor();
    gui::cursor(c1);
    gui::min_item_size({ 65, c2.y - c1.y - gui::PADDING });
    gui::vertical_drag_int(m_song_scroll, 0, max_scroll, PAGE_LENGTH);
    gui::cursor(c2);

    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();


    // buttons
    gfx::font(FONT_MONO);
    gui::min_item_size({ 88, 88 });
    if (gui::button("-")) {
        if (m_block < song.table_length && song.table_length > 1) {
            table[m_block] = {};
            std::rotate(
                table.begin() + m_block,
                table.begin() + m_block + 1,
                table.begin() + song.table_length);
            --song.table_length;
        }
    }
    gui::same_line();
    gui::min_item_size({ 88, 88 });
    if (gui::button("+")) {
        if (m_block <= song.table_length && song.table_length < MAX_SONG_LENGTH) {
            std::rotate(
                table.begin() + m_block,
                table.begin() + song.table_length,
                table.begin() + song.table_length + 1);
            ++song.table_length;
        }
    }
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();
}


