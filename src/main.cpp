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

	unsigned char* data = new unsigned char[100*100*3];
	for(int y=0; y < 100; ++y){
		for(int x=0; x < 100; ++x){
			data[y*100*3 + x*3] = 0xff;  //Red
			data[y*100*3 + x*3 + 1] = 0; //Green
			data[y*100*3 + x*3 + 2] = 0; //Blue
		}
	}
	//Crear la ventana.
	glfwMakeContextCurrent(window);
	// Configuración del viewport
	glViewport(0, 0, 640, 480);
	// Configuración de la proyección ortogonal
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 640.0, 0.0, 480.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	while(!glfwWindowShouldClose(window)){

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawPixels(100, 100, GL_RGB, GL_UNSIGNED_BYTE, data);
		glfwSwapBuffers(window);

		glfwPollEvents();
	}
	return 0;
}