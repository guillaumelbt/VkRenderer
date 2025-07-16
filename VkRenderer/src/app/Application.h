#pragma once
#include "window/Window.h"
#include "core/Device.h"
#include "core/Renderer.h"
#include "model/Model.h"
#include "model/GameObject.h"
#include <vector>
#include <stdexcept>
#include <memory>
#include <array>
#include "camera/Camera.h"
#include "core/Descriptors.h"
#include "systems/EntityComponentSystem.h"
#include "ui/ImGuiInterface.h"
#include <vector>
#include <string>

class Application
{
public :
	static constexpr int WIDTH = 1920;
	static constexpr int HEIGHT = 1080;

	Application();
	~Application();

	Application(const Application&) = delete;
	void operator =(const Application&) = delete;


	void Run();

private:
	void LoadGameObjects();

	Window m_window{ WIDTH,HEIGHT,"VkRenderer" };
	Device m_device{ m_window };
	Renderer m_renderer { m_window, m_device };

	std::unique_ptr<DescriptorPool> m_globalPool{};
	EntityComponentSystem m_ec;
	Entity m_viewerEntity;
	Entity m_particleEntity;
	
	std::unique_ptr<ImGuiInterface> m_imguiInterface;
};

