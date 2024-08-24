#include <stdio.h>
#include <GLFW/glfw3.h>
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
}


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
	//Buffer donde se renderiza la matriz de píxeles, byte a byte (RGB->3Bytes)
	unsigned char* data = new unsigned char[100*100*3];
	for(int y=0; y < 100; ++y){
		for(int x=0; x < 100; ++x){
			data[y*100*3 + x*3] = 0xff;  //Red
			data[y*100*3 + x*3 + 1] = 0x00; //Green
			data[y*100*3 + x*3 + 2] = 0x00; //Blue
		}
	}
		for(int y=25; y < 75; ++y){
		for(int x=25; x < 75; ++x){
			data[y*100*3 + x*3] = 0x00;  //Red
			data[y*100*3 + x*3 + 1] = 0x00; //Green
			data[y*100*3 + x*3 + 2] = 0xff; //Blue
		}
	}
	//Crear la ventana.
	glfwMakeContextCurrent(window);
	//Crear una textura
	GLuint tex_handle;
	int text_width = 100;
	int text_height = 100;
	glGenTextures(1, &tex_handle);
	glBindTexture(GL_TEXTURE_2D, tex_handle);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 100, 100, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	while(!glfwWindowShouldClose(window)){
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Configuración de la proyección ortogonal
		int window_width, window_height;
		glViewport(0, 0, window_width, window_height);
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, window_width, 0, window_height, -1, 1);
		glMatrixMode(GL_MODELVIEW);

		//render texture
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex_handle);
		glBegin(GL_QUADS);
			glTexCoord2d(0, 0); glVertex2i(0, 0); 
			glTexCoord2d(1, 0); glVertex2i(text_width, 0);
			glTexCoord2d(1, 1); glVertex2i(text_width, text_height);
			glTexCoord2d(0, 1); glVertex2i(0, text_height);
		glEnd();
		glDisable(GL_TEXTURE_2D);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	return 0;
}