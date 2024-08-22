#include <stdio.h>
#include <GLFW/glfw3.h>


int main(int argc, const char** argv){
	GLFWwindow* window;

	if(!glfwInit()){
		printf("Couldn't initialize window\n");
		return 1;
	}

	window = glfwCreateWindow(640, 480, "Hello world", nullptr, nullptr);
	if(!window){
		printf("Couldn't open window\n");
		return 1;
	}
	glfwMakeContextCurrent(window);

	while(!glfwWindowShouldClose(window)){
		glfwWaitEvents();
	}
	return 0;
}