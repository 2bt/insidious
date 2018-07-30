#include "game.hpp"
#include "gui.hpp"
#include "player.hpp"
#include "wavelog.hpp"


namespace game {
namespace {


char* m_pref_path;


bool save_tune(char const* name) {
    if (!m_pref_path) return false;
    static std::array<char, 1024> path;
    snprintf(path.data(), path.size(), "%s%s", m_pref_path, name);
    SDL_RWops* file = SDL_RWFromFile(path.data(), "wb");
    if (!file) return false;
    Tune& t = player::tune();
    SDL_WriteU8(file, t.tempo);
    SDL_WriteU8(file, t.swing);
    SDL_RWwrite(file, t.tracks.data(), sizeof(Track), t.tracks.size());
    SDL_RWwrite(file, t.instruments.data(), sizeof(Instrument), t.instruments.size());
    SDL_RWwrite(file, t.effects.data(), sizeof(Effect), t.effects.size());
    SDL_WriteU8(file, t.table.size());
    SDL_RWwrite(file, t.table.data(), sizeof(Tune::Block), t.table.size());
    SDL_RWclose(file);
    return true;
}


bool load_tune(char const* name) {
    if (!m_pref_path) return false;
    static std::array<char, 1024> path;
    snprintf(path.data(), path.size(), "%s%s", m_pref_path, name);
    SDL_RWops* file = SDL_RWFromFile(path.data(), "rb");
    if (!file) return false;
    Tune& t = player::tune();
    t.tempo = SDL_ReadU8(file);
    t.swing = SDL_ReadU8(file);
    SDL_RWread(file, t.tracks.data(), sizeof(Track), t.tracks.size());
    SDL_RWread(file, t.instruments.data(), sizeof(Instrument), t.instruments.size());
    SDL_RWread(file, t.effects.data(), sizeof(Effect), t.effects.size());
    uint8_t table_len = SDL_ReadU8(file);
    t.table.resize(table_len);
    SDL_RWread(file, t.table.data(), sizeof(Tune::Block), table_len);
    SDL_RWclose(file);
    return true;
}


void audio_callback(void* userdata, Uint8* stream, int len) {
    player::fill_buffer((short*) stream, len / 2);
    wavelog::write((short*) stream, len / 2);
}


struct Edit {
    // track view
    int track          = 1;
    int clavier_offset = 48;
    int track_page     = 0;
    int instrument     = 1;
    int effect         = 1;

    // song view
    int block          = 0;


    struct TrackSelect {
        bool active;
        bool allow_nil;
        int* value;
    } track_select;

} m_edit;


void song_view();
void track_view();
void instrument_view();
void effect_view();
void (*m_view)(void);


template<class F>
void columns(int n, F f) {
    int x = gfx::screensize().x - gui::PADDING * (n + 1);
    for (int i = 0; i < n; ++i) {
        int w = x / (n - i);
        x -= w;
        f(i, w);
    }
}


bool track_select() {
    if (!m_edit.track_select.active) return false;

    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ 260, 65 });
    if (gui::button("cancel")) {
        m_edit.track_select.active = false;
    }

    gfx::font(FONT_SMALL);
    int track_nr = *m_edit.track_select.value;
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            if (j > 0) gui::same_line();
            gui::min_item_size({ 65, 65 });
            int n = i * 16 + j;
            if (m_edit.track_select.allow_nil == false && n == 0) {
                gui::text("");
                continue;
            }

            char str[3] = "..";
            if (n > 0) sprintf(str, "%02X", n);
            if (gui::button(str, n == track_nr)) {
                *m_edit.track_select.value = n;
                m_edit.track_select.active = false;
            }
        }
    }

    return true;
}


void song_view() {

    gfx::font(FONT_MONO);

    // mute buttons
    gui::padding({ 88, 65 });
    gui::same_line();
    gui::separator();
    gui::same_line();
    for (int c = 0; c < CHANNEL_COUNT; ++c) {

        gui::same_line();

        gui::min_item_size({ 88, 65 });
        char str[2] = { char('0' + c) };
        bool a = player::is_channel_active(c);
        if (gui::button(str, a)) {
            player::set_channel_active(c, !a);
        }
    }

    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();

    Tune& tune = player::tune();
    auto& table = tune.table;

    int block_nr = player::block();
    for (int i = 0; i < 16; ++i) {
        bool highlight = i == block_nr;

        char str[3];
        sprintf(str, "%02X", i);
        gui::min_item_size({ 88, 65 });
        if (highlight) gui::highlight();
        if (gui::button(str, i == m_edit.block)) {
            m_edit.block = i;
        }
        if (gui::hold()) {
            player::block(i);
        }
        gui::same_line();
        gui::separator();
        if (i >= (int) table.size()) {
            gui::padding({});
            continue;
        }


        Tune::Block& block = table[i];

        for (int c = 0; c < CHANNEL_COUNT; ++c) {

            gui::same_line();

            char str[3] = "..";
            if (block[c] > 0) sprintf(str, "%02X", block[c]);
            gui::min_item_size({ 88, 65 });
            if (highlight) gui::highlight();
            if (gui::button(str)) {
                m_edit.track_select.active = true;
                m_edit.track_select.value = &block[c];
                m_edit.track_select.allow_nil = true;
            }
            if (block[c] > 0 && gui::hold()) {
                gui::block_touch();
                m_edit.track = block[c];
                m_view = track_view;
            }
        }
    }

    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();

    gfx::font(FONT_DEFAULT);

    gui::min_item_size({ 260, 65 });
    if (gui::button("add")) {
        if (m_edit.block <= (int) table.size()) {
            table.insert(table.begin() + m_edit.block, { 0, 0, 0, 0 });
        }
    }
    gui::same_line();
    gui::min_item_size({ 260, 65 });
    if (gui::button("delete")) {
        if (m_edit.block < (int) table.size() && table.size() > 1) {
            table.erase(table.begin() + m_edit.block);
        }
    }

    gui::min_item_size({ 260, 65 });
    if (gui::button("save")) save_tune("tune");
    gui::same_line();
    gui::min_item_size({ 260, 65 });
    if (gui::button("load")) load_tune("tune");
}


void track_view() {
    enum { PAGE_LENGTH = 16 };

    // track select
    gfx::font(FONT_MONO);
    gui::min_item_size({ 65, 65 });
    if (gui::button("-")) m_edit.track = std::max(1, m_edit.track - 1);
    char str[3];
    sprintf(str, "%02X", m_edit.track);
    gui::min_item_size({ 88, 65 });
    gui::same_line();
    if (gui::button(str)) {
        m_edit.track_select.active = true;
        m_edit.track_select.value = &m_edit.track;
        m_edit.track_select.allow_nil = false;
    }
    gui::min_item_size({ 65, 65 });
    gui::same_line();
    if (gui::button("+")) m_edit.track = std::min(255, m_edit.track + 1);

    // track pages
    gui::same_line();
    gfx::font(FONT_DEFAULT);
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, 65 });
    gui::drag_int("page", m_edit.track_page, 0, TRACK_LENGTH / PAGE_LENGTH - 1);

    gui::separator();

    // clavier slider
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 65 });
    enum { COLS = 21 };
    gui::drag_int("clavier", m_edit.clavier_offset, 0, 96 - COLS, COLS);

    int player_row = player::row();
    assert(m_edit.track > 0);
    Track& track = player::tune().tracks[m_edit.track - 1];

    gfx::font(FONT_MONO);
    for (int i = 0; i < PAGE_LENGTH; ++i) {
        int row_nr = m_edit.track_page * PAGE_LENGTH + i;
        bool highlight = row_nr == player_row;
        Track::Row& row = track.rows[row_nr];

        char str[4] = ".";

        // instrument
        if (row.instrument > 0) str[0] = '0' - 1  + row.instrument;
        gui::min_item_size({ 0, 65 });
        if (highlight) gui::highlight();
        if (gui::button(str)) {
            if (row.instrument > 0) row.instrument = 0;
            else row.instrument = m_edit.instrument;
        }
        if (row.instrument > 0 && gui::hold()) {
            gui::block_touch();
            m_edit.instrument = row.instrument;
            m_view = instrument_view;
        }

        // effect
        str[0] = '.';
        if (row.effect > 0) str[0] = '0' - 1  + row.effect;
        gui::same_line();
        gui::min_item_size({ 0, 65 });
        if (highlight) gui::highlight();
        if (gui::button(str)) {
            if (row.effect > 0) row.effect = 0;
            else row.effect = m_edit.effect;
        }
        if (row.effect > 0 && gui::hold()) {
            gui::block_touch();
            m_edit.effect = row.effect;
            m_view = effect_view;
        }


        // note
        str[0] = str[1] = str[2] = '.';
        if (row.note == 255) {
            str[0] = str[1] = str[2] = '=';
        }
        else if (row.note > 0) {
            str[0] = "CCDDEFFGGAAB"[(row.note - 1) % 12];
            str[1] = "-#-#--#-#-#-"[(row.note - 1) % 12];
            str[2] = '0' + (row.note - 1) / 12;
        }
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
        if (gui::clavier(row.note, m_edit.clavier_offset, highlight)) {
            row.instrument = m_edit.instrument;
            row.effect     = m_edit.effect;
        }
    }

    gfx::font(FONT_DEFAULT);
}


void instrument_view() {

    // instrument select
    gfx::font(FONT_MONO);
    gui::min_item_size({ 65, 65 });
    if (gui::button("-")) m_edit.instrument = std::max(1, m_edit.instrument - 1);
    char str[2] = { char('0' - 1  + m_edit.instrument) };
    gui::min_item_size({ 65, 65 });
    gui::same_line();
    if (gui::button(str)) {
        // TODO
    }
    gui::min_item_size({ 65, 65 });
    gui::same_line();
    if (gui::button("+")) m_edit.instrument = std::min<int>(INSTRUMENT_COUNT, m_edit.instrument + 1);

    Tune& tune = player::tune();
    Instrument& inst = tune.instruments[m_edit.instrument - 1];

    // name
    gfx::font(FONT_DEFAULT);
    gui::same_line();
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, 65 });
    gui::input_text(inst.name.data(), inst.name.size() - 1);

    gui::separator();

    // adsr
    gfx::font(FONT_MONO);
    columns(2, [&inst](int i, int w) {
        if (i) gui::same_line();
        gui::min_item_size({ w, 65 });
        gui::id(&inst.adsr[i]);
        int v = inst.adsr[i];
        if (gui::drag_int("%X", v, 0, 15)) {
            inst.adsr[i] = v;
        }
    });
    columns(2, [&inst](int i, int w) {
        if (i) gui::same_line();
        i += 2;
        gui::min_item_size({ w, 65 });
        gui::id(&inst.adsr[i]);
        int v = inst.adsr[i];
        if (gui::drag_int("%X", v, 0, 15)) {
            inst.adsr[i] = v;
        }
    });

    gui::separator();


    // rows
    for (int i = 0; i < MAX_INSTRUMENT_LENGTH; ++i) {

        char str[3];
        sprintf(str, "%02X", i);
        gui::min_item_size({ 88, 65 });

        if (gui::button(str, i == inst.loop)) inst.loop = i;
        gui::same_line();
        gui::separator();
        if (i >= inst.length) {
            gui::padding({ 425, 0 });
            gui::same_line();
            gui::separator();
            gui::padding({});
            continue;
        }
        auto& row = inst.rows[i];

        // flags
        constexpr std::pair<uint8_t, char const*> flags[] = {
            { NOISE, "\x13" },
            { PULSE, "\x14" },
            { SAW,   "\x15" },
            { TRI,   "\x16" },
            { RING,  "R" },
            { SYNC,  "S" },
            { GATE,  "G" },
        };
        for (auto p : flags) {
            gui::same_line();
            gui::min_item_size({ 0, 65 });
            if (gui::button(p.second, row.flags & p.first)) {
                row.flags ^= p.first;
            }
        }

        gui::same_line();
        gui::separator();
        str[0] = row.operaton == SET_PULSEWIDTH ? '=' : '+';
        str[1] = '\0';
        if (gui::button(str)) {
            row.operaton = !row.operaton;
        }
        gui::same_line();
        gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, 65 });
        int v = row.value;
        gui::id(&row.value);
        if (gui::drag_int("%X", v, 0, 15)) row.value = v;
    }

    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();

    gfx::font(FONT_DEFAULT);

    gui::min_item_size({ 260, 65 });
    if (gui::button("add") && inst.length < MAX_INSTRUMENT_LENGTH) {
        inst.rows[inst.length] = { GATE, SET_PULSEWIDTH, 8 };
        ++inst.length;
    }
    gui::same_line();
    gui::min_item_size({ 260, 65 });
    if (gui::button("delete")) {
        if (inst.length > 0) {
            --inst.length;
        }
    }

}


void effect_view() {

    // effecct select
    gfx::font(FONT_MONO);
    gui::min_item_size({ 65, 65 });
    if (gui::button("-")) m_edit.effect = std::max(1, m_edit.effect - 1);
    char str[2] = { char('0' - 1  + m_edit.effect) };
    gui::min_item_size({ 65, 65 });
    gui::same_line();
    if (gui::button(str)) {
        // TODO
    }
    gui::min_item_size({ 65, 65 });
    gui::same_line();
    if (gui::button("+")) m_edit.effect = std::min<int>(EFFECT_COUNT, m_edit.effect + 1);


    Tune& tune = player::tune();
    Effect& effect = tune.effects[m_edit.effect - 1];

    // name
    gfx::font(FONT_DEFAULT);
    gui::same_line();
    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, 65 });
    gui::input_text(effect.name.data(), effect.name.size() - 1);

    gui::separator();


    // rows
    gfx::font(FONT_MONO);
    for (int i = 0; i < MAX_EFFECT_LENGTH; ++i) {

        char str[3];
        sprintf(str, "%02X", i);
        gui::min_item_size({ 88, 65 });

        if (gui::button(str, i == effect.loop)) effect.loop = i;
        gui::same_line();
        gui::separator();
        if (i >= effect.length) {
            gui::padding({});
            continue;
        }

        int v = (effect.rows[i] >> 2) - 32;
        gui::min_item_size({ gfx::screensize().x * 4 / 5, 65 });
        gui::id(&effect.rows[i]);
        if (gui::drag_int("%+3d", v, -16, 16)) effect.rows[i] = (effect.rows[i] & 0x03) | ((v + 32) << 2);

        v = effect.rows[i] & 0x03;
        gui::same_line();
        gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2 - gui::cursor().x, 65 });
        gui::id(&effect.rows[i + MAX_EFFECT_LENGTH]);
        if (gui::drag_int("%d", v, 0, 3)) {
            effect.rows[i] = (effect.rows[i] & 0xfc) | v;
        }
    }

    gui::min_item_size({ gfx::screensize().x - gui::PADDING * 2, 0 });
    gui::separator();

    gfx::font(FONT_DEFAULT);

    gui::min_item_size({ 260, 65 });
    if (gui::button("add") && effect.length < MAX_EFFECT_LENGTH) {
        effect.rows[effect.length] = 0x80;
        ++effect.length;
    }
    gui::same_line();
    gui::min_item_size({ 260, 65 });
    if (gui::button("delete")) {
        if (effect.length > 0) {
            --effect.length;
        }
    }
}


} // namespace


bool init() {
    m_pref_path = SDL_GetPrefPath("sdl", "rausch");
    if (!m_pref_path) return false;

    m_view = song_view;


    // default tune
    Tune& t = player::tune();
    t.tempo = 5;
    t.table = {
        { 1, 0, 0, 0 }
    };
    Track& track = t.tracks[0];
    track.rows[0] = { 1, 1, 37 };
    track.rows[2] = { 0, 0, 255 };
    track.rows[4] = { 1, 1, 49 };
    track.rows[6] = { 0, 2, 0 };
    track.rows[14] = { 0, 0, 255 };

    Instrument& i = t.instruments[0];
    i.adsr = { 1, 8, 8, 8 };
    i.rows[0] = { NOISE | GATE, SET_PULSEWIDTH, 0x8 };
    i.rows[1] = { PULSE | GATE, INC_PULSEWIDTH, 0x1 };
    i.length = 2;
    i.loop = 1;

    // vibrato
    Effect& e = t.effects[1];
    e.rows[0] = 0x80;
    e.rows[1] = 0x81;
    e.rows[2] = 0x82;
    e.rows[3] = 0x82;
    e.rows[4] = 0x81;
    e.rows[5] = 0x80;
    e.rows[6] = 0x7f;
    e.rows[7] = 0x7e;
    e.rows[8] = 0x7e;
    e.rows[9] = 0x7f;
    e.length = 10;
    e.loop = 0;

    wavelog::init(MIXRATE);
	SDL_AudioSpec spec = { MIXRATE, AUDIO_S16, 1, 0, 1024, 0, 0, audio_callback };
	SDL_OpenAudio(&spec, nullptr);
	SDL_PauseAudio(0);
    return true;
}


void free() {
    SDL_free(m_pref_path);
    wavelog::free();
}


struct View {
    char const* name;
    void (*func)(void);
};
constexpr std::array<View, 4> views = {
    View{ "song", song_view },
    View{ "track", track_view },
    View{ "instrument", instrument_view },
    View{ "effect", effect_view },
};


void draw() {
    gfx::clear();
    gui::begin_frame();

    if (!track_select()) {
        gfx::font(FONT_DEFAULT);

        columns(views.size(), [](int i, int w) {
            if (i) gui::same_line();
            gui::min_item_size({ w, 88 });
            if (gui::button(views[i].name, m_view == views[i].func)) m_view = views[i].func;
        });
        gui::separator();

        m_view();

        // stop and play
        gui::padding({ 0, gfx::screensize().y - gui::cursor().y - gui::PADDING * 3 - 88 });
        gfx::font(FONT_DEFAULT);
        bool is_playing = player::is_playing();
        columns(2, [is_playing](int i, int w) {
            if (i) gui::same_line();
            gui::min_item_size({ w, 88 });
            switch (i) {
            case 0:
                if (gui::button("\x11")) player::stop();
                break;
            case 1:
                if (gui::button("\x10\x12", is_playing)) {
                    if (is_playing) player::pause();
                    else player::play();
                }
                break;
            }
        });

    }

    gfx::present();
}


} // namespace
