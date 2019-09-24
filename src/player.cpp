//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2019 Sam Hocevar <sam@hocevar.net>
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

#include <lol/engine.h>

#include "player.h"

#include "zepto8.h"
#include "pico8/vm.h"
#include "raccoon/vm.h"

namespace z8
{

using lol::msg;

player::player(lol::ivec2 window_size,
               bool is_raccoon)
  : m_input_map
    {
        { lol::input::key::SC_Left, 0 },
        { lol::input::key::SC_Right, 1 },
        { lol::input::key::SC_Up, 2 },
        { lol::input::key::SC_Down, 3 },
        { lol::input::key::SC_Z, 4 },
        { lol::input::key::SC_C, 4 },
        { lol::input::key::SC_N, 4 },
        { lol::input::key::SC_Insert, 4 },
        { lol::input::key::SC_X, 5 },
        { lol::input::key::SC_V, 5 },
        { lol::input::key::SC_M, 5 },
        { lol::input::key::SC_Delete, 5 },

        { lol::input::key::SC_P, 6 },
        { lol::input::key::SC_Return, 6 },

        { lol::input::key::SC_S, 8 },
        { lol::input::key::SC_F, 9 },
        { lol::input::key::SC_E, 10 },
        { lol::input::key::SC_D, 11 },
        { lol::input::key::SC_LShift, 12 },
        { lol::input::key::SC_A, 12 },
        { lol::input::key::SC_Q, 13 },
        { lol::input::key::SC_Tab, 13 },
    }
{
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
    m_scenecam->SetProjection(lol::mat4::ortho(0.f, (float)window_size.x, 0.f, (float)window_size.y, -100.f, 100.f));
    lol::Scene& scene = lol::Scene::GetScene();
    scene.PushCamera(m_scenecam);
    lol::Ticker::Ref(m_scenecam);

    // Register audio callbacks
    for (int i = 0; i < 4; ++i)
    {
        auto f = m_vm->get_streamer(i);
        m_streams[i] = lol::audio::start_streaming(f, lol::audio::format::sint16le, 22050, 1);
    }

    // FIXME: the image gets deleted by TextureImage class, it
    // does not seem right to me.
    auto img = new lol::image(lol::ivec2(128, 128));
    img->unlock(img->lock<lol::PixelFormat::RGBA_8>()); // ensure RGBA_8 is present
    m_tile = lol::TileSet::create("tile", new lol::image(*img), lol::ivec2(128, 128), lol::ivec2(1, 1));

    img = new lol::image(lol::ivec2(128, 32));
    img->unlock(img->lock<lol::PixelFormat::RGBA_8>()); // ensure RGBA_8 is present
    m_font_tile = lol::TileSet::create("font", new lol::image(*img), lol::ivec2(128, 32), lol::ivec2(1, 1));

    /* Allocate memory */
    m_screen.resize(128 * 128);
}

player::~player()
{
    lol::TileSet::destroy(m_tile);
    lol::TileSet::destroy(m_font_tile);

    for (int i = 0; i < 4; ++i)
        lol::audio::stop_streaming(i);

    lol::Scene& scene = lol::Scene::GetScene();
    lol::Ticker::Unref(m_scenecam);
    scene.PopCamera(m_scenecam);
}

void player::load(char const *name)
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

    auto mouse = lol::input::mouse();
    auto keyboard = lol::input::keyboard();

    // Update button states
    for (auto const &k : m_input_map)
        m_vm->button(k.second, keyboard->key(k.first));

    if (keyboard->key_pressed(lol::input::key::SC_Return))
        m_vm->keyboard('\r');
    if (keyboard->key_pressed(lol::input::key::SC_Backspace))
        m_vm->keyboard('\x08');
    if (keyboard->key_pressed(lol::input::key::SC_Delete))
        m_vm->keyboard('\x7f');

    // Mouse events
    lol::ivec2 mousepos((int)mouse->axis(lol::input::axis::ScreenX),
                        (int)mouse->axis(lol::input::axis::ScreenY));
    int buttons = (mouse->button(lol::input::button::BTN_Left) ? 1 : 0)
                + (mouse->button(lol::input::button::BTN_Right) ? 2 : 0)
                + (mouse->button(lol::input::button::BTN_Middle) ? 4 : 0);
    m_vm->mouse(lol::ivec2(mousepos.x - (WINDOW_WIDTH - SCREEN_WIDTH) / 2,
                           WINDOW_HEIGHT - 1 - mousepos.y - (WINDOW_HEIGHT - SCREEN_HEIGHT) / 2) / 4,
                buttons);

    // Keyboard events
    for (auto ch : keyboard->text())
    {
        // Convert uppercase characters to special glyphs
        if (ch >= 'A' && ch <= 'Z')
            ch = '\x80' + (ch - 'A');
        m_vm->keyboard(ch);
    }

    // Step the VM
    m_vm->step(seconds);
}

void player::tick_draw(float seconds, lol::Scene &scene)
{
    lol::WorldEntity::tick_draw(seconds, scene);

    // Render the VM screen to our buffer
    m_vm->render(m_screen.data());

    if (m_vm->m_bios) // FIXME: PICO-8 specific
    {
        // Render the font
        u8vec4 data[128 * 32];
        for (int j = 0; j < 32; ++j)
        for (int i = 0; i < 128; ++i)
        {
            auto p = m_vm->m_bios->get_spixel(i, j);
            data[j * 128 + i] = p ? palette::get8(p) : u8vec4(0);
        }
        m_font_tile->GetTexture()->Bind();
        m_font_tile->GetTexture()->SetData(data);
    }

    // Blit buffer to the texture
    // FIXME: move this to some kind of memory viewer class?
    m_tile->GetTexture()->Bind();
    m_tile->GetTexture()->SetData(m_screen.data());

    // Special mode where we render ourselves
    if (m_render)
    {
        scene.get_renderer()->clear_color(lol::Color::black);

        float delta_x = 0.5f * (WINDOW_WIDTH - SCREEN_WIDTH);
        float delta_y = 0.5f * (WINDOW_HEIGHT - SCREEN_HEIGHT);
        float scale = (float)SCREEN_WIDTH / 128;
        scene.AddTile(m_tile, 0, lol::vec3(delta_x, delta_y, 10.f), lol::vec2(scale), 0.f);
    }
}

lol::Texture *player::get_texture()
{
    m_render = false; // Someone else wants to render us
    return m_tile ? m_tile->GetTexture() : nullptr;
}

lol::Texture *player::get_font_texture()
{
    return m_font_tile ? m_font_tile->GetTexture() : nullptr;
}

} // namespace z8

