//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#include <lol/engine.h>

#include "player.h"
#include "vm.h"

namespace z8
{

using lol::msg;

player::player()
{
    // Bind controls
    m_controller = new lol::Controller("default controller");

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

    m_input << lol::InputProfile::Keyboard(6, "Return");

    m_input << lol::InputProfile::Keyboard(8, "S");
    m_input << lol::InputProfile::Keyboard(9, "F");
    m_input << lol::InputProfile::Keyboard(10, "E");
    m_input << lol::InputProfile::Keyboard(11, "D");
    m_input << lol::InputProfile::Keyboard(12, "LShift");
    m_input << lol::InputProfile::Keyboard(12, "A");
    m_input << lol::InputProfile::Keyboard(13, "Tab");
    m_input << lol::InputProfile::Keyboard(13, "Q");

    m_controller->Init(m_input);
    m_controller->SetInputCount(64 /* keys */, 0 /* axes */);

    // Create an ortho camera
    m_scenecam = new lol::Camera();
    m_scenecam->SetView(lol::mat4(1.f));
    m_scenecam->SetProjection(lol::mat4::ortho(0.f, 600.f, 0.f, 600.f, -100.f, 100.f));
    lol::Scene& scene = lol::Scene::GetScene();
    scene.PushCamera(m_scenecam);
    lol::Ticker::Ref(m_scenecam);

    // FIXME: the image gets deleted by TextureImage class, it
    // does not seem right to me.
    auto img = new lol::Image(lol::ivec2(128, 128));
    img->Unlock(img->Lock<lol::PixelFormat::RGBA_8>()); // ensure RGBA_8 is present
    m_tile = lol::Tiler::Register("fuck", new lol::Image(*img), lol::ivec2(128, 128), lol::ivec2(1, 1));

    /* Allocate memory */
    m_screen.resize(128 * 128);
}

player::~player()
{
    lol::Tiler::Deregister(m_tile);

    lol::Scene& scene = lol::Scene::GetScene();
    lol::Ticker::Unref(m_scenecam);
    scene.PopCamera(m_scenecam);
}

void player::TickGame(float seconds)
{
    lol::WorldEntity::TickGame(seconds);

    // Update button states
    for (int i = 0; i < 64; ++i)
        m_vm.button(i, m_controller->IsKeyPressed(i));

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

    int delta = (600 - 512) / 2;
    scene.AddTile(m_tile, 0, lol::vec3(delta, delta, 10.f), 0, lol::vec2(4.f), 0.f);
}

} // namespace z8

