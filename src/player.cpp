//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2018 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

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

    m_input << lol::InputProfile::MouseKey(0, "Left");
    m_input << lol::InputProfile::MouseKey(1, "Right");
    m_input << lol::InputProfile::MouseKey(2, "Middle");

    m_input << lol::InputProfile::Keyboard(3, "Left");
    m_input << lol::InputProfile::Keyboard(4, "Right");
    m_input << lol::InputProfile::Keyboard(5, "Up");
    m_input << lol::InputProfile::Keyboard(6, "Down");
    m_input << lol::InputProfile::Keyboard(7, "Z");
    m_input << lol::InputProfile::Keyboard(7, "C");
    m_input << lol::InputProfile::Keyboard(7, "N");
    m_input << lol::InputProfile::Keyboard(7, "Insert");
    m_input << lol::InputProfile::Keyboard(8, "X");
    m_input << lol::InputProfile::Keyboard(8, "V");
    m_input << lol::InputProfile::Keyboard(8, "M");
    m_input << lol::InputProfile::Keyboard(8, "Delete");

    m_input << lol::InputProfile::Keyboard(9, "Return");
    m_input << lol::InputProfile::Keyboard(9, "P");

    m_input << lol::InputProfile::Keyboard(11, "S");
    m_input << lol::InputProfile::Keyboard(12, "F");
    m_input << lol::InputProfile::Keyboard(13, "E");
    m_input << lol::InputProfile::Keyboard(14, "D");
    m_input << lol::InputProfile::Keyboard(15, "LShift");
    m_input << lol::InputProfile::Keyboard(15, "A");
    m_input << lol::InputProfile::Keyboard(16, "Tab");
    m_input << lol::InputProfile::Keyboard(16, "Q");

    m_mouse = lol::InputDevice::GetMouse();

    m_controller->Init(m_input);
    // 64 keyboard / joystick buttons + 3 mouse buttons
    m_controller->SetInputCount(67 /* keys */, 0 /* axes */);

    // Allow text input
    m_keyboard = lol::InputDevice::GetKeyboard();

    // Create an ortho camera
    m_scenecam = new lol::Camera();
    m_scenecam->SetView(lol::mat4(1.f));
    m_scenecam->SetProjection(lol::mat4::ortho(0.f, window_size.x, 0.f, window_size.y, -100.f, 100.f));
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
    m_tile = lol::Tiler::Register("fuck", new lol::image(*img), lol::ivec2(128, 128), lol::ivec2(1, 1));

    /* Allocate memory */
    m_screen.resize(128 * 128);
}

player::~player()
{
    lol::Tiler::Deregister(m_tile);

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

void player::TickGame(float seconds)
{
    lol::WorldEntity::TickGame(seconds);

    // Update button states
    for (int i = 0; i < 64; ++i)
        m_vm.button(i, m_controller->IsKeyPressed(3 + i));

    if (m_mouse)
    {
        lol::ivec2 mousepos = m_mouse->GetCursorPixel(0);
        int buttons = (m_controller->IsKeyPressed(0) ? 1 : 0)
                    + (m_controller->IsKeyPressed(1) ? 2 : 0)
                    + (m_controller->IsKeyPressed(2) ? 4 : 0);
        m_vm.mouse(lol::ivec2(mousepos.x - (WINDOW_WIDTH - SCREEN_WIDTH) / 2,
                              WINDOW_HEIGHT - 1 - mousepos.y - (WINDOW_HEIGHT - SCREEN_HEIGHT) / 2) / 4,
                   buttons);
    }

    if (m_keyboard)
    {
        m_keyboard->SetTextInputActive(true);
        auto text = m_keyboard->GetText();
        for (auto ch : text)
        {
            // Convert uppercase characters to special glyphs
            if (ch >= 'A' && ch <= 'Z')
                ch = ch - 'A' + '\x80';
            m_vm.keyboard(ch);
        }
    }

    // Step the VM
    m_vm.step(seconds);
}

void player::TickDraw(float seconds, lol::Scene &scene)
{
    lol::WorldEntity::TickDraw(seconds, scene);

    lol::Renderer::Get()->SetClearColor(lol::Color::black);

    m_vm.render(m_screen.data());

    m_tile->GetTexture()->Bind();
    m_tile->GetTexture()->SetData(m_screen.data());

    int delta_x = (WINDOW_WIDTH - SCREEN_WIDTH) / 2;
    int delta_y = (WINDOW_HEIGHT - SCREEN_HEIGHT) / 2;
    float scale = (float)SCREEN_WIDTH / 128;
    scene.AddTile(m_tile, 0, lol::vec3(delta_x, delta_y, 10.f), lol::vec2(scale), 0.f);
}

} // namespace z8

