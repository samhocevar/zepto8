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
#include "vm/vm.h"

namespace z8
{

using lol::msg;

player::player(lol::ivec2 window_size)
{
    // Bind controls
    m_controller = new lol::Controller("default controller");

    m_input << lol::InputProfile::MouseKey(100, "Left");
    m_input << lol::InputProfile::MouseKey(101, "Right");
    m_input << lol::InputProfile::MouseKey(102, "Middle");

    m_input << lol::InputProfile::Keyboard(0, "Left");
    m_input << lol::InputProfile::Keyboard(1, "Right");
    m_input << lol::InputProfile::Keyboard(2, "Up");
    m_input << lol::InputProfile::Keyboard(3, "Down");
    m_input << lol::InputProfile::Keyboard(4, "Z");
    m_input << lol::InputProfile::Keyboard(4, "C");
    m_input << lol::InputProfile::Keyboard(4, "N");
    m_input << lol::InputProfile::Keyboard(4, "Insert");
    m_input << lol::InputProfile::Keyboard(5, "X");
    m_input << lol::InputProfile::Keyboard(5, "V");
    m_input << lol::InputProfile::Keyboard(5, "M");
    m_input << lol::InputProfile::Keyboard(5, "Delete");

    m_input << lol::InputProfile::Keyboard(6, "P");
    m_input << lol::InputProfile::Keyboard(6, "Return");

    m_input << lol::InputProfile::Keyboard(8, "S");
    m_input << lol::InputProfile::Keyboard(9, "F");
    m_input << lol::InputProfile::Keyboard(10, "E");
    m_input << lol::InputProfile::Keyboard(11, "D");
    m_input << lol::InputProfile::Keyboard(12, "LShift");
    m_input << lol::InputProfile::Keyboard(12, "A");
    m_input << lol::InputProfile::Keyboard(13, "Q");
    m_input << lol::InputProfile::Keyboard(13, "Tab");

    m_mouse = lol::InputDevice::GetMouse();

    m_controller->Init(m_input);

    // Allow text input
    m_keyboard = lol::InputDevice::GetKeyboard();
    if (m_keyboard)
        m_keyboard->SetTextInputActive(true);

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
        auto f = std::bind(&vm::getaudio, &m_vm, i,
                           std::placeholders::_1,
                           std::placeholders::_2);
        m_streams[i] = lol::audio::start_streaming(f);
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
    m_vm.load(name);
}

void player::run()
{
    m_vm.run();
}

void player::tick_game(float seconds)
{
    lol::WorldEntity::tick_game(seconds);

    // Update button states
    for (int i = 0; i < 64; ++i)
        m_vm.button(i, m_controller->IsKeyPressed(i));

    // Debug the keyboard input
    auto const &keys = m_keyboard->keys();
    for (size_t i = 0; i < keys.size(); ++i)
        if (keys[i])
            msg::info("input: %s\n", m_keyboard->key_names()[i].c_str());

    if (m_mouse)
    {
        lol::ivec2 mousepos = m_mouse->GetCursorPixel(0);
        int buttons = (m_controller->IsKeyPressed(100) ? 1 : 0)
                    + (m_controller->IsKeyPressed(101) ? 2 : 0)
                    + (m_controller->IsKeyPressed(102) ? 4 : 0);
        m_vm.mouse(lol::ivec2(mousepos.x - (WINDOW_WIDTH - SCREEN_WIDTH) / 2,
                              WINDOW_HEIGHT - 1 - mousepos.y - (WINDOW_HEIGHT - SCREEN_HEIGHT) / 2) / 4,
                   buttons);
    }

    if (m_keyboard)
    {
        auto text = m_keyboard->GetText();
        for (auto ch : text)
        {
            // Convert uppercase characters to special glyphs
            if (ch >= 'A' && ch <= 'Z')
                ch = ch - 'A' + '\x80';
            // PICO-8 compatibility
            if (ch == '\n')
                ch = '\r';
            m_vm.keyboard(ch);
        }
    }

    // Step the VM
    m_vm.step(seconds);
}

void player::tick_draw(float seconds, lol::Scene &scene)
{
    lol::WorldEntity::tick_draw(seconds, scene);

    // Render the VM screen to our buffer
    m_vm.render(m_screen.data());

    {
        // Render the font
        u8vec4 data[128 * 32];
        for (int j = 0; j < 32; ++j)
        for (int i = 0; i < 128; ++i)
        {
            auto p = m_vm.m_bios.get_spixel(i, j);
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
        scene.get_renderer()->SetClearColor(lol::Color::black);

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

