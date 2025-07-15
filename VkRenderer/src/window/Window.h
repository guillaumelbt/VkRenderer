#pragma once
#define GLFW_INCLUDE_VULKAN
#include "glfw/glfw3.h"
#include <glm/gtc/constants.hpp>

#include <string>

class Window
{
public:
	Window(int width, int height, std::string name);
	~Window();
	
	Window(const Window&) = delete;
	void operator=(const Window&) = delete;

	VkExtent2D GetExtent() { return { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) }; };


	bool ShouldClose() { return glfwWindowShouldClose(m_window); };
	bool WasWindowResized() { return m_framebufferResized; }
	void ResetWindowResizedFlag() { m_framebufferResized = false; }

	GLFWwindow* GetWindow() { return m_window; };

	void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

private:
	void InitWindow();

	static void FramebufferResizeCallback(GLFWwindow* _window, int _width, int _height);

	int m_width;
	int m_height;

	bool m_framebufferResized = false;

	std::string m_windowName;
	GLFWwindow* m_window;

};

