#pragma once
#include <vector>
#include <filesystem>
#include <Windows.h>
#include "rtre.h"
#include "GLFW/rtre_Window.h"
#include "engine_movement/controller.h"

#define LOG(x) std::cout << x << "\n"

void prv(glm::vec3 v) {
	std::cout << v.x << " " << v.y << " " << v.z << "\n";
}

float getTime() {
	using std::chrono::milliseconds;
	using std::chrono::duration_cast;
	return (float)duration_cast<milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}


glm::mat4 matrix(const rtre::Camera& c) {
	auto view = glm::lookAt(c.position(), c.position() + c.orientation(), glm::vec3(0, 1, 0));
	return glm::inverse(view);
	
}

// don't flip the state of it, causes black screen?
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{	
	int cursorMode = glfwGetInputMode(window, GLFW_CURSOR);
	if (key == GLFW_KEY_F && action == GLFW_PRESS) {

		if (cursorMode == GLFW_CURSOR_DISABLED)
			cursorMode = GLFW_CURSOR_NORMAL;
		else
			cursorMode = GLFW_CURSOR_DISABLED;

		glfwSetInputMode(window, GLFW_CURSOR, cursorMode);
	}
}


namespace fs = std::filesystem;

int main()
{

	rtre::Window::init();
	rtre::Window::initHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	rtre::Window::initHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	rtre::Window::initHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	rtre::Window window(800, 480,"Engine");
	window.makeContextCurrent();
	rtre::init(1000, 1000, window,glm::vec3(5,5,5));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window.getWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 430");

	bool show_another_window = true;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	std::shared_ptr<rtre::RenderShader> shader = std::make_shared<rtre::RenderShader>("engine_resources\\vert.vert", "engine_resources\\frag.frag", "");
	rtre::Quad screen = rtre::Quad(shader);
	GLfloat fov = 75.f;


	float stime = getTime();
	
	// Uniforms
	glm::vec4 matColor = glm::vec4(1.0, 0.32, 0.32, 1.0);
	glm::vec3 sphereloc = glm::vec3(0,0,3);
	glm::vec3 boxPos = glm::vec3(1, 1, 3);
	glm::vec3 boxBounds = glm::vec3(1, 1, 1);
	glm::vec3 lightPos = glm::vec3(5, 5, 5);
	glm::vec3 skyColor = glm::vec3(0.05, 0.65, 0.86);
	GLfloat sphereRadius = 0.5f;
	GLfloat thresh = 0.005;
	GLfloat shadowK = 8.0;
	GLfloat smoothing = 1.2f;
	GLfloat speed = 5;
	GLfloat time = 0;
	GLfloat modulo = 6;
	GLint maxits = 120;

	while (!window.shouldClose() && !window.isKeyPressed(GLFW_KEY_ESCAPE)) {

		screen.m_Shader->checkAndHotplug();
		rtre::Window::pollEvents();

		int display_w, display_h;


		glfwGetFramebufferSize(window.getWindow(), &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (show_another_window)
		{
			ImGui::Begin("Control Panel", &show_another_window);

			ImGui::SliderFloat3("Sphere Position", (float*)&sphereloc, -10, 10);
			ImGui::SliderFloat("Sphere Radius", &sphereRadius, 0, 10);

			ImGui::SliderFloat3("Box Position", (float*)&boxPos, -10, 10);
			ImGui::SliderFloat3("Box Boundaries", (float*)&boxBounds, 0, 15);

			ImGui::SliderFloat3("Light Position", (float*)&lightPos, -15, 15);


			ImGui::ColorEdit3("Material Color", (float*)&matColor);
			ImGui::ColorEdit3("Sky Color", (float*)&skyColor);

			ImGui::SliderFloat("Smoothing", &smoothing, 0, 10);
			ImGui::SliderFloat("ShadowK", &shadowK, 0, 128);
			ImGui::SliderFloat("modulo", &modulo, 0, 128);

			ImGui::SliderFloat("Speed", (float*)&speed, 5, 100, "%.1f");
			ImGui::DragInt("Max Iterations", &maxits, 1.f, 1, 1000);
			if (ImGui::Button("Reset position")) {
				rtre::camera.setPosition(glm::vec3(1));
				rtre::camera.setOrientation(glm::vec3(0,0,-1));
			}


			rtre::camera.setSpeed(glm::vec3(speed/1000000));

			ImGui::End();
		}

		GLfloat aspectRatio = aspectRatio = float(display_w) / display_h;

		screen.m_Shader->SetUniform("cameraPos", rtre::camera.position());
		screen.m_Shader->SetUniform("time", time);
		screen.m_Shader->SetUniform("aspec", aspectRatio);
		screen.m_Shader->SetUniform("sphereloc", sphereloc);
		screen.m_Shader->SetUniform("sphereRadius", sphereRadius);
		screen.m_Shader->SetUniform("maxits", maxits);
		screen.m_Shader->SetUniform("thresh", thresh);
		screen.m_Shader->SetUniform("matrix", matrix(rtre::camera));
		screen.m_Shader->SetUniform("orientation", rtre::camera.orientation());
		screen.m_Shader->SetUniform("matColor", matColor);
		screen.m_Shader->SetUniform("skyColor", skyColor);
		screen.m_Shader->SetUniform("boxPos", boxPos);
		screen.m_Shader->SetUniform("boxBounds", boxBounds);
		screen.m_Shader->SetUniform("lightPos", lightPos);
		screen.m_Shader->SetUniform("smoothing", smoothing);
		screen.m_Shader->SetUniform("shadowK", shadowK);
		screen.m_Shader->SetUniform("modulo", modulo);
		screen.draw();



		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSetKeyCallback(window.getWindow(), key_callback);

		time = getTime() - stime;

		rtre::controller::control();
		window.swapInterval(1);
		window.swapBuffers();
	}


	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	rtre::Window::terminate();
}
