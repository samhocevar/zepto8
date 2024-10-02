//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2020 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <lol/engine.h>  // lol::input
#include <lol/msg>       // lol::msg
#include <lol/vector>    // lol::vec2
#include <lol/transform> // lol::mat4
#include <lol/color>     // lol::color

#include "player.h"

#include "zepto8.h"
#include "pico8/vm.h"
#include "pico8/pico8.h"
#include "raccoon/vm.h"

namespace z8
{

player::player(bool is_embedded, bool is_raccoon)
  : m_input_map
    {
        { lol::input::key::SC_Left, 0 },
        { lol::input::key::SC_Right, 1 },
        { lol::input::key::SC_Up, 2 },
        { lol::input::key::SC_Down, 3 },
    },
    m_embedded(is_embedded)
{
    if (is_raccoon)
    {
        m_input_map[lol::input::key::SC_X] = 4;
        m_input_map[lol::input::key::SC_C] = 5;
        m_input_map[lol::input::key::SC_V] = 6;
        m_input_map[lol::input::key::SC_B] = 7;
    }
    else
    {
        // Player 1 buttons
        m_input_map[lol::input::key::SC_Z] = 4;
        m_input_map[lol::input::key::SC_C] = 4;
        m_input_map[lol::input::key::SC_N] = 4;
        m_input_map[lol::input::key::SC_Insert] = 4;
        m_input_map[lol::input::key::SC_X] = 5;
        m_input_map[lol::input::key::SC_V] = 5;
        m_input_map[lol::input::key::SC_M] = 5;
        m_input_map[lol::input::key::SC_Delete] = 5;

        // Pause menu
        m_input_map[lol::input::key::SC_P] = 6;
        m_input_map[lol::input::key::SC_Return] = 6;

        // Player 2 buttons
        m_input_map[lol::input::key::SC_S] = 8;
        m_input_map[lol::input::key::SC_F] = 9;
        m_input_map[lol::input::key::SC_E] = 10;
        m_input_map[lol::input::key::SC_D] = 11;
        m_input_map[lol::input::key::SC_LShift] = 12;
        m_input_map[lol::input::key::SC_A] = 12;
        m_input_map[lol::input::key::SC_Q] = 13;
        m_input_map[lol::input::key::SC_Tab] = 13;
    }

    // FIXME: find a way to use std::make_shared here on MSVC
    if (is_raccoon)
        m_vm.reset((z8::vm_base *)new raccoon::vm());
    else
        m_vm.reset((z8::vm_base *)new pico8::vm());

    // Allow text input
    lol::input::keyboard()->capture_text(true);

    // Create an ortho camera
    m_scenecam = new lol::Camera();
    m_scenecam->SetView(lol::mat4(1.f));
    lol::Scene& scene = lol::Scene::GetScene();
    scene.PushCamera(m_scenecam);
    lol::Ticker::Ref(m_scenecam);

    // Register audio callbacks
    {
        using namespace std::placeholders;
        std::function<void(void*, int)> f = std::bind(&vm_base::get_audio, m_vm, _1, _2);
        m_stream = lol::audio::start_streaming(f, lol::audio::format::sint16le, 22050, 1);
    }

    // FIXME: the image gets deleted by TextureImage class, it
    // does not seem right to me.
    auto img = new lol::old_image(lol::ivec2(128, 128));
    img->unlock(img->lock<lol::PixelFormat::RGBA_8>()); // ensure RGBA_8 is present
    m_tile = lol::TileSet::create("tile", new lol::old_image(*img), lol::ivec2(128, 128), lol::ivec2(1, 1));

#if 0
    img = new lol::old_image(lol::ivec2(128, 32));
    img->unlock(img->lock<lol::PixelFormat::RGBA_8>()); // ensure RGBA_8 is present
    m_font_tile = lol::TileSet::create("font", new lol::old_image(*img), lol::ivec2(128, 32), lol::ivec2(1, 1));
#endif

    /* Allocate memory */
    m_screen.resize(128 * 128);
}

player::~player()
{
    lol::TileSet::destroy(m_tile);
#if 0
    lol::TileSet::destroy(m_font_tile);
#endif

    lol::audio::stop_streaming(m_stream);

    lol::Scene& scene = lol::Scene::GetScene();
    lol::Ticker::Unref(m_scenecam);
    scene.PopCamera(m_scenecam);
}

void player::load(std::string const &name)
{
    m_vm->load(name);
}

void player::run()
{
    m_vm->run();
}

void player::tick_game(float seconds)
{
    lol::WorldEntity::tick_game(seconds);

    // Aspect ratio
    lol::ivec2 screen_size = m_vm->get_screen_resolution();
    m_win_size = lol::video::size();
    m_scale = (float)std::min(m_win_size.x / screen_size.x, m_win_size.y / screen_size.y);
    m_screen_pos = lol::ivec2((lol::vec2(m_win_size) - lol::vec2(screen_size.x * m_scale, screen_size.y * m_scale)) / 2.f);
    m_scenecam->SetProjection(lol::mat4::ortho(0.f, (float)m_win_size.x, 0.f, (float)m_win_size.y, -100.f, 100.f));

    auto mouse = lol::input::mouse();
    auto keyboard = lol::input::keyboard();

    // Mouse events
    lol::vec2 mouse_pos(mouse->axis(lol::input::axis::ScreenX),
                        mouse->axis(lol::input::axis::ScreenY));
    int mx = (int)((mouse_pos.x - m_screen_pos.x) / m_scale);
    int my = screen_size.y - 1 - (int)((mouse_pos.y - m_screen_pos.y) / m_scale);
    int buttons = (mouse->button(lol::input::button::BTN_Left) ? 1 : 0)
                + (mouse->button(lol::input::button::BTN_Right) ? 2 : 0)
                + (mouse->button(lol::input::button::BTN_Middle) ? 4 : 0);
    // TODO mouse
    lol::ivec2 relative(0, 0);
    int scroll = 0;
    m_vm->mouse(lol::ivec2(mx, my), relative, buttons, scroll);

    // Joystick events
    if (auto joy = lol::input::joystick(0))
    {
        m_vm->button(0, 0, joy->button(lol::input::button::BTN_DpadLeft));
        m_vm->button(0, 1, joy->button(lol::input::button::BTN_DpadRight));
        m_vm->button(0, 2, joy->button(lol::input::button::BTN_DpadUp));
        m_vm->button(0, 3, joy->button(lol::input::button::BTN_DpadDown));
        m_vm->button(0, 4, joy->button(lol::input::button::BTN_A));
        m_vm->button(0, 5, joy->button(lol::input::button::BTN_B));
        m_vm->button(0, 6, joy->button(lol::input::button::BTN_Start));
    }

    if (!m_embedded)
    {
        // Keyboard events as buttons
        for (auto const &k : m_input_map)
            m_vm->button(0, k.second, keyboard->key(k.first));

        // Keyboard events as text
        if (keyboard->key_pressed(lol::input::key::SC_Return))
            m_vm->text('\r');
        if (keyboard->key_pressed(lol::input::key::SC_Backspace))
            m_vm->text('\x08');
        if (keyboard->key_pressed(lol::input::key::SC_Delete))
            m_vm->text('\x7f');

        for (auto ch : keyboard->text())
            m_vm->text(ch);
    }

    // Drag-and-drop events
    if (lol::input::has_dnd())
        lol::msg::info("dropped file %s\n", lol::input::get_dnd().c_str());

    // Step the VM
    m_vm->step(seconds);
}

void player::tick_draw(float seconds, lol::Scene &scene)
{
    lol::WorldEntity::tick_draw(seconds, scene);

#if 0
    if (m_vm->m_bios) // FIXME: PICO-8 specific
    {
        // Render the font
        u8vec4 data[128 * 32];
        for (int j = 0; j < 32; ++j)
        for (int i = 0; i < 128; ++i)
        {
            auto p = m_vm->m_bios->get_spixel(i, j);
            data[j * 128 + i] = p ? pico8::palette::get8(p) : u8vec4(0);
        }
        m_font_tile->GetTexture()->Bind();
        m_font_tile->GetTexture()->SetData(data);
    }
#endif

    if (!m_embedded)
    {
        lol::ivec2 screen_size = m_vm->get_screen_resolution();

        lol::ivec2 tile_size = m_tile->GetImageSize();
        if (tile_size.x != screen_size.x || tile_size.y != screen_size.y)
        {
            lol::TileSet::destroy(m_tile);

            auto img = new lol::old_image(screen_size);
            img->unlock(img->lock<lol::PixelFormat::RGBA_8>()); // ensure RGBA_8 is present

            m_tile = lol::TileSet::create("tile", new lol::old_image(*img), screen_size, lol::ivec2(1, 1));

            m_screen.resize(screen_size.x * screen_size.y);
        }

        // Render the VM screen to our buffer
        m_vm->render(m_screen.data());

        if (m_tile->GetTexture())
        {
            // Blit buffer to the texture
            // FIXME: move this to some kind of memory viewer class?
            m_tile->GetTexture()->Bind();
            m_tile->GetTexture()->SetData(m_screen.data());

            scene.get_renderer()->clear_color(lol::color::black);
            scene.AddTile(m_tile, 0, lol::vec3((float)m_screen_pos.x, (float)m_screen_pos.y, 10.f), lol::vec2(m_scale), 0.f);
        }
    }
}

lol::Texture *player::get_texture()
{
    return m_tile ? m_tile->GetTexture() : nullptr;
}

#if 0
lol::Texture *player::get_font_texture()
{
    return m_font_tile ? m_font_tile->GetTexture() : nullptr;
}
#endif

} // namespace z8

