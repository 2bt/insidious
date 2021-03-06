#include "track_view.hpp"
#include "gui.hpp"
#include "player.hpp"
#include "edit.hpp"


namespace {

bool       m_filter_mode;
Instrument m_copy_inst;
Effect     m_copy_effect;

void draw_instrument_select() {

    gfx::font(FONT_DEFAULT);
    auto widths = calculate_column_widths({ -1 });
    gui::min_item_size({ widths[0], BUTTON_BIG });
    if (gui::button("Cancel")) edit::set_popup(nullptr);
    gui::separator();

    Song& song = player::song();

    widths = calculate_column_widths({ -1, -1 });
    for (int y = 0; y < INSTRUMENT_COUNT / 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            int nr = y + x * (INSTRUMENT_COUNT / 2) + 1;
            Instrument const& inst = song.instruments[nr - 1];

            Vec c1 = gui::cursor();
            gui::min_item_size({ widths[x], BUTTON_SMALL });
            if (inst.length > 0) gui::highlight();
            if (gui::button("", nr == selected_instrument())) {
                edit::set_popup(nullptr);
                select_instrument(nr);
            }

            gui::same_line();
            Vec c2 = gui::cursor();
            gui::cursor(c1);
            gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
            char str[2];
            sprint_inst_effect_id(str, nr);
            gfx::font(FONT_MONO);
            gui::text(str);
            gfx::font(FONT_DEFAULT);
            gui::same_line();
            gui::min_item_size({ widths[x] - BUTTON_SMALL - gui::PADDING, BUTTON_SMALL });
            gui::align(gui::LEFT);
            gui::text(inst.name.data());
            gui::align(gui::CENTER);
            gui::same_line();
            gui::cursor(c2);
        }
        gui::next_line();
    }
}
// XXX: this is a copy of instrument_select with s/instrument/effect/g :(
void draw_effect_select() {

    gfx::font(FONT_DEFAULT);
    auto widths = calculate_column_widths({ -1 });
    gui::min_item_size({ widths[0], BUTTON_BIG });
    if (gui::button("Cancel")) edit::set_popup(nullptr);
    gui::separator();

    Song& song = player::song();

    widths = calculate_column_widths({ -1, -1 });
    for (int y = 0; y < EFFECT_COUNT / 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            int nr = y + x * (EFFECT_COUNT / 2) + 1;
            Effect const& effect = song.effects[nr - 1];

            Vec c1 = gui::cursor();
            gui::min_item_size({ widths[x], BUTTON_SMALL });
            if (effect.length > 0) gui::highlight();
            if (gui::button("", nr == selected_effect())) {
                edit::set_popup(nullptr);
                select_effect(nr);
            }

            gui::same_line();
            Vec c2 = gui::cursor();
            gui::cursor(c1);
            gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
            char str[2];
            sprint_inst_effect_id(str, nr);
            gfx::font(FONT_MONO);
            gui::text(str);
            gfx::font(FONT_DEFAULT);
            gui::same_line();
            gui::min_item_size({ widths[x] - BUTTON_SMALL - gui::PADDING, BUTTON_SMALL });
            gui::align(gui::LEFT);
            gui::text(effect.name.data());
            gui::align(gui::CENTER);
            gui::same_line();
            gui::cursor(c2);
        }
        gui::next_line();
    }
}


} // namespace


void enter_instrument_select() {
    edit::set_popup(draw_instrument_select);
}

void enter_effect_select() {
    edit::set_popup(draw_effect_select);
}


enum {
    PAGE_LENGTH = 16
};


void draw_instrument_view() {

    // cache
    draw_instrument_cache();
    gui::separator();

    Song& song = player::song();
    Instrument& inst = song.instruments[selected_instrument() - 1];

    // name
    auto widths = calculate_column_widths({ -1, BUTTON_BIG, BUTTON_BIG });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], BUTTON_BIG });
    gui::align(gui::LEFT);
    gui::input_text(inst.name.data(), inst.name.size() - 1);
    gui::align(gui::CENTER);

    // copy & paste
    gfx::font(FONT_MONO);
    gui::same_line();
    gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
    if (gui::button("\x1d")) m_copy_inst = inst;
    gui::same_line();
    gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
    if (gui::button("\x1e")) inst = m_copy_inst;
    gui::separator();


    // wave/filter switch
    gfx::font(FONT_DEFAULT);
    widths = calculate_column_widths({ -1, -1 });
    gui::min_item_size({ widths[0], BUTTON_BIG });
    if (gui::tab_button("Wave", !m_filter_mode)) m_filter_mode = false;
    gui::same_line();
    gui::min_item_size({ widths[1], BUTTON_BIG });
    if (gui::tab_button("Filter", m_filter_mode)) m_filter_mode = true;
    gui::separator();


    if (!m_filter_mode) {

        // adsr
        constexpr char const* labels[] = { "Attack", "Decay", "Sustain", "Release" };
        widths = calculate_column_widths({ -1, -1, BUTTON_BIG });
        for (int i = 0; i < 2; ++i) {
            if (i > 0) gui::same_line();
            gui::min_item_size({ widths[i], BUTTON_SMALL });
            gui::drag_int(labels[i], "%X", inst.adsr[i], 0, 15);
        };
        Vec c = gui::cursor();
        gui::same_line();
        gui::min_item_size({ BUTTON_BIG, BUTTON_SMALL * 2 + gui::PADDING });
        if (gui::button("\x11", inst.hard_restart)) inst.hard_restart ^= 1;
        gui::cursor(c);
        for (int i = 0; i < 2; ++i) {
            if (i > 0) gui::same_line();
            gui::min_item_size({ widths[i], BUTTON_SMALL });
            gui::drag_int(labels[i + 2], "%X", inst.adsr[i + 2], 0, 15);
        };


        gui::separator();
        gfx::font(FONT_MONO);

        // wave
        for (int i = 0; i < MAX_INSTRUMENT_LENGTH; ++i) {

            char str[2];
            snprintf(str, 1, "%X", i);
            gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });

            if (gui::button(str, i == inst.loop)) inst.loop = i;
            gui::same_line();
            gui::separator();
            if (i >= inst.length) {
                gui::padding({ 467, 0 });
                gui::same_line();
                gui::separator();
                gui::padding({});
                continue;
            }
            auto& row = inst.rows[i];

            // flags
            constexpr std::pair<uint8_t, char const*> flags[] = {
                { Instrument::F_NOISE, "\x14" },
                { Instrument::F_PULSE, "\x15" },
                { Instrument::F_SAW,   "\x16" },
                { Instrument::F_TRI,   "\x17" },
                { Instrument::F_RING,  "R" },
                { Instrument::F_SYNC,  "S" },
                { Instrument::F_GATE,  "G" },
            };
            for (auto p : flags) {
                gui::same_line();
                gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
                if (gui::button(p.second, row.flags & p.first)) {
                    row.flags ^= p.first;
                }
            }

            gui::same_line();
            gui::separator();
            str[0] = "+=-"[row.operation];
            str[1] = '\0';
            gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
            if (gui::button(str)) row.operation = !row.operation;
            gui::same_line();
            gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, BUTTON_SMALL });
            gui::drag_int("", "%02X", row.value, 0, 31);
        }

        gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
        gui::separator();


        gfx::font(FONT_MONO);
        gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
        if (gui::button("-")) {
            if (inst.length > 0) {
                --inst.length;
            }
        }
        gui::same_line();
        gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
        if (gui::button("+") && inst.length < MAX_INSTRUMENT_LENGTH) {
            inst.rows[inst.length] = { Instrument::F_GATE, Instrument::OP_INC, 0 };
            ++inst.length;
        }
    }
    else {
        // filter table
        Filter& filter = inst.filter;

        widths = calculate_column_widths({ -1, -1, -1, -1 });
        char str[] = "Voice .";
        for (int c = 0; c < CHANNEL_COUNT; ++c) {
            str[6] = '1' + c;
            if (c) gui::same_line();
            gui::min_item_size({ widths[c], BUTTON_SMALL });
            if (gui::button(str, filter.routing & (1 << c))) filter.routing ^= 1 << c;
        }
        gui::separator();

        gfx::font(FONT_MONO);
        for (int i = 0; i < MAX_FILTER_LENGTH; ++i) {

            snprintf(str, 1, "%X", i);
            gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });

            if (gui::button(str, i == filter.loop)) filter.loop = i;
            gui::same_line();
            gui::separator();
            if (i >= filter.length) {
                gui::padding({ 199, 0 });
                gui::same_line();
                gui::separator();
                gui::padding({ 240, 0 });
                gui::same_line();
                gui::separator();
                gui::padding({});
                continue;
            }
            auto& row = filter.rows[i];


            // type
            gui::same_line();
            gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
            if (gui::button("\x18", row.type & FILTER_LOW)) row.type ^= FILTER_LOW;
            gui::same_line();
            gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
            if (gui::button("\x19", row.type & FILTER_BAND)) row.type ^= FILTER_BAND;
            gui::same_line();
            gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
            if (gui::button("\x1a", row.type & FILTER_HIGH)) row.type ^= FILTER_HIGH;

            // resonance
            gui::same_line();
            gui::separator();

            gui::min_item_size({ 240, BUTTON_SMALL });
            gui::drag_int("", "%X", row.resonance, 0, 15);


            // operation
            gui::same_line();
            gui::separator();
            str[0] = "+=-"[row.operation];
            str[1] = '\0';
            gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
            if (gui::button(str)) row.operation = (row.operation + 1) % 3;

            // cutoff
            gui::same_line();
            gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, BUTTON_SMALL });
            gui::drag_int("", "%02X", row.value, 0, 31);
        }

        gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
        gui::separator();


        gfx::font(FONT_MONO);
        gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
        if (gui::button("-")) {
            if (filter.length > 0) {
                --filter.length;
            }
        }
        gui::same_line();
        gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
        if (gui::button("+") && filter.length < MAX_FILTER_LENGTH) {
            filter.rows[filter.length] = { 0, 0xf, Filter::OP_SET, 0 };
            ++filter.length;
        }
    }
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();
}


void draw_effect_view() {

    // cache
    draw_effect_cache();
    gui::separator();


    Song& song = player::song();
    Effect& effect = song.effects[selected_effect() - 1];

    // name
    auto widths = calculate_column_widths({ -1, BUTTON_BIG, BUTTON_BIG });
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ widths[0], BUTTON_BIG });
    gui::align(gui::LEFT);
    gui::input_text(effect.name.data(), effect.name.size() - 1);
    gui::align(gui::CENTER);

    // copy & paste
    gfx::font(FONT_MONO);
    gui::same_line();
    gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
    if (gui::button("\x1d")) m_copy_effect = effect;
    gui::same_line();
    gui::min_item_size({ BUTTON_BIG, BUTTON_BIG });
    if (gui::button("\x1e")) effect = m_copy_effect;

    gui::separator();

    widths = calculate_column_widths({ BUTTON_SMALL, gui::SEPARATOR_WIDTH, BUTTON_SMALL, -1, BUTTON_SMALL, BUTTON_SMALL });

    // rows
    gfx::font(FONT_MONO);
    for (int i = 0; i < MAX_EFFECT_LENGTH; ++i) {
        char str[2];
        snprintf(str, 1, "%X", i);
        gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });

        if (gui::button(str, i == effect.loop)) effect.loop = i;
        gui::same_line();
        gui::separator();
        if (i >= effect.length) {
            gui::padding({});
            continue;
        }

        auto& row = effect.rows[i];

        str[0] = "+=~"[row.operation];
        gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
        if (gui::button(str)) {
            row.operation = (row.operation + 1) % 3;
            row.value = 0x30;
        }

        int min_value;
        int max_value;

        gui::same_line();
        gui::min_item_size({ widths[3], BUTTON_SMALL });
        if (row.operation == Effect::OP_RELATIVE || row.operation == Effect::OP_DETUNE) {
            int v = row.value - 0x30;
            gui::id(&row.value);
            if (gui::drag_int("", "%d", v, -24, 24)) row.value = v + 0x30;
            min_value = 0x30 - 24;
            max_value = 0x30 + 24;
        }
        else {
            min_value = 0;
            max_value = 96;
            gui::drag_int("", "%d", row.value, min_value, max_value, 2);
        }

        gui::same_line();
        gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
        if (gui::button("\x1b")) row.value = std::max(row.value - 1, min_value);
        gui::same_line();
        gui::min_item_size({ BUTTON_SMALL, BUTTON_SMALL });
        if (gui::button("\x1c")) row.value = std::min(row.value + 1, max_value);


    }

    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();


    widths = calculate_column_widths({ BUTTON_BIG, BUTTON_BIG, -1, -1 });
    gfx::font(FONT_MONO);
    gui::min_item_size({ widths[0], BUTTON_BIG });
    if (gui::button("-")) {
        if (effect.length > 0) {
            --effect.length;
        }
    }
    gui::same_line();
    gui::min_item_size({ widths[1], BUTTON_BIG });
    if (gui::button("+") && effect.length < MAX_EFFECT_LENGTH) {
        effect.rows[effect.length] = { Effect::OP_RELATIVE, 0x30 };
        ++effect.length;
    }
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();
}

