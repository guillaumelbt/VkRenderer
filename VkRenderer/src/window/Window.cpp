#include "window/Window.h"


Window::Window(int width, int height, std::string name) : m_width{ width }, m_height{ height }, m_windowName{ name }
{
	InitWindow();
}

Window::~Window()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Window::InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(m_width, m_height, m_windowName.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);

}

void Window::CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS)
		throw std::exception("falied to create window surface");
}


void Window::FramebufferResizeCallback(GLFWwindow* _window, int _width, int _height) 
{
	auto window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(_window));
	window->m_framebufferResized = true;
	window->m_width = _width;
	window->m_height = _height;
}